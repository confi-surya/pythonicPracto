#include <assert.h>
//#include "Python.h"

#include "containerLibrary/container_library.h"
#include "containerLibrary/containerInfoFile_manager.h"
#include "containerLibrary/journal_reader.h"
#include "containerLibrary/updater_info.h"

using namespace object_storage_log_setup;

namespace container_library
{

LibraryInterface::~LibraryInterface()
{
	OSDLOG(INFO, "ContainerLibraryInterface destructed");
}

LibraryImpl::LibraryImpl(std::string journal_path, int service_node_id):service_node_id(service_node_id)
{
	uint8_t node_id = service_node_id;
	this->container_library_ptr.reset(new ContainerLibraryImpl(journal_path, node_id));
	
	//Performance : Stat Framework initialization
	if ( service_node_id != -1 )
	{
		this->statsManager.reset(StatsManager::getStatsManager());
	        this->statFramework.reset(OsdStatGatherFramework::getStatsFrameWorkReference(ContainerLib));
        	this->statFramework->initializeStatsFramework();
	}
}

LibraryImpl::~LibraryImpl()
{
	//Performance : Stat Thread Stop
	if ( this->service_node_id != -1 )
	        this->statFramework->cleanup();
}
//UNCOVERED_CODE_BEGIN:

int64_t LibraryImpl::event_wait()
{
	return this->container_library_ptr->event_wait();
}

void LibraryImpl::event_wait_stop()
{
	this->container_library_ptr->event_wait_stop();
}
//UNCOVERED_CODE_END
void LibraryImpl::create_container(
	std::string const & temp_path,
	std::string const path,
	recordStructure::ContainerStat & container_stat_data,
	Status &result,
	int64_t event_id
)
{
	OSDLOG(INFO, "LibraryImpl::create_container " << path);
	this->container_library_ptr->create_container(temp_path, path, container_stat_data, result, event_id);
}

void LibraryImpl::delete_container(
	std::string const path,
	Status & result,
	int64_t event_id
)
{
	OSDLOG(DEBUG, "LibraryImpl::delete_container " << path);
	this->container_library_ptr->delete_container(path, result, event_id);
}

/**
 * @brief Function to move object records non-flushable memory to fluhsable memory
 * @param component_names list of components for which data needs to be moved 
 * @param base_version_changed A bool variable ( If true then clean the non-flushable memory) 
 * @ret   This function return a Status objects which containes return status and return message 
 */
Status LibraryImpl::commit_recovered_data(std::list<int32_t> component_names, bool base_version_changed, int32_t event_id)
{
	try
	{
		this->container_library_ptr->commit_recovered_data(component_names, base_version_changed, event_id);
	}
        catch(...)
	{
		return Status(OSDException::OSDErrorCode::COMMIT_CONT_DATA_FAILED);	
	}
	return Status(OSDException::OSDSuccessCode::COMMIT_CONT_DATA_SUCCESS);
}

/**
 * @brief Function to extract the object records from memory
 * @param component_names list of components for which data needs to be extracted
 * @ret A Python dictionary which container the component name as the key and list of records as the value
 */
boost::python::dict LibraryImpl::extract_container_data(std::list<int32_t> component_names)
{
	return this->container_library_ptr->extract_container_data(component_names, true);
}

/**
 * @brief Function to re-store the extracted data( extracted by above function ) 
 *	  in case of component transfer is not successfull
 * @param records A Python list of reocrds which needs to be re-stored
 * @param memory_flag a bool variable which specify that the records should be 
 *		      re-stored in flushable or non-flushable memory
 */
void LibraryImpl::revert_back_cont_data(boost::python::list records, bool memory_flag)
{
	this->container_library_ptr->revert_back_cont_data(records, memory_flag);
}

/**
 * @brief Function to remove the extracted data from memory in case of component 
 * 	  transfer is  successfull
 * @param component_names list of components for which data needs to be removed
 */
void LibraryImpl::remove_transferred_records(std::list<int32_t> component_names)
{
	this->container_library_ptr->remove_transferred_records(component_names);
}

/**     
 * @brief Function to accept the data of other container service 
 *        The above data will be stored in journal and in non-flushable memory
 * @param records A Python list of reocrds which needs to be stored
 * @param event_id An integer variable which used to notify the service after 
 * 	  the completion of operation.
 * @ret   This function return a Status objects which containes return status and return message
 */
Status LibraryImpl::accept_container_data(
		        boost::python::list records, 
                        bool request_from_recovery,
                        std::list<int32_t> component_number_list,
                        int64_t event_id)
{
	OSDLOG(INFO,"Start accepting recovered records ");
	try
        {
		std::list<recordStructure::FinalObjectRecord> object_records;
		for (int i = 0; i < len(records); ++i)
        	{
                	boost::python::extract<recordStructure::TransferObjectRecord>  extracted_record(records[i]);
                	if(!extracted_record.check())
                   		continue;
			recordStructure::TransferObjectRecord t_record = extracted_record;
        	        recordStructure::FinalObjectRecord record = t_record.get_final_object_record();
                	object_records.push_back(record);
        	}
                this->container_library_ptr->accept_container_data(object_records, request_from_recovery, component_number_list, event_id);
        }
        catch(...)
        {
                return Status(OSDException::OSDErrorCode::ACCEPT_CONT_DATA_FAILED);
        }
	OSDLOG(DEBUG,"Accept container data success ");
        return Status(OSDException::OSDSuccessCode::ACCEPT_CONT_DATA_SUCCESS);	
}

/**     
 * @brief Function to flush all the data present in memory to Info files
 * @param event_id An integer variable which used to notify the service after 
 *        the completion of operation.
 * @ret   This function return a Status objects which containes return status and return message
 */

Status LibraryImpl::flush_all_data(int64_t event_id)
{
        try
        {
                this->container_library_ptr->flush_all_data(event_id);
        }
        catch(...)
        {
                return Status(OSDException::OSDErrorCode::FLUSH_ALL_DATA_ERROR);
        }
        return Status(OSDException::OSDSuccessCode::FLUSH_ALL_DATA_SUCCESS);
}


Status LibraryImpl::update_container(
	std::string path,
	recordStructure::ObjectRecord & data,
	int64_t event_id
)
{	try
	{
		this->container_library_ptr->update_container(path, data, event_id);
	}
	catch(...)
	{
		return Status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);	
	}
	return Status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
}

Status LibraryImpl::update_bulk_delete_records(
        std::string path,
        boost::python::list records,
        int64_t event_id
)
{
        try
        {
                std::list<recordStructure::FinalObjectRecord> final_records;
                for (int i = 0; i < len(records); ++i)
                {
                        boost::python::extract<recordStructure::ObjectRecord>  extracted_record(records[i]);
                        if(!extracted_record.check())
                                continue;
                        recordStructure::ObjectRecord obj_record = extracted_record;
                        recordStructure::FinalObjectRecord final_record(path, obj_record);
                        final_records.push_back(final_record);
                }
                this->container_library_ptr->update_bulk_delete_records(path, final_records, event_id);

        }
        catch(...)
        {
                OSDLOG(DEBUG, "update_bulk_delete_records Failed");
                return Status(OSDException::OSDErrorCode::BULK_DELETE_CONTAINER_FAILED);
        }
        return Status(OSDException::OSDSuccessCode::BULK_DELETE_CONTAINER_SUCCESS);
}

void LibraryImpl::list_container(
	std::string const path,
	ListObjectWithStatus & result,
	uint64_t count, 
	std::string marker,
	std::string end_marker,
	std::string prefix,
	std::string delimiter,
	int64_t event_id
)
{
	OSDLOG(DEBUG, "LibraryImpl::list_container " << path);
	this->container_library_ptr->list_container(path, result, count, marker, end_marker, prefix, delimiter, event_id);
}

void LibraryImpl::get_container_stat(
	std::string const path,
	ContainerStatWithStatus & result,
	bool request_from_updater,
	int64_t event_id
)
{
	OSDLOG(DEBUG, "LibraryImpl::get_container_stat " << path);
	this->container_library_ptr->get_container_stat(path, result, request_from_updater, event_id);
}

/*void LibraryImpl::get_container_stat_for_updater(
	std::string const path,
	ContainerStatWithStatus & result
)
{
	OSDLOG(INFO, "LibraryImpl::get_container_stat_for_updater " << path);
//	recordStructure::ContainerStat container_stat;
	try
	{
		ContainerLibraryImpl::get_container_stat_for_updater(path, result);
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}

}*/

void LibraryImpl::set_container_stat(
	std::string const path,
	recordStructure::ContainerStat & container_stat_data,
	Status & result,
	int64_t event_id
)
{
	OSDLOG(INFO, "LibraryImpl::set_container_stat " << path);
	this->container_library_ptr->set_container_stat(path, container_stat_data, result, event_id);
}


boost::python::dict
	 LibraryImpl::extract_cont_journal_data(std::list<int32_t> component_names)
{
	return this->container_library_ptr->extract_cont_journal_data(component_names);
}


bool LibraryImpl::start_recovery(std::list<int32_t> component_names)
{
	return this->container_library_ptr->start_recovery(component_names);
}

ContainerLibrary::~ContainerLibrary()
{
	OSDLOG(DEBUG, "ContainerLibrary destructed");
}

//TODO asrivastava, this can create problem in case of write in parallel
/*void ContainerLibraryImpl::get_container_stat_for_updater(
	std::string const & path,
	ContainerStatWithStatus & result
)
{
	cool::unused(result);
	OSDLOG(INFO, "Getting container stat for " << path << " for account updater");
//	containerInfoFile::ContainerInfoFileManager info_manager_ptr;
//	info_manager_ptr.get_container_stat(path, &result.container_stat,);
	return;
}*/

cool::SharedPtr<config_file::ConfigFileParser> const get_config_parser()
{	
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
//UNCOVERED_CODE_BEGIN:	
	if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/cont_lib.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/cont_lib.conf");
//UNCOVERED_CODE_END
	return config_file_builder.buildConfiguration();
}

ContainerLibraryImpl::ContainerLibraryImpl(std::string journal_path, uint8_t node_id):journal_path(journal_path),node_id(node_id)
{
	this->stop_requested = false;
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
	this->cont_mem_size = parser_ptr->memory_entry_countValue();
	this->memory_ptr.reset(new MemoryManager);
	this->journal_ptr.reset(new JournalImpl(journal_path, this->memory_ptr, parser_ptr, node_id));
	this->info_file_ptr.reset(new containerInfoFile::ContainerInfoFileManager(node_id));
	this->updater_ptr.reset(new updater_maintainer::UpdaterFileManager(this->memory_ptr, parser_ptr));
	this->file_writer_ptr.reset(new FileWriter(this->info_file_ptr,
			this->memory_ptr, this->journal_ptr->get_journal_writer(), this->updater_ptr, parser_ptr));
	OSDLOG(INFO, "ContainerLibraryImpl instantiation complete");

	this->work.reset(new boost::asio::io_service::work(this->io_service_obj));
	uint32_t async_thread_count = parser_ptr->async_thread_countValue();
	for (uint32_t i = 1; i <= async_thread_count; i++)
	{
		this->thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &this->io_service_obj));
	}

	//Performance : Register Container Library stats
        CREATE_AND_REGISTER_STAT(this->cont_lib_stats, "cont_lib_stats", ContainerLib);
}

ContainerLibraryImpl::~ContainerLibraryImpl()
{
	OSDLOG(DEBUG, "Destructing ContainerLibraryImpl");
	this->work.reset();
	this->thread_pool.join_all();
	this->io_service_obj.reset();
	this->journal_ptr.reset();
	this->file_writer_ptr.reset();
	this->updater_ptr.reset();
	OSDLOG(INFO, "Destructed ContainerLibraryImpl");

	//Performance : De-register container library stats
        UNREGISTER_STAT(this->cont_lib_stats);
}

/**
 * @brief Creating a unique id with the combination of node_id and file_id
 * @param node_id An integer variable which represents node_id (HN0101_<local_leader_port>)
 * @param file_id An integer variable which represents file_id (journal file id)
 * @ret A unique id ( integer )
 */

uint64_t ContainerLibraryImpl::get_unique_file_id(uint64_t node_id, uint64_t file_id)
{
        uint64_t unique_id;
        uint64_t tmp = MAX_ID_LIMIT & file_id;
        node_id = node_id << 56;
        unique_id = tmp | node_id;
        return unique_id;
}

void ContainerLibraryImpl::finish_recovery(container_read_handler::ContainerRecovery & rec_obj)
{
	uint64_t file_id = rec_obj.get_next_file_id();
	file_id = this->get_unique_file_id(this->node_id,file_id);
	this->journal_ptr->get_journal_writer()->set_active_file_id(file_id);
	this->file_writer_ptr->initialize_worker_threads();
	
	OSDLOG(INFO,  "Recovery completed of Container Library");
}

/**
 * @brief Function to extract the object records from journal files
 * @param component_names list of components for which data needs to be extracted
 * @ret A Python dictionary which container the component name as the key and list of records as the value
 */
boost::python::dict
         ContainerLibraryImpl::extract_cont_journal_data(std::list<int32_t>& component_names)
{
        OSDLOG(INFO, "Extract object records from Container Journal");
	boost::python::dict object_record_map;
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
        container_read_handler::ContainerRecovery rec_obj(this->journal_path, this->memory_ptr, parser_ptr);
	try
        {
                rec_obj.perform_recovery(component_names);
	}
	catch(OSDException::FileNotFoundException& e)
        {
		OSDLOG(INFO, "Journal file not found");
                return object_record_map;
        }
        catch(...)
        {
                OSDLOG(WARNING, "Catching unknown exception, Aborting");
                return object_record_map;
        }
	object_record_map = this->extract_container_data(component_names, true);
	return object_record_map;
}


bool ContainerLibraryImpl::start_recovery(std::list<int32_t>& component_names)
{
	OSDLOG(INFO,  "Starting Recovery of Container Library");
	
	//Performance : variable for performance update
	struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
	container_read_handler::ContainerRecovery rec_obj(this->journal_path, this->memory_ptr, parser_ptr);
	try
	{
		rec_obj.perform_recovery(component_names);
		this->journal_ptr->get_journal_writer()->set_active_file_id(rec_obj.get_current_file_id());
//		this->journal_ptr->get_journal_writer()->set_active_file_id(rec_obj.get_next_file_id());
		OSDLOG(INFO,  "Starting flushing containers in recovery");
		this->memory_ptr->flush_containers(boost::bind(&FileWriter::write, this->file_writer_ptr, _1));
		this->file_writer_ptr->stop_worker_threads();		//TODO:GM- Verify if threads are actually joining
		std::map<int32_t, std::pair<std::map<std::string, SecureObjectRecord>, bool> > mem_map = this->memory_ptr->get_memory_map();
		if (mem_map.empty())
			rec_obj.clean_journal_files();

		this->finish_recovery(rec_obj);
		
		//Performance : Updating Stats for Container Library
		try
                {
                        if(this->cont_lib_stats)
                        {
                                ContainerLibraryStats* contLibStat = dynamic_cast<ContainerLibraryStats*>(this->cont_lib_stats.get());
                                OSDLOG(DEBUG,"Updating Container Recovery performance Stats");
                                gettimeofday(&end_time, NULL);
                                contLibStat->totalTimeForContainerRecovery =
                                                ((end_time.tv_sec * 1000.0) + (end_time.tv_usec / 1000.0))
                                                       - ((start_time.tv_sec * 1000.0) + (start_time.tv_usec / 1000.0));
                                contLibStat->totalNumberOfContainerRecovery++;
                        }
                }
                catch(std::exception &e)
                {
                        OSDLOG(DEBUG,"Failed to update Total time taken by Recovery in Stats :Exception:"<<e.what());
                }
		return true;
	}
	catch(OSDException::FileNotFoundException& e)
	{
		this->finish_recovery(rec_obj);
		return true;
	}
	catch(...)
	{
		OSDLOG(WARNING, "Catching unknown exception, Aborting");
		return false;
	}
}

void ContainerLibraryImpl::finish_event_and_flush_container(int64_t event_id)
{
	boost::mutex::scoped_lock lock(this->mutex);

	this->finish_event(event_id);
		
	//TODO asrivastava, segreggated this part, although it can be kept intact(will have no side effect), needs to be discussed.	
	if (this->memory_ptr->get_record_count() >= this->cont_mem_size)
	{
		OSDLOG(INFO, "Flushing in Info Files at size : " << this->memory_ptr->get_record_count());
		this->memory_ptr->flush_containers(boost::bind(
					&FileWriter::write, this->file_writer_ptr, _1));
	}
}

void ContainerLibraryImpl::finish_event_with_lock(int64_t event_id)
{
	boost::mutex::scoped_lock lock(this->mutex);
	this->finish_event(event_id);
	
}

//This function should not be called directly, should be called with lock
void ContainerLibraryImpl::finish_event(int64_t event_id)
{
	OSDLOG(DEBUG,  "finish_event called for: " << event_id);
	this->waiting_events.push(event_id);
	this->condition.notify_one();
}

int64_t ContainerLibraryImpl::event_wait()
{
	int64_t event_id = -1;
	Py_BEGIN_ALLOW_THREADS
	boost::mutex::scoped_lock lock(this->mutex);
	while (!this->stop_requested && this->waiting_events.empty())
	{
		this->condition.wait(lock);
	}
	if (!this->stop_requested)
	{
		event_id = this->waiting_events.front();
		this->waiting_events.pop();
	}
	Py_END_ALLOW_THREADS
	return event_id;
}

//UNCOVERED_CODE_BEGIN:	
void ContainerLibraryImpl::event_wait_stop()
{
	boost::mutex::scoped_lock lock(this->mutex);
	this->stop_requested = true;
} 
//UNCOVERED_CODE_END
void ContainerLibraryImpl::list_container(
	std::string const & path,
	ListObjectWithStatus & result,
	uint64_t count,
	std::string marker,
	std::string end_marker,
	std::string prefix,
	std::string delimiter,
	int64_t event_id
)
{
	this->io_service_obj.post(boost::bind(&ContainerLibraryImpl::list_container_internal,
			this, path, boost::ref(result), count, marker, end_marker, prefix, delimiter, event_id));
}

void ContainerLibraryImpl::list_container_internal(
	std::string const & path,
	ListObjectWithStatus & result,
	uint64_t count,
	std::string marker,
	std::string end_marker,
	std::string prefix,
	std::string delimiter,
	int64_t event_id
)
{
	OSDLOG(INFO,  "List request for " << path);
	result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	try
	{
		this->info_file_ptr->get_object_list(path, &result.object_record,
				this->memory_ptr->get_list_records(path), count, marker, end_marker, prefix, delimiter);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
		OSDLOG(WARNING, "INFO file not found");
	}
	catch(OSDException::FileException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
		OSDLOG(WARNING, "Catching unknown exception");
	}
	this->finish_event_with_lock(event_id);
}

void ContainerLibraryImpl::get_container_stat(
	std::string const & path,
	ContainerStatWithStatus & result,
	bool request_from_updater,
	int64_t event_id
)
{
	this->io_service_obj.post(boost::bind(&ContainerLibraryImpl::get_container_stat_internal,
			this, path, boost::ref(result), request_from_updater, event_id));
}

void ContainerLibraryImpl::get_container_stat_internal(
	std::string const & path,
	ContainerStatWithStatus & result,
	bool request_from_updater,
	int64_t event_id
)
{
	OSDLOG(DEBUG,  "Container Stat request for " << path);
	try
	{
		while(request_from_updater) //true: poll only in case of account updater
		{
			{
			if(!this->memory_ptr->is_flush_running(path))
			{
				OSDLOG(DEBUG,  "get_container_stat_internal polling done, exiting loop");	
				break;
			}
			}
			poll(NULL, 0, 1);
		}
		bool unknown_status = false;
		uint32_t total_records = 0;
		RecordList * records = NULL;
		if(request_from_updater)
		{
			OSDLOG(DEBUG,  "get_container_stat is requested from account updater and Unknown status flag is true.");
			records = this->memory_ptr->get_flush_records(path, unknown_status, true);
			if(records != NULL)
			{
				total_records = records->size();
			}
		}

		if (unknown_status and total_records != 0)
		{
			this->info_file_ptr->get_container_stat_after_compaction(path,
								&result.container_stat,
								records,
								boost::bind(&MemoryManager::release_flush_records, this->memory_ptr, path),
								boost::bind(&MemoryManager::delete_container, this->memory_ptr, path)
								);
			// reset the memory count
			this->memory_ptr->reset_in_memory_count(path, total_records);
		}
		else
		{
			this->info_file_ptr->get_container_stat(path, &result.container_stat,
						boost::bind(&MemoryManager::get_stat, this->memory_ptr, path));
		}
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		if(request_from_updater)
		{
			this->memory_ptr->release_flush_records(path);
			this->memory_ptr->delete_container(path); // TODO check its requiremment
		}
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(WARNING, "Catching unknown exception");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}

	this->finish_event_with_lock(event_id);
}

void ContainerLibraryImpl::set_container_stat(
	std::string const & path,
	recordStructure::ContainerStat & container_stat_data,
	Status &result,
	int64_t event_id
)
{
	this->io_service_obj.post(boost::bind(&ContainerLibraryImpl::set_container_stat_internal,
			this, path, boost::ref(container_stat_data), boost::ref(result), event_id));
}

void ContainerLibraryImpl::set_container_stat_internal(
	std::string const & path,
	recordStructure::ContainerStat & container_stat_data,
	Status &result,
	int64_t event_id
)
{
	OSDLOG(DEBUG,  "Setting container stat for " << path);
	result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	try
	{
		this->info_file_ptr->set_container_stat(path, container_stat_data);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(OSDException::MetaSizeException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_META_SIZE_REACHED);
	}
	catch(OSDException::MetaCountException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_META_COUNT_REACHED);
	}
	catch(OSDException::ACLMetaSizeException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_ACL_SIZE_REACHED);
	}
	catch(OSDException::SystemMetaSizeException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_SYSTEM_META_SIZE_REACHED);
	}
	catch(...)
	{
		OSDLOG(WARNING, "Catching unknown exception");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	this->finish_event_with_lock(event_id);	
}

void ContainerLibraryImpl::add_entry_in_journal(
	std::string const & path,
	recordStructure::ObjectRecord & data,
	boost::function<void(void)> const & done
)
{
	this->journal_ptr->add_entry_in_queue(path, data, done);
}

void ContainerLibraryImpl::create_container(
	std::string const & temp_path,
	std::string const & path,
	recordStructure::ContainerStat & container_stat_data,
	Status &result,
	int64_t event_id
)
{
	this->io_service_obj.post(boost::bind(&ContainerLibraryImpl::create_container_internal,
		this, temp_path, path, boost::ref(container_stat_data), boost::ref(result), event_id));
}

void ContainerLibraryImpl::create_container_internal(
	std::string const & temp_path,
	std::string const & path,
	recordStructure::ContainerStat & container_stat_data,
	Status &result,
	int64_t event_id
)
{
	//Performance : Varibale to update the stats
	struct timeval start_time, end_time; 
        gettimeofday(&start_time, NULL);

	OSDLOG(DEBUG,  "Creating container " << path);
	result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	JournalOffset offset = this->journal_ptr->get_journal_writer()->get_last_sync_offset();
	OSDLOG(DEBUG, "Journal Last Sync offset while creating container is " << offset);
	
	int32_t component_number = this->memory_ptr->get_component_name(path);
	OSDLOG(DEBUG, "Extracting the Updater Pointer for the component " << component_number);
	try
	{
	this->updater_ptr->write_in_file(component_number, path);
		result.set_return_status(this->info_file_ptr->add_container(temp_path, path, container_stat_data, offset).get_return_status());
		if(result.get_return_status() == OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS)
		{
			this->memory_ptr->delete_container(path);
		}
	
		//Performance : Updating Container count and container creation time
		try
        	{
                	if(this->cont_lib_stats)
	                {
        	                ContainerLibraryStats* contLibStat = dynamic_cast<ContainerLibraryStats*>(this->cont_lib_stats.get());
                	        gettimeofday(&end_time, NULL);
                        	contLibStat->totalTimeForContainerCreation =
                                	((end_time.tv_sec * 1000.0) + (end_time.tv_usec / 1000.0))
	                                - ((start_time.tv_sec * 1000.0) + (start_time.tv_usec / 1000.0));
        	                contLibStat->totalNumberOfContainersCreated++;
                	}
	        }
        	catch(std::exception &e)
	        {
        	        OSDLOG(DEBUG,"Failed to update Total Number of Container Created by Recovery in Stats :Exception:"<<e.what());
	        }

	}
	catch(OSDException::FileException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(OSDException::MetaSizeException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_META_SIZE_REACHED);
	}
	catch(OSDException::MetaCountException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_META_COUNT_REACHED);
	}
	catch(OSDException::ACLMetaSizeException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_ACL_SIZE_REACHED);
	}
	catch(OSDException::SystemMetaSizeException& e)
	{
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_SYSTEM_META_SIZE_REACHED);
	}
	catch(...)
	{
		OSDLOG(WARNING, "Catching unknown exception");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	this->finish_event_with_lock(event_id);
}

void ContainerLibraryImpl::delete_container(
	std::string const & path,
	Status &result,
	int64_t event_id
)
{
	this->io_service_obj.post(boost::bind(&ContainerLibraryImpl::delete_container_internal,
		this, path, boost::ref(result), event_id));
}

void ContainerLibraryImpl::delete_container_internal(
	std::string const & path,
	Status &result,
	int64_t event_id
)
{
	OSDLOG(DEBUG, "Deleting Container " << path);
	result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	int32_t component_number = this->memory_ptr->get_component_name(path);
	try
	{
		this->updater_ptr->write_in_file(component_number, path);
		bool status = this->info_file_ptr->delete_container(path, boost::bind(&MemoryManager::delete_container, this->memory_ptr, path));
		if (!status)
		{
			result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
		}
	}
	catch(...)
	{
		OSDLOG(WARNING, "Catching unknown exception");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	this->finish_event_with_lock(event_id);
}

void ContainerLibraryImpl::commit_recovered_data(std::list<int32_t>& component_names, bool base_version_changed, int32_t event_id)
{
	OSDLOG(DEBUG,"Committing the recovered data in to main memory");
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
	uint32_t record_count = this->memory_ptr->get_recovered_record_count() + this->memory_ptr->get_record_count();
        // if the record size more than the flushing limit, first flush the main memory
        // and then move the recovered records
        if ( record_count > 1000 )
        {
                this->memory_ptr->flush_containers(boost::bind(
                                      &FileWriter::write, this->file_writer_ptr, _1));
        }
	
	bool ret = this->memory_ptr->commit_recovered_data(component_names,base_version_changed);
	this->finish_event(event_id);
	if ( ret ) {
		OSDLOG(INFO,"Start Flushing while merging memory");
		this->memory_ptr->flush_containers(boost::bind(
                                        &FileWriter::write, this->file_writer_ptr, _1));
	}
}

void ContainerLibraryImpl::revert_back_cont_data(boost::python::list records, bool memory_flag)
{
	OSDLOG(INFO, "Reverting back container data with memory flag : " << memory_flag);
        for (int i = 0; i < len(records); ++i)
        {
                boost::python::extract<recordStructure::TransferObjectRecord>  extracted_record(records[i]);
                if(!extracted_record.check())
                   continue;
                recordStructure::TransferObjectRecord t_record = extracted_record;
                recordStructure::FinalObjectRecord record = t_record.get_final_object_record();
                uint64_t file_id = t_record.get_offset_file();
                uint64_t id = t_record.get_offset_id();
                JournalOffset offset = std::make_pair(file_id,id);
                this->memory_ptr->revert_back_cont_data(record, offset, memory_flag);
        }
}

void ContainerLibraryImpl::remove_transferred_records(std::list<int32_t>& component_names)
{
	OSDLOG(INFO, "Removing transferred container data ");
	std::list<int32_t>::iterator it = component_names.begin();
	this->updater_ptr->remove_component_records(component_names);
	this->memory_ptr->remove_transferred_records(component_names);
}

void ContainerLibraryImpl::accept_container_data(
        std::list<recordStructure::FinalObjectRecord>& object_records, bool request_from_recovery, 
        std::list<int32_t> component_number_list, int64_t event_id)
{

	OSDLOG(INFO,"Adding Accept Component Marker in Journal");
	try 
	{
		this->journal_ptr->get_journal_writer()->process_accept_component_journal(component_number_list);
	}
        catch (OSDException::WriteFileException e)
        {
                PASSERT(false, "Unable to write Accept Component Marker in Journal. Aborting!");
	}
	catch(...) 
	{
		OSDLOG(INFO,"Failed to write Accept Component Marker in Journal");
		OSDException::FileNotFoundException e;
		throw e;
	}

	if ( object_records.size() <= 0 )
        {
                OSDLOG(INFO,"No object records in accept container data function");
                this->finish_event(event_id);
                return;
        }
	
	OSDLOG(INFO, "Adding Recovered Object Records in Journal");
	std::list<recordStructure::FinalObjectRecord>::iterator it = object_records.begin();
	std::list<int32_t> component_names;
	for ( ; it != object_records.end(); it++ )
	{
		int32_t comp_name = this->memory_ptr->get_component_name((*it).get_path());
		if( component_names.size() <= 0 )
		{
			component_names.push_back(comp_name);
		}
		else
		{
			if ( std::find(component_names.begin(), component_names.end(), comp_name ) == component_names.end())
			{
				component_names.push_back(comp_name);
			}
		}
	}

	if ( component_names.size() > 0 )
	{
		this->memory_ptr->remove_transferred_records(component_names);
	}
	
	if ( request_from_recovery )	
		this->memory_ptr->set_recovery_flag();

	this->add_recovered_entry_in_journal(object_records, boost::bind(&ContainerLibraryImpl::finish_event_and_flush_container, this, event_id));
}

void ContainerLibraryImpl::flush_all_data(int64_t event_id)
{
	OSDLOG(DEBUG, "Start flushing all the data of memory in Info files");
	this->memory_ptr->flush_containers(boost::bind(
                                        &FileWriter::write, this->file_writer_ptr, _1));
	this->finish_event(event_id);
}


boost::python::dict
	 ContainerLibraryImpl::extract_container_data(std::list<int32_t>& component_names, bool request_from_recovery)
{
	OSDLOG(INFO, "Extract object records from memory");
	boost::python::dict component_object_records;
	std::map<int32_t, std::list<recordStructure::TransferObjectRecord> > temp_map =
			 this->memory_ptr->get_component_object_records(component_names, request_from_recovery);
	for (std::map<int32_t, std::list<recordStructure::TransferObjectRecord> >
			::const_iterator it = temp_map.begin(); it != temp_map.end(); it++ )	
	{
		boost::python::list record_list;
		for( std::list<recordStructure::TransferObjectRecord>::const_iterator it1 = 
					it->second.begin(); it1 != it->second.end(); it1++)
		{
			record_list.append(*it1);
		}
		component_object_records[it->first] = record_list;
	}
	return component_object_records;
}

void ContainerLibraryImpl::add_recovered_entry_in_journal(
	std::list<recordStructure::FinalObjectRecord>& object_records,
	boost::function<void(void)> const & done
)
{
	this->journal_ptr->add_recovered_entry_in_queue(object_records,done);
}
//
void ContainerLibraryImpl::update_bulk_delete_records(
        std::string path,
        std::list<recordStructure::FinalObjectRecord>& object_records ,
        int64_t event_id
)
{
        OSDLOG(DEBUG, "Adding Buld Delete Object Records in Journal");
        this->add_bulk_delete_records_in_journal(object_records, boost::bind(&ContainerLibraryImpl::finish_event_and_flush_container, this, event_id));
}

void ContainerLibraryImpl::add_bulk_delete_records_in_journal(
        std::list<recordStructure::FinalObjectRecord>& object_records,
        boost::function<void(void)> const & done
)
{
        this->journal_ptr->add_bulk_delete_in_queue(object_records,done);
}

void ContainerLibraryImpl::update_container(
	std::string & path,
	recordStructure::ObjectRecord & data,
	int64_t event_id
)
{
	OSDLOG(DEBUG, "Adding object " << path << "/" << data.get_name() << " in Journal");
	this->add_entry_in_journal(path, data,
			boost::bind(&ContainerLibraryImpl::finish_event_and_flush_container, this, event_id));
	OSDLOG(DEBUG, "Check for update container file for container : " << path); 
	int32_t component_number = this->memory_ptr->get_component_name(path);
	this->updater_ptr->check_for_updater_file(component_number, path);
	
	//Performance : updating total number of object records
	try
        {
                if(this->cont_lib_stats)
                        {
                                ContainerLibraryStats* contLibStat = dynamic_cast<ContainerLibraryStats*>(this->cont_lib_stats.get());
                                contLibStat->totalNumberOfObjectRecords++;
                        }
         }
         catch(std::exception &e)
         {
                OSDLOG(DEBUG,"Failed to update Total time taken by Recovery in Stats :Exception:"<<e.what());
         }
}

} // namespace ContainerLibrary
