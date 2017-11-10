#include <iostream>
#include "osm/osmServer/utils.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/globalLeader/global_leader.h"
#include "osm/localLeader/local_leader.h"
#include "osm/localLeader/ll_msg_handler.h"
#include "libraryUtils/osd_logger_iniliazer.h"
#include "osm/osmServer/osm_daemon.h"
#include "communication/message_type_enum.pb.h"
#include "communication/callback.h"

MainThreadHandler::MainThreadHandler(
	boost::shared_ptr<global_leader::GlobalLeader> &gl_obj,
	boost::shared_ptr<gl_parser::GLMsgHandler> &gl_msg_ptr,
	boost::shared_ptr<boost::thread> &gl_thread,
	boost::shared_ptr<common_parser::MessageParser> &parser_obj,
	boost::shared_ptr<local_leader::LocalLeader> &ll_ptr,
	boost::shared_ptr<Listener> &listener_ptr,
	boost::shared_ptr<boost::thread> &listener_thread,
	cool::SharedPtr<config_file::OSMConfigFileParser> config_parser,
	int32_t osm_port,
	bool &if_i_am_gl,
	boost::shared_ptr<common_parser::GLMsgProvider> gl_msg_provider_obj
)
:
	gl_obj(gl_obj),
	gl_msg_ptr(gl_msg_ptr),
	gl_thread(gl_thread),
	parser_obj(parser_obj),
	ll_ptr(ll_ptr),
	listener_ptr(listener_ptr),
	listener_thread(listener_thread),
	config_parser(config_parser),
	osm_port(osm_port),
	if_i_am_gl(if_i_am_gl),
	gl_msg_provider_obj(gl_msg_provider_obj),
	mount_retry_count(0)
{
}

void MainThreadHandler::push(boost::shared_ptr<MessageCommunicationWrapper> msg)
{
	boost::mutex::scoped_lock lock(this->mtx);
	this->main_thread_queue.push(msg);
	this->main_thread_cond.notify_one();
}

boost::shared_ptr<MessageCommunicationWrapper> MainThreadHandler::pop()
{
	boost::mutex::scoped_lock lock(this->mtx);
	while(this->main_thread_queue.empty())
	{
		this->main_thread_cond.wait(lock);
	}
	boost::shared_ptr<MessageCommunicationWrapper> msg = this->main_thread_queue.front();
	this->main_thread_queue.pop();
	return msg;
}

void MainThreadHandler::handle_msg()
{
	bool service_running = true;
	while(service_running)
	{
		boost::shared_ptr<MessageCommunicationWrapper> msg = this->pop();
		if (msg)
		{
			const uint32_t msg_type = msg->get_msg_result_wrap()->get_type();
			switch(msg_type)
			{
				case common_parser::NODE_STOP: 	//Stop all threads
				{
					listener_thread->interrupt();
					listener_ptr->close_all_sockets();
					listener_thread->join();
					this->listener_ptr.reset();
					if(if_i_am_gl)
					{
						if(this->gl_obj)
						{
						OSDLOG(INFO, "Closing GL thread");
//						this->gl_thread->interrupt();
//						this->gl_thread->join();
						this->gl_obj->abort();
						}
					}
					//this->ll_ptr->terminate_all_active_thread();
					service_running = false;
					break;
				}
				case common_parser::GL_STOP:
				{
					//this->gl_thread->interrupt();
					this->gl_obj->abort();
					break;
				}
				case messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP:
				{
					this->gl_obj.reset();
                                        this->gl_msg_ptr.reset();
					boost::shared_ptr<comm_messages::TakeGlOwnership> own_obj(new comm_messages::TakeGlOwnership);
                                        own_obj->deserialize(msg->get_msg_result_wrap()->get_payload(), msg->get_msg_result_wrap()->get_size());
					OSDLOG(INFO, "Take GL ownership msg rcvd");
					//OSDLOG(DEBUG, "Let pre-conditions fail, restart old GL now");
					//boost::this_thread::sleep(boost::posix_time::seconds(20));
					std::pair<bool, std::string> pre_cond = this->verify_election_pre_conditions();
					//OSDLOG(DEBUG, "Let Updation in GL info file fail, restart old GL now");
					//boost::this_thread::sleep(boost::posix_time::seconds(20));
					bool status = false;
					if(pre_cond.first)
					{
						bool umount_status = this->unmount_gl_journal_fs(own_obj->get_old_gl_id());
						if(umount_status)	//unmount successful
						{
							bool updated = this->update_gl_info_file(pre_cond.second, this->osm_port);
							if(updated)	//if GL info file updated successfully
							{
								status = this->mount_gl_journal_fs(
									own_obj->get_old_gl_id(),
									own_obj->get_new_gl_id()
								);
								if(status)	//If mount done successfully
								{
									instantiate_GL(
										this->gl_obj,
										this->gl_msg_ptr,
										this->gl_thread,
										this->parser_obj,
										true,
										true,
										this->osm_port,
										false,
										true,
										this->gl_msg_provider_obj
									);
									this->send_ownership_notification(
										own_obj->get_new_gl_id(),
										msg->get_comm_object(),
										OSDException::OSDSuccessCode::ELECTION_SUCCESS
									);
								}
								else		//Mount failure
								{
									this->send_ownership_notification(
										own_obj->get_new_gl_id(),
										msg->get_comm_object(),
										OSDException::OSDSuccessCode::ELECTION_FAILURE
									);
								}
							}
							else	//if GL info file update failed, may be due to old GL
							{
								this->send_ownership_notification(
									own_obj->get_new_gl_id(),
									msg->get_comm_object(),
									OSDException::OSDSuccessCode::RE_ELECTION_NOT_REQUIRED
								);
							}
						}	
						else	//Unmount failed, busy or some other reason
						{
							this->send_ownership_notification(
									own_obj->get_new_gl_id(),
									msg->get_comm_object(),
									OSDException::OSDSuccessCode::ELECTION_FAILURE
									);
						}
					}
					else		//Pre-condition failure like lock and gl info file version mismatch
					{
						this->send_ownership_notification(
							own_obj->get_new_gl_id(),
							msg->get_comm_object(),
							OSDException::OSDSuccessCode::RE_ELECTION_NOT_REQUIRED
						);
					}
					break;
				}
				default:
					break;
			}
		}
		else
		{
			break;
		}
	}
}

// TO-DO: Later there will be any mechanism through which LL of the node having GL will not participating in Election 
bool MainThreadHandler::unmount_gl_journal_fs(std::string old_gl_id)
{
	GLUtils util_obj;
	OSDLOG(INFO, "Old GL service name is " << old_gl_id);
	std::string old_gl_node_name = util_obj.get_node_name(old_gl_id);
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> config_timeout_parser = util_obj.get_osm_timeout_config();
	std::string journal_path = this->config_parser->journal_pathValue();
	std::string journal_name = journal_path.substr(journal_path.find_last_of("/")+1, journal_path.size());
	CommandExecutor exec;
	std::string umount_cmd = "sudo -u hydragui hydra_ssh " + old_gl_node_name + " " + this->config_parser->UMOUNT_HYDRAFSValue() + " -d " + journal_name;
	OSDLOG(INFO, "Cmd for unmounting fs is " << umount_cmd);
	boost::system_time timeout = boost::get_system_time()+ boost::posix_time::seconds(config_timeout_parser->fs_mount_timeoutValue());
	int retry = 0;
	while(retry < 2)
	{
		int ret = exec.execute_cmd_in_thread(umount_cmd, timeout);
		if(ret == 255)
			ret = 0;
		
		if(ret == -1)
		{
			return false;
		}
		else if(ret != 0)
		{
			if(ret == 12)
				return true;
			else if(ret == 9 and retry < 1)	//Flow should enter this loop only on first un-mount failure
			{
				OSDLOG(ERROR, "HFS services on the old GL node is down. Will retry after 2 minutes");
				boost::this_thread::sleep(boost::posix_time::seconds(120));
			}
			else if(ret == 9 and retry >= 1)
			{
				OSDLOG(INFO, "HFS services are not coming up. Assuming un-mount successful");
				return true;
			}else if(ret == 10) { // When old GL is updating global leader journal
				OSDLOG(INFO, "OLD GL is performing some operation on journal");
				return false;
			}

		}
		else
		{
			break;
		}
		retry++;
	}
	boost::this_thread::sleep(boost::posix_time::seconds(10));	//Time gap is needed between unmount and mount
	return true;

}


std::pair<bool, std::string> MainThreadHandler::verify_election_pre_conditions()
{
	OsmUtils osm_utils;
	std::vector<std::string> fileList = osm_utils.list_files_in_creation_order(this->config_parser->osm_info_file_pathValue(), "LOCK");
	//fileList.sort();
	std::string latest_lock_file = fileList.back();
	GLUtils util_obj;
	std::string lock_file_version = util_obj.get_version_from_file_name(latest_lock_file);
	boost::tuple<std::string, bool> gl_info_file = util_obj.find_file(this->config_parser->osm_info_file_pathValue(), GLOBAL_LEADER_INFORMATION_FILE);
	std::string gl_info_file_version;
	if(boost::get<1>(gl_info_file))
	{
		gl_info_file_version = util_obj.get_version_from_file_name(boost::get<0>(gl_info_file));
		if(gl_info_file_version == lock_file_version)
		{
			return std::make_pair(true, gl_info_file_version);
		}
		else
		{
			OSDLOG(WARNING, "Lock file version " << lock_file_version << " and gl info file version mismatch, cannot continue");
			return std::make_pair(false, gl_info_file_version);
		}
	}
	return std::make_pair(false, gl_info_file_version);
}

void MainThreadHandler::send_ownership_notification(
	std::string new_gl_id,
	boost::shared_ptr<Communication::SynchronousCommunication> comm,
	int32_t status
)
{
	boost::shared_ptr<ErrorStatus> err_status;
	err_status.reset(new ErrorStatus(status, "Ownership response to Election co-ordinator"));
	boost::shared_ptr<comm_messages::TakeGlOwnershipAck> own_ack(new comm_messages::TakeGlOwnershipAck(new_gl_id, err_status));
	SuccessCallBack callback_obj;
	boost::shared_ptr<MessageResultWrap> result(new MessageResultWrap(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP_ACK));
	MessageExternalInterface::sync_send_take_gl_ownership_ack(
					own_ack.get(),
					comm,
					boost::bind(&SuccessCallBack::success_handler, callback_obj, result),
					boost::bind(&SuccessCallBack::failure_handler, callback_obj)
					);
	OSDLOG(DEBUG, "Sent ownership msg to election co-ordinator");

}

bool MainThreadHandler::mount_gl_journal_fs(std::string old_gl_id, std::string new_gl_id)
{
	GLUtils util_obj;
//	std::string gl_info = util_obj.read_GL_file();
//	std::string gl_service_id = gl_info.substr(0, gl_info.find("\t"));
	std::string new_gl_node_name = util_obj.get_node_name(new_gl_id);
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> config_timeout_parser = util_obj.get_osm_timeout_config();
	std::string journal_path = this->config_parser->journal_pathValue();
	std::string journal_name = journal_path.substr(journal_path.find_last_of("/")+1, journal_path.size());
	CommandExecutor exec;
/*	std::string umount_cmd = "sudo -u hydragui hydra_ssh " + old_gl_node_name + " " + this->config_parser->UMOUNT_HYDRAFSValue() + " -d " + journal_name;
	OSDLOG(INFO, "Cmd for unmounting fs is " << umount_cmd);
	boost::system_time timeout = boost::get_system_time()+ boost::posix_time::seconds(config_timeout_parser->fs_mount_timeoutValue());
	int ret = exec.execute_cmd_in_thread(umount_cmd, timeout);
	if (ret == 255)
		ret = 0;

	if(ret != 0)
	{
		if(ret != 12)
			return false;
	}
	boost::this_thread::sleep(boost::posix_time::seconds(10));	//Time gap is needed between unmount and mount
*/	std::string mount_cmd = " sudo mkdir " +  journal_path + "; sudo chmod -R 777 " + journal_path + "; " + this->config_parser->mount_commandValue() + " -O " + journal_name + " " + journal_path;
	this->mount_retry_count += 1;
	boost::system_time timeout = boost::get_system_time()+ boost::posix_time::seconds(config_timeout_parser->fs_mount_timeoutValue());
	OSDLOG(INFO, "Cmd for mounting gl journal fs is " << mount_cmd);
	int ret = exec.execute_cmd_in_thread(mount_cmd, timeout);
	if(ret != 0)
	{
		if(ret == 11)
		{
			this->mount_retry_count = 0;
			return true;
		}
		else if (ret == 9)
		{
			if(this->mount_retry_count <= 3)
			{
				boost::this_thread::sleep(boost::posix_time::seconds(5));
				return this->mount_gl_journal_fs(old_gl_id, new_gl_id);
			}
			else
			{
				this->mount_retry_count = 0;
				return false;
			}
		}
		else
		{
			this->mount_retry_count = 0;
			OSDLOG(ERROR, "Unable to mount the journal fs");
			return false;
		}
	}
	this->mount_retry_count = 0;
	return true;

}

bool MainThreadHandler::update_gl_info_file(std::string gl_info_file_version, int32_t gl_port)
{
	GLUtils util_obj;
	return util_obj.rename_gl_info_file(gl_info_file_version, gl_port);

}

bool create_and_mount_gl_journal()
{
	GLUtils util_obj;
	std::string my_node_name = util_obj.get_my_node_id();
	cool::SharedPtr<config_file::OSMConfigFileParser> config_parser = util_obj.get_config();
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> config_timeout_parser = util_obj.get_osm_timeout_config();
	std::string journal_path = config_parser->journal_pathValue();
	std::string journal_name = journal_path.substr(journal_path.find_last_of("/")+1, journal_path.size());
	//std::string cmd = "sudo -u hydragui rsh anmgmt /usr/sbin/clido fs create name=" + journal_name + " node=" + my_node_name;
	std::string cmd = config_parser->mkfs_commandValue() + " -r copy " + journal_name;
	OSDLOG(DEBUG, "Going to create journal FS " << cmd);

	CommandExecutor cmd_exe;
	boost::system_time timeout = boost::get_system_time()+ boost::posix_time::seconds(config_timeout_parser->fs_create_timeoutValue());
	int status = cmd_exe.execute_cmd_in_thread(cmd, timeout);
	
	if(status == 3 or status == 0)
	{
		OSDLOG(DEBUG, "GL journal FS is created");
	}
	else
		return false;

	cmd = "sudo mkdir " +  journal_path + "; sudo chmod -R 777 " + journal_path + "; " + config_parser->mount_commandValue() + " -O " + journal_name + " " + journal_path;
	OSDLOG(DEBUG, "Going to mount journal FS " << cmd);
	timeout=boost::get_system_time()+ boost::posix_time::seconds(config_timeout_parser->fs_mount_timeoutValue());
	status = cmd_exe.execute_cmd_in_thread(cmd, timeout);

	if(status == 0 or status == 11)
	{
		OSDLOG(INFO, "GL journal fs is mounted");
		return true;
	}
	return false;
}

int instantiate_GL(
	boost::shared_ptr<global_leader::GlobalLeader> &gl_obj,
	boost::shared_ptr<gl_parser::GLMsgHandler> &gl_msg_ptr,
	boost::shared_ptr<boost::thread> &gl_thread,
	boost::shared_ptr<common_parser::MessageParser> &parser_obj,
	bool if_cluster,
	bool env_check,
	int32_t gl_port,
	bool create_flag,
	bool if_by_election,
	boost::shared_ptr<common_parser::GLMsgProvider> &gl_msg_provider_obj
)
{
	OSDLOG(INFO, "Instantiating GL");
	GLUtils util_obj;
	bool if_update;
	if(!if_by_election)	//If not called during election
	{
		if(!util_obj.update_gl_info_file(gl_port))
			return UNABLE_TO_UPDATE_INFO;
	}
	if(create_flag)
	{
		if(!create_and_mount_gl_journal())
		{
			OSDLOG(ERROR, "Unable to create/mount GL Journal FS");
			return UNABLE_TO_MOUNT;
		}
	}
	//boost::shared_ptr<global_leader::GlobalLeader> gl_obj(new global_leader::GlobalLeader);
	gl_obj.reset(new global_leader::GlobalLeader);
	//gl_obj.initialize();
	gl_msg_ptr = gl_obj->get_gl_msg_handler();
	//boost::shared_ptr<gl_parser::GLMsgHandler> gl_msg_ptr(gl_obj->get_gl_msg_handler());
	if(if_cluster and env_check)
	{
		OSDLOG(DEBUG, "Cluster and env check passed, initiating GL");
		//boost::thread gl_main_thread(&global_leader::GlobalLeader::start, gl_obj, true);
		gl_thread.reset(new boost::thread(boost::bind(&global_leader::GlobalLeader::start, gl_obj)));
	}
	else
	{
		// GL should run in wait mode
		gl_thread.reset(new boost::thread(boost::bind(&global_leader::GlobalLeader::start, gl_obj)));
	}

	//------------System Stop File Removal
	cool::SharedPtr<config_file::OSMConfigFileParser> osm_parser_ptr = util_obj.get_config();
	std::string system_stop_file = osm_parser_ptr->system_stop_fileValue();
	if (boost::filesystem::exists(system_stop_file))
	{
		OSDLOG(INFO, "Removing System Stop file: "<< system_stop_file);
		boost::filesystem::remove(boost::filesystem::path(system_stop_file));
	}
	//------------

	if(gl_msg_provider_obj != NULL) {
		OSDLOG(INFO, "unregister message by election");
		gl_msg_provider_obj->unregister_msgs(parser_obj);
	}
	gl_msg_provider_obj.reset(new common_parser::GLMsgProvider);
	gl_msg_provider_obj->register_msgs(parser_obj, gl_msg_ptr);
	return SUCCESS;

}


