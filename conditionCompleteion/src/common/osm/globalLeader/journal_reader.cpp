#include "osm/globalLeader/journal_reader.h"
#include "osm/osmServer/utils.h"

namespace gl_journal_read_handler
{

GlobalLeaderRecovery::GlobalLeaderRecovery(
	std::string path,
	boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	std::vector<std::string> failed_node_list
)
{
	this->recovery_obj.reset(new GLJournalReadHandler(path, map_mgr_ptr, parser_ptr, failed_node_list));
}

bool GlobalLeaderRecovery::perform_recovery()
{
        this->recovery_obj->init();
	OSDLOG(INFO, "Init done");
	//this->recovery_obj->recover_snapshot();
//	this->recovery_obj->recover_last_checkpoint();
//	this->recovery_obj->set_recovering_offset();
	              
	this->recovery_obj->recover_till_eof();
	this->recovery_obj->clean_recovery_processes();
	return true;
}

void GlobalLeaderRecovery::clean_journal_files()
{
	this->recovery_obj->clean_journal_files();
}

uint64_t GlobalLeaderRecovery::get_current_file_id() const                
{                            
	return this->recovery_obj->get_current_file_id();
}

uint64_t GlobalLeaderRecovery::get_next_file_id() const
{       
	return this->recovery_obj->get_next_file_id();
}

GLJournalReadHandler::GLJournalReadHandler(
	std::string path,
	boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	std::vector<std::string> nodes_failed
):
	journal_manager::JournalReadHandler(path, parser_ptr),
	map_mgr_ptr(map_mgr_ptr),
	failed_node_list(nodes_failed)
{
	//this->deserializor.reset(new serializor_manager::Deserializor());
	OSDLOG(INFO, "GLJournalReadHandler constructed");
}

/* Called after recovering from journal, cleans all old recovery process */
void GLJournalReadHandler::clean_recovery_processes()
{
	std::map<std::string, std::string>::iterator it = this->rec_process_map.begin();
	for ( ; it != this->rec_process_map.end(); ++it )
	{
		std::string rec_process_name = "RECOVERY_" + it->first;
		std::string cmd = "sudo -u hydragui hydra_ssh " + it->first + " sudo pkill -9 -f " + rec_process_name;

		CommandExecutor ex_obj;
		int ret = ex_obj.execute_command(cmd);
		if(ret != 0)
		{
			OSDLOG(INFO, "No Recovery Process with name " << rec_process_name << " found.");
		}
		else
		{
			OSDLOG(INFO, "Recovery process " << rec_process_name << " killed.");
		}
	}
}

void GLJournalReadHandler::unmount_journal_fs(std::string failed_node_id, std::string dest_node_id)
{
	GLUtils util_obj;
	cool::SharedPtr<config_file::OSMConfigFileParser> osm_parser_ptr = util_obj.get_config();
	OSDLOG(INFO, "Unmounting Container Service Journals of " << failed_node_id << " from " << dest_node_id);

	std::string journal_name = "." + failed_node_id + "_" + "container_journal";
	std::string umount_cmd = osm_parser_ptr->UMOUNT_HYDRAFSValue();
	std::string umount_full_cmd = "sudo -u hydragui hydra_ssh " + dest_node_id + " " + umount_cmd + " -d " + journal_name;

	CommandExecutor exe;
	OSDLOG(INFO, "Unmounting container journal fs: "<< umount_full_cmd);
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> timeout_parser = util_obj.get_osm_timeout_config();
	int fs_mount_timeout = timeout_parser->fs_mount_timeoutValue();
	boost::system_time timeout = boost::get_system_time()+ boost::posix_time::seconds(fs_mount_timeout);
	int status = exe.execute_cmd_in_thread(umount_full_cmd, timeout);
	if(status != 0)
	{
		if(status != 12)
		{
			OSDLOG(ERROR, "Unable to unmount " << journal_name << " from " << dest_node_id);
		}
	}
	journal_name = "." + failed_node_id + "_" + "transaction_journal";
	umount_full_cmd = "sudo -u hydragui hydra_ssh " + dest_node_id + " " + umount_cmd + " -d " + journal_name;
	OSDLOG(INFO, "Unmounting transaction journal fs: "<< umount_full_cmd);
	status = exe.execute_cmd_in_thread(umount_full_cmd, timeout);
	if(status != 0)
	{
		if(status != 12)
		{
			OSDLOG(ERROR, "Unable to unmount " << journal_name << " from " << dest_node_id);
		}
	}
}

GLJournalReadHandler::~GLJournalReadHandler()
{
}

//UNCOVERED_CODE_BEGIN:
bool GLJournalReadHandler::process_last_checkpoint(int64_t last_checkpoint_offset)
{
/*	this->file_handler->seek(last_checkpoint_offset);
	if (last_checkpoint_offset <= 0)
	{
		OSDLOG(DEBUG,  "Checkpoint not found in Journal file");
		return false;
	}
	OSDLOG(DEBUG,  "Last checkpoint found at offset: " << last_checkpoint_offset);
	this->prepare_read_record();

	recordStructure::Record * result = this->get_next_record().first;
	if (result == NULL)
	{
		PASSERT(false, "Corrupted Checkpoint Header. Aborting.");
	}
	recordStructure::GLCheckpointHeader * checkpoint_obj = dynamic_cast<recordStructure::GLCheckpointHeader *>(result);

	this->state_change_ids = checkpoint_obj->get_id_list();

//	OSDLOG(INFO, "Last commit offset in checkpoint header is " << this->last_commit_offset);
//	DASSERT(checkpoint_obj, "MUST be checkpoint record at offset " << last_checkpoint_offset);
	delete result;
*/
	return true;
}

bool GLJournalReadHandler::process_snapshot()
{
	return true;
}

bool GLJournalReadHandler::process_recovering_offset()
{
	return true;
}
//UNCOVERED_CODE_END

bool GLJournalReadHandler::recover_objects(recordStructure::Record *record_ptr, std::pair<JournalOffset, int64_t> offset1)
{
	JournalOffset offset = offset1.first;
	int type = record_ptr->get_type();
	OSDLOG(INFO, "Type identified during objectRecovery: " << type << " at offset: " << offset.second);
	switch(type)
	{
		case recordStructure::TRANSIENT_MAP:
		{
			recordStructure::TransientMapRecord *tmap_ptr = dynamic_cast<recordStructure::TransientMapRecord *>(record_ptr);
			int service_type = tmap_ptr->get_service_type();
			boost::shared_ptr<ServiceToRecordTypeMapper> service_to_record_type_map_ptr(new ServiceToRecordTypeMapper);
			ServiceToRecordTypeBuilder type_builder;
			type_builder.register_services(service_to_record_type_map_ptr);

			std::string service_name = service_to_record_type_map_ptr->get_name_for_type(service_type);
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_status;
			std::vector<recordStructure::ServiceComponentCouple> tmap_from_journal = tmap_ptr->get_service_comp_obj();
			std::vector<recordStructure::ServiceComponentCouple>::iterator it = tmap_from_journal.begin();
			for (; it != tmap_from_journal.end(); ++it)
			{
				comp_status.push_back(std::make_pair((*it).get_component(), (*it).get_component_info_record()));
			}
			this->map_mgr_ptr->update_transient_map_during_recovery(service_name, comp_status);
			this->map_mgr_ptr->set_transient_map_version(tmap_ptr->get_version());
			OSDLOG(INFO, "Transient Map version recovered for service " << service_name << " is " << this->map_mgr_ptr->get_transient_map_version());
		}
		break;
		case recordStructure::GL_MOUNT_INFO_RECORD:
		{
			recordStructure::GLMountInfoRecord *info_ptr = dynamic_cast<recordStructure::GLMountInfoRecord *>(record_ptr);
			std::vector<std::string>::iterator it = this->failed_node_list.begin();
			for(; it != this->failed_node_list.end(); ++it)
			{
				if(*it == info_ptr->get_failed_node_id())
				{
					this->unmount_journal_fs(info_ptr->get_failed_node_id(), info_ptr->get_destination_node_id());
					break;
				}
			}
		}
		break;
/*		case recordStructure::GL_RECOVERY_RECORD:
		{
			recordStructure::RecoveryRecord *rec_record_ptr = dynamic_cast<recordStructure::RecoveryRecord *>(record_ptr);
			if (offset < this->last_checkpoint_offset)
			{
				if(std::find(this->state_change_ids.begin(), this->state_change_ids.end(), rec_record_ptr->get_state_change_node()) != this->state_change_ids.end())
				{
					this->rec_process_map.insert(std::make_pair(
							rec_record_ptr->get_state_change_node(), rec_record_ptr->get_destination_node())
						);
							//add this record in sci map if operation is not BALANCING
				}		
			}
			else
			{
				if (rec_record_ptr->get_status() == system_state::RECOVERY_PLANNED)
				{
					this->rec_process_map.insert(std::make_pair(
							rec_record_ptr->get_state_change_node(), rec_record_ptr->get_destination_node())
						);
				}
				if (rec_record_ptr->get_status() == system_state::RECOVERY_COMPLETED or
						rec_record_ptr->get_status() == system_state::RECOVERY_ABORTED)
				{
					if (this->rec_process_map.find(rec_record_ptr->get_state_change_node()) != this->rec_process_map.end())
					{
						this->rec_process_map.erase(rec_record_ptr->get_state_change_node());
					}
					
				}
						//add or remove from sci map on basis of operation and status
			}
		}
		
		break;
*/
		default:
			break;
	}
	return true;
}
} // namespace gl_journal_read_handler
