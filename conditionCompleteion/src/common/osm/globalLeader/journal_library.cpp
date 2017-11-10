#include "osm/globalLeader/journal_library.h"
#include "osm/osmServer/utils.h"

namespace global_leader
{



JournalLibrary::JournalLibrary(
	std::string journal_path,
//	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
	boost::function<std::vector<std::string>(void)> get_state_change_ids
):
	component_map_mgr(component_map_mgr)
{
	GLUtils util_obj;
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = util_obj.get_journal_config();
//	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
	this->journal_writer_ptr.reset(new journal_manager::GLJournalWriteHandler(
						journal_path, parser_ptr, component_map_mgr, get_state_change_ids
						)
					);
	boost::shared_ptr<job_manager::SyncJobs> sync_jobs_ptr(new job_manager::SyncJobs(journal_writer_ptr));
	this->queue_ptr.reset(new job_manager::JobQueueMgr<job_manager::Job>(sync_jobs_ptr));
	this->job_executor_ptr.reset(new job_manager::JobExecutor<job_manager::Job>(this->queue_ptr));
	this->job_executor_thread = boost::thread(boost::bind(&job_manager::JobExecutor<job_manager::Job>::execute_for_global_leader, this->job_executor_ptr));
	this->service_to_record_type_map_ptr.reset(new ServiceToRecordTypeMapper());
	ServiceToRecordTypeBuilder type_builder;
	type_builder.register_services(this->service_to_record_type_map_ptr);

}

boost::shared_ptr<journal_manager::GLJournalWriteHandler> JournalLibrary::get_journal_writer()
{       
	return this->journal_writer_ptr;
}

void JournalLibrary::add_entry_in_journal(
	std::vector<boost::shared_ptr<recordStructure::Record> > gl_records,
	boost::function<void(void)> const & done
)
{
	std::list<boost::shared_ptr<job_manager::Job> > job_ptr_list;
	std::vector<boost::shared_ptr<recordStructure::Record> >::iterator it = gl_records.begin();
	for ( ; it != (--gl_records.end()); ++it)
	{
		boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(NULL));
		boost::shared_ptr<job_manager::Job> job_ptr =
				boost::make_shared<job_manager::WriteJob>(
					job_manager::WriteJob(
						//boost::make_shared<recordStructure::Record>(*it),
						*it,
						callback_ptr,
						this->journal_writer_ptr,
						false
					)
				);
		job_ptr_list.push_back(job_ptr);
	}

	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(done));
	boost::shared_ptr<job_manager::Job> job_ptr =
				boost::make_shared<job_manager::WriteJob>(
					job_manager::WriteJob(
						//boost::make_shared<recordStructure::Record>(*it),
						*it,
						callback_ptr,
						this->journal_writer_ptr,
						false
					)
				);
	job_ptr_list.push_back(job_ptr);
	//std::cout << "JournalLibrary::add_entry_in_journal\n";
	this->queue_ptr->push_job(job_ptr_list);
}

/* It is only adding transient map into list of records to be flushed on journal */
void JournalLibrary::prepare_transient_record_for_journal(
	std::string service_id,
	std::vector<boost::shared_ptr<recordStructure::Record> > & node_rec_records
)
{
	GLUtils util_obj;
	std::string service_name = util_obj.get_service_name(service_id);
	int type = this->service_to_record_type_map_ptr->get_type_for_service(service_name);
	boost::shared_ptr<recordStructure::Record> tmap_ptr = this->component_map_mgr->convert_tmap_into_journal_record(service_name, type);
	node_rec_records.push_back(tmap_ptr);
}

} // namespace global_leader
