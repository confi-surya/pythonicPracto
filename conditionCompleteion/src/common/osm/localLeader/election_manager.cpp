#include <map>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "osm/osmServer/utils.h"
#include "communication/message_interface_handlers.h"
#include "osm/osmServer/osm_timer.h"
#include "osm/osmServer/file_handler.h"
#include "osm/osmServer/osm_info_file.h"
#include "libraryUtils/record_structure.h"
#include "osm/localLeader/election_manager.h"
#include "communication/communication.h"
#include "libraryUtils/object_storage_exception.h"
#include "communication/message_type_enum.pb.h"
#include "communication/callback.h"


using namespace std;

ElectionManager::ElectionManager(std::string id, cool::SharedPtr<config_file::OSMConfigFileParser> config_parser, boost::shared_ptr<HfsChecker> hfs_checker)
{
	this->id = id;
	this->gfs_path = config_parser->osm_info_file_pathValue();
	this->node_info_file_path = config_parser->node_info_file_pathValue();
	this->te_file_wait_timeout = config_parser->te_file_wait_timeoutValue();
	this->t_el_timeout = config_parser->t_el_timeoutValue();
	this->t_notify_timeout = config_parser->t_notify_timeoutValue();
	this->new_GL_response_timeout = config_parser->new_GL_response_timeoutValue();
	this->exportPath = config_parser->export_pathValue();
	this->t_p_el_timeout = this->t_el_timeout + this->t_notify_timeout + DELTA;
	this->election_handler.reset(new ElectionFileHandler(this->gfs_path));
	this->hfs_checker = hfs_checker;
	this->lock_file.reset(new LockFile(this->gfs_path, boost::bind(&HfsChecker::get_gfs_stat, this->hfs_checker)));
}


int ElectionManager::start_election(std::string gl_service_id)
{
	OSDLOG(DEBUG, "Global leader Election started");

	GLUtils utils;
	OsmUtils osm_utils;
	boost::tuple<std::string, bool> file = utils.find_file(
					utils.get_mount_path(this->exportPath, ".osd_meta_config"),
					"osd_global_leader_information_");

	std::string gl_version = osm_utils.split( boost::get<0>(file), "osd_global_leader_information_" ) ;
	if(this->gl_version == gl_version)
	{
		this->election_sub_version += 1;
	}
	else
	{
		this->gl_version = gl_version;
		this->election_sub_version = 0;
	}
	std::string election_version = this->gl_version + "-" + osm_utils.ItoStr(this->election_sub_version);
	OSDLOG(INFO, "Global leader election started with election version: " << election_version);
	try
	{
		int lock_status = this->lock_file->get_lock(election_version);
		if (lock_status == LOCK_NOT_NEEDED)
		{
			OSDLOG(INFO, "HFS stats are updated. No more need of election.");
			return OSDException::OSDSuccessCode::ELECTION_SUCCESS;
		}
		else if (lock_status == LOCK_ACQUIRED)
		{
			OSDLOG(INFO, "Election lock acquired, Proceed as coordinator");
			this->proceed_as_coordinator(gl_service_id);
		}
	        else
		{
			OSDLOG(INFO, "Election already in progress, Proceed as participant");
			int retVal = this->election_handler->temp_file_cleaner(t_p_el_timeout + 60);
			if(retVal != 0)
			{
				OSDLOG(ERROR, "Return Val for temp_file_cleaner : " << retVal);
			}
			this->proceed_as_participent(gl_service_id);
		}
	}
	catch(...)//TODO:Proper handling required for exceptions.
	{
		OSDLOG(ERROR, "Error in election, election failed")
		return -1;//TODO: should return with specific code
	}
	return OSDException::OSDSuccessCode::ELECTION_SUCCESS;
}

int ElectionManager::gl_ownership_notification(std::string old_gl_id, std::string new_gl_id, std::string ip, int32_t port)
{
	boost::shared_ptr<Communication::SynchronousCommunication> com(new Communication::SynchronousCommunication(ip, port));
	boost::shared_ptr<comm_messages::TakeGlOwnership> gl_ownership(new comm_messages::TakeGlOwnership(old_gl_id, new_gl_id));

	if(!com->is_connected())
	{
		OSDLOG(INFO, "Connection could not be established with newly election GL");
		return OSDException::OSDSuccessCode::ELECTION_FAILURE;
	}
	
	OSDLOG(INFO, "Sending take gl ownership message");
	boost::shared_ptr<MessageResultWrap> result(new MessageResultWrap(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP));
	SuccessCallBack callback_obj;	
	MessageExternalInterface::sync_send_take_gl_ownership(
					gl_ownership.get(),
					com,
					boost::bind(&SuccessCallBack::success_handler, &callback_obj, result),
					boost::bind(&SuccessCallBack::failure_handler, &callback_obj)
				);
	if(callback_obj.get_result() == false)
	{
		OSDLOG(ERROR, "Couldn't send ownership msg to new GL");
		return OSDException::OSDSuccessCode::ELECTION_FAILURE;
	}

	boost::shared_ptr<comm_messages::TakeGlOwnershipAck> gl_ownership_ack(new comm_messages::TakeGlOwnershipAck());
	OSDLOG(INFO, "Receiving take gl ownership ack message");
	boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnershipAck> > result_ack = 
					MessageExternalInterface::sync_read_take_gl_ownership_ack(com, this->new_GL_response_timeout);
	if(result_ack->get_return_code() == Communication::TIMEOUT_OCCURED)
	{
		OSDLOG(ERROR, "Timeout occurred while rcving response from new GL");
		return OSDException::OSDSuccessCode::ELECTION_FAILURE;
	}
	else if(result_ack->get_message_object()->get_status()->get_code() == OSDException::OSDSuccessCode::ELECTION_FAILURE) /* 10-> GLcouldnt rename the file, other node became GL*/
	{
		OSDLOG(ERROR, new_gl_id << " could not take GL ownership");
		return OSDException::OSDSuccessCode::ELECTION_FAILURE;
	}
	else if (result_ack->get_message_object()->get_status()->get_code() == OSDException::OSDSuccessCode::ELECTION_SUCCESS or
		result_ack->get_message_object()->get_status()->get_code() == OSDException::OSDSuccessCode::RE_ELECTION_NOT_REQUIRED)
	{
		OSDLOG(INFO, "ELecion completed with value " << result_ack->get_message_object()->get_status()->get_code());
		return OSDException::OSDSuccessCode::ELECTION_SUCCESS;
	}
	return OSDException::OSDSuccessCode::ELECTION_FAILURE;
}

void ElectionManager::proceed_as_coordinator(std::string gl_service_id)
{
	OsmTimer timer;
	int connectivity_count = DEFAULT_CONNECTIVITY_COUNT;
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> node_info_reader(new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));
	std::list<std::string> node_ids = node_info_reader->list_node_ids();
	if(node_ids.size() == 0)
	{
		OSDLOG(ERROR, "No any node exist in cluster");
		throw OSDException::IdNotFoundException();
	}
	OSDLOG(DEBUG, "Total number of nodes in cluster: " << node_ids.size());
	node_ids.sort();
	OSDLOG(INFO, "Co-ordinating election process with node id: " << this->id);
	recordStructure::ElectionRecord record(this->id, connectivity_count);
	int64_t record_size = record.get_serialized_size();
	char buffer[record_size];
	record.serialize_record(buffer, record_size);
	int fileSize = node_ids.size() * ELECTION_RECORD_SIZE;
	this->election_handler->create_election_file(fileSize);

/* re check gfs availability starts */
	if(OSDException::OSDHfsStatus::NOT_WRITABILITY != this->hfs_checker->get_gfs_stat())
	{
		OSDLOG(INFO, "Old GL is up again, returning election");
		this->close_election();
		return;
	}
/* re check gfs availability ends */

	OsmUtils osm_utils;
	int my_offset = osm_utils.find_index(node_ids, this->id) * ELECTION_RECORD_SIZE;
	if(my_offset < 0)
	{
		OSDLOG(ERROR, "Id: " << this->id << " not found in node info");
		throw OSDException::IdNotFoundException();
	}
	this->election_handler->write_on_offset(buffer, record_size, my_offset);
	OSDLOG(DEBUG, "Wait for some time to let all other participate");
	timer.blockingWait(t_el_timeout);
	OSDLOG(DEBUG, "Wait time over, getting new global leader");
	string newGL = this->election_handler->getGL(ELECTION_RECORD_SIZE, fileSize);

	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3(
						new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));

	boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
								rhandler3->get_record_from_id(newGL);
	std::string ip = node_info_file_rcrd->get_node_ip();
	int32_t port = node_info_file_rcrd->get_localLeader_port();

//	GLUtils util_obj;
//	std::string gl_info = util_obj.read_GL_file();
//	std::string gl_service_id = gl_info.substr(0, gl_info.find("\t"));

/* re check gfs availability starts */
	if(OSDException::OSDHfsStatus::NOT_WRITABILITY != this->hfs_checker->get_gfs_stat())
	{
		OSDLOG(INFO, "Old GL is up again, returning election");
		this->close_election();
		return;
	}
/* re check gfs availability ends */
	if(this->gl_ownership_notification(gl_service_id, newGL, ip, port) != OSDException::OSDSuccessCode::ELECTION_FAILURE)
	{
		OSDLOG(DEBUG, "Elected node taking ownership, wait for " << t_notify_timeout << " seconds");
		timer.blockingWait(t_notify_timeout);
	
		if(this->is_gl_updated())
		{
			this->close_election();
			OSDLOG(INFO, "Welcome to new minister");
		}
	}
	else
	{
		this->close_election();
		OSDLOG(ERROR, "New leader could not govern. Restarting election");
		//this->start_election(this->gl_version, this->election_sub_version);
//		this->start_election(gl_service_id);
	}
}

void ElectionManager::proceed_as_participent(std::string gl_service_id)
{
        OsmTimer timer;
	int connectivity_count = DEFAULT_CONNECTIVITY_COUNT;
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> node_info_reader(new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));
	std::list<std::string> node_ids = node_info_reader->list_node_ids();
	node_ids.sort();
	OsmUtils osm_utils;
	OSDLOG(INFO, "Participating in election with node id: " << this->id);
	recordStructure::ElectionRecord record(this->id, connectivity_count);
	int64_t record_size = record.get_serialized_size();
	char buffer[record_size];
	record.serialize_record(buffer, record_size);

	int my_offset = osm_utils.find_index(node_ids, this->id) * ELECTION_RECORD_SIZE;
	if(my_offset < 0)
	{
		OSDLOG(ERROR, "Id: " << this->id << " not found in node info");
		throw OSDException::IdNotFoundException();
	}
	if(!this->election_handler->election_file_exist())
	{
		if(this->election_handler->post_election_file_exist())
		{
			OSDLOG(WARNING, "Post election file exist, it seems election already finished. Exit simply");
			return;
		}
		timer.blockingWait(te_file_wait_timeout);

	}
	try
	{
	        this->election_handler->write_on_offset(buffer, record_size, my_offset);
	}
	catch(...)
	{
		OSDLOG(ERROR, "Error occurred while writing in ELECT_FILE.");
	}
	OSDLOG(DEBUG, "Waiting for election participant timeout: " << t_p_el_timeout << " seconds");
        timer.blockingWait(t_p_el_timeout);
	if(this->is_gl_updated())
	{
		OSDLOG(DEBUG, "Leader updated. Participant exiting election");
	}
	else
	{
		OSDLOG(INFO, "Leader not elected, start new election");
//		this->election_sub_version = this->election_sub_version+1;
//		this->start_election(gl_service_id);
	}

}


bool ElectionManager::is_gl_updated()
{
	OsmUtils osm_utils;
	OSDLOG(DEBUG, "Check, is GL updated?");
	std::list<std::string> file_list = osm_utils.list_files(this->gfs_path.c_str(), "osd_global_leader_information");
	OSDLOG(DEBUG, "Current GL Info file is " << file_list.front());
	std::string v = osm_utils.split(file_list.front(), "osd_global_leader_information_");
	if(atoi(v.c_str()) > atoi(this->gl_version.c_str()))
	{
		OSDLOG(INFO, "Global leader updated, old: " << this->gl_version << ", new: " << v);
		return true;
	}
	else
	{
		OSDLOG(DEBUG, "Global leader not updated");
		return false;
	}
}

int ElectionManager::close_election()
{
	OsmUtils osm_utils;
	std::string election_version = this->gl_version + "-" + osm_utils.ItoStr(this->election_sub_version);
	int retVal = 0;
	if(0 != this->lock_file->release_lock(election_version))
	{
		OSDLOG(ERROR, "Lock File Remove Error");
		retVal = -1;
	}
	if(0 != this->election_handler->sweep_election_file())
	{
		OSDLOG(ERROR, "Election File Remove Error: " << errno);
		retVal = -1;
	}
	return retVal;
}

