#include <iostream>
#include <time.h>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>

#include "transactionLibrary/transaction_library.h"
#include "libraryUtils/journal.h"

namespace transaction_library
{

BaseTransactionImpl::BaseTransactionImpl()
{
}

BaseTransactionImpl::~BaseTransactionImpl()
{
	OSDLOG(DEBUG, "Destructing BaseTransactionImpl");
}

cool::SharedPtr<config_file::ConfigFileParser> const get_config_parser()
{
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
	if (!osd_config)
	{
	//UNCOVERED_CODE_BEGIN:
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/trans_lib.conf");
	//UNCOVERED_CODE_END
	}
	else
	{
		std::string conf_path(osd_config);
		conf_path = conf_path + "/trans_lib.conf";
		config_file_builder.readFromFile(conf_path);
	}
	return config_file_builder.buildConfiguration();
}

TransactionLibrary::TransactionLibrary(
	std::string transaction_journal_path,
	int service_node_id,
	int listening_port
): service_node_id(service_node_id)
{
	OSDLOG(INFO,  "TransactionLibrary constructor");
	
	uint8_t node_id = service_node_id;
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
	this->transaction_impl.reset(new TransactionImpl(transaction_journal_path, parser_ptr, node_id, listening_port));

	//Performance : Intialization of framework
	if ( service_node_id != -1 )
	{
	        this->statsManager.reset(StatsManager::getStatsManager());
        	this->statFramework.reset(OsdStatGatherFramework::getStatsFrameWorkReference(TransactionLib));
	        this->statFramework->initializeStatsFramework();
	}
/* Constructor class for Transaction Library
 */
}

TransactionLibrary::~TransactionLibrary()
{
	OSDLOG(DEBUG, "Destructing TransactionLibrary");

	//Performance : Stop Thread
	if ( this->service_node_id != -1 )
	        this->statFramework->cleanup();
}

boost::python::dict TransactionLibrary::get_transaction_map()
{
	std::map<int64_t, recordStructure::ActiveRecord> tmp_map = this->transaction_impl->get_transaction_map();
	boost::python::dict ret_data;
	for(std::map<int64_t, recordStructure::ActiveRecord>::const_iterator it = tmp_map.begin(); it != tmp_map.end(); ++it)
	{
		   ret_data[it->first] = it->second;
	}
	return ret_data;
}
//UNCOVERED_CODE_BEGIN:

int64_t TransactionLibrary::event_wait()
{
	return this->transaction_impl->event_wait();
}

void TransactionLibrary::event_wait_stop()
{
	this->transaction_impl->event_wait_stop();
}
//UNCOVERED_CODE_END

StatusAndResult::Status<int64_t> TransactionLibrary::add_bulk_delete_transaction(boost::python::list active_records, int32_t poll_fd)
{
        std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries;
        for (int i = 0; i < len(active_records); ++i)
        {
               boost::python::extract<recordStructure::ActiveRecord>  extracted_record(active_records[i]);
               if(!extracted_record.check())
                  continue;
               boost::shared_ptr<recordStructure::ActiveRecord> record = boost::make_shared<recordStructure::ActiveRecord>(extracted_record);
               transaction_entries.push_back(record);
        }
        int64_t ret = this->transaction_impl->add_bulk_delete_transaction(transaction_entries, poll_fd);
        if ( ret == 0 )
        {
                return StatusAndResult::Status<int64_t>(ret, "add_bulk_delete_transaction success", true, OSDException::OSDSuccessCode::BULK_DELETE_TRANS_ADDED);
        }
        else if( ret == -1 || ret == -2 )
        {
                return StatusAndResult::Status<int64_t>(ret, "add_bulk_delete_transaction failed", false, OSDException::OSDErrorCode::BULK_DELETE_TRANS_CONFLICT);
        }
        else
        {
                return StatusAndResult::Status<int64_t>(ret, "add_bulk_delete_transaction failed", false, OSDException::OSDErrorCode::BULK_DELETE_TRANS_FAILED);
        }
}

boost::python::dict
         TransactionLibrary::extract_trans_journal_data(std::list<int32_t> component_names)
{
	return this->transaction_impl->extract_trans_journal_data(component_names);
}

boost::python::dict
         TransactionLibrary::extract_transaction_data(std::list<int32_t> component_names)
{
	return this->transaction_impl->extract_transaction_data(component_names);
}


void TransactionLibrary::revert_back_trans_data(boost::python::list records, bool memory_flag)
{
	std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries;
        for (int i = 0; i < len(records); ++i)
        {
                boost::python::extract<recordStructure::ActiveRecord>  extracted_record(records[i]);
                if(!extracted_record.check())
                   continue;
		boost::shared_ptr<recordStructure::ActiveRecord> record = boost::make_shared<recordStructure::ActiveRecord>(extracted_record);
                transaction_entries.push_back(record);
        }
	this->transaction_impl->revert_back_trans_data(transaction_entries,memory_flag);
}

StatusAndResult::Status<bool> TransactionLibrary::upgrade_accept_transaction_data(
                                       boost::python::list records, int32_t poll_fd)
{
	return this->transaction_impl->upgrade_accept_transaction_data(records, poll_fd);
}


StatusAndResult::Status<bool> TransactionImpl::upgrade_accept_transaction_data(
                                       boost::python::list records, int32_t poll_fd)
{
	this->journal_ptr->get_journal_writer()->set_upgrade_active_file_id();
        std::list<recordStructure::ActiveRecord> transaction_entries;
        for (int i = 0; i < len(records); ++i)
        {
                boost::python::extract<recordStructure::ActiveRecord>  extracted_record(records[i]);
                if(!extracted_record.check())
                   continue;
                recordStructure::ActiveRecord record = extracted_record;
                transaction_entries.push_back(record);
        }

        bool ret;
        std::string str;
        try
        {
                ret = this->accept_transaction_data(transaction_entries, poll_fd);
                if ( ret )
                {
                        return StatusAndResult::Status<bool>(ret, "", true, OSDException::OSDSuccessCode::ACCEPT_TRANS_DATA_SUCCESS);
                }
                else
                {
                        str = "Transaction Library failed during accepting the recovered data";
                        return StatusAndResult::Status<bool>(ret, str, false, OSDException::OSDErrorCode::ACCEPT_TRANS_DATA_FAILED);
                }
        }
        catch(...)
        {
                OSDLOG(DEBUG, "accept_transaction_data failed");
                str = "Transaction Library failed during accepting the recovered data";
                ret = false;
                return StatusAndResult::Status<bool>(ret, str, false, OSDException::OSDErrorCode::ACCEPT_TRANS_DATA_FAILED);
        }

        this->journal_ptr->get_journal_writer()->rename_file();
        OSDLOG(INFO, "Temporary file renamed");

}




StatusAndResult::Status<bool> TransactionLibrary::accept_transaction_data(
 			               boost::python::list records, int32_t poll_fd)
{
	std::list<recordStructure::ActiveRecord> transaction_entries;
	for (int i = 0; i < len(records); ++i)
        {
                boost::python::extract<recordStructure::ActiveRecord>  extracted_record(records[i]);
		if(!extracted_record.check())
                   continue;
		recordStructure::ActiveRecord record = extracted_record;
		transaction_entries.push_back(record);
	}

	bool ret;
	std::string str;
	try
	{
		ret = this->transaction_impl->accept_transaction_data(transaction_entries, poll_fd);
		if ( ret )
		{
			return StatusAndResult::Status<bool>(ret, "", true, OSDException::OSDSuccessCode::ACCEPT_TRANS_DATA_SUCCESS);
	    	}
		else
		{
			str = "Transaction Library failed during accepting the recovered data";
			return StatusAndResult::Status<bool>(ret, str, false, OSDException::OSDErrorCode::ACCEPT_TRANS_DATA_FAILED);
		}
	}
	catch(...)
	{
		OSDLOG(DEBUG, "accept_transaction_data failed");
		str = "Transaction Library failed during accepting the recovered data";
		ret = false;
		return StatusAndResult::Status<bool>(ret, str, false, OSDException::OSDErrorCode::ACCEPT_TRANS_DATA_FAILED);
	}


}

//

StatusAndResult::Status<bool> TransactionLibrary::commit_recovered_data(std::list<int32_t> component_list, bool base_version_changed)
{
	try
	{
		this->transaction_impl->commit_recovered_data(component_list, base_version_changed);
	}
	catch(...)
        {
		OSDLOG(DEBUG, "commit_recovered_data failed");
		return StatusAndResult::Status<bool>(false,"commit recovered data failed", false, OSDException::OSDErrorCode::COMMIT_TRANS_DATA_FAILED);
        }
                return StatusAndResult::Status<bool>(true, "", true, OSDException::OSDSuccessCode::COMMIT_TRANS_DATA_SUCCESS);
}
StatusAndResult::Status<int64_t> TransactionLibrary::add_transaction(helper_utils::Request request, int32_t poll_fd)
{
/* Interface method to acquire lock for given object
 * @param request: A class containing the all request specific values
 * @return transactionID: number corrosponding to transaction lock
 */
	std::string str = "";
	int64_t id = -1;
	try{
		helper_utils::Request req(request);
		boost::shared_ptr<helper_utils::Request> req1 = boost::make_shared<helper_utils::Request>(req);
		id = this->transaction_impl->add_transaction(req1, poll_fd);
		if (id == -1 or id == -2)
		{
			str = "Transaction lock denied";
			return StatusAndResult::Status<int64_t>(id, str, false, OSDException::OSDErrorCode::TRANSACTION_LOCK_DENIED);
		}
	}

	//UNCOVERED_CODE_BEGIN:
	catch(OSDException::FileException& e){
		str = "Could not acquire lock due to file exception raise";
		return StatusAndResult::Status<int64_t>(id, str, false, OSDException::OSDErrorCode::TRANSACTION_LOCK_DENIED);
	}
	//UNCOVERED_CODE_END
	catch(...){
		str  = "Could not acquire lock due to generic exception raise";
		return StatusAndResult::Status<int64_t>(id, str, false, OSDException::OSDErrorCode::TRANSACTION_LOCK_DENIED);
	}
	return StatusAndResult::Status<int64_t>(id, str, true, OSDException::OSDSuccessCode::TRANSACTION_LOCK_GRANTED);
}

StatusAndResult::Status<bool> TransactionLibrary::release_lock(int64_t transaction_id, int32_t poll_fd)
{
/* Interface method to release lock for the given transaction ID
 * @param transaction_id: number corrosponding to transaction lock
 * @return boolean: bool value for success and failure
 */
	bool lock_flag = false;
	std::string str = "";
	try{
		OSDLOG(DEBUG,  "TransactionLibrary::release_lock: Releasing lock with Transaction ID " << transaction_id);
		lock_flag = transaction_impl->release_lock(transaction_id, poll_fd);
		if(lock_flag)
		{
			return StatusAndResult::Status<bool>(lock_flag, str, true, OSDException::OSDSuccessCode::TRANSACTION_LOCK_RELEASED);
		}
		else
		{
			str = "Transaction id does not exist";
			return StatusAndResult::Status<bool>(lock_flag, str, false, OSDException::OSDErrorCode::TRANSACTION_RELEASE_FAILED);
		}
	}
	//UNCOVERED_CODE_BEGIN:
	catch(OSDException::FileException& e){
		str = "Could not acquire lock due to file exception raise";
		return StatusAndResult::Status<bool>(lock_flag, str, false, OSDException::OSDErrorCode::TRANSACTION_RELEASE_FAILED);
	}
	//UNCOVERED_CODE_END
	catch(...){
		str = "Could not release lock due to exception raise";
		return StatusAndResult::Status<bool>(lock_flag, str, false, OSDException::OSDErrorCode::TRANSACTION_RELEASE_FAILED);
	}
}

StatusAndResult::Status<bool> TransactionLibrary::release_lock(std::string object_name, int32_t poll_fd)
{
	OSDLOG(DEBUG,  "TransactionLibrary::release_lock: Releasing lock for object " << object_name);
	std::string str = "";
	bool lock_flag = false;
	try
	{
		lock_flag = transaction_impl->release_lock(object_name, poll_fd);
		if(lock_flag)
		{
			return StatusAndResult::Status<bool>(lock_flag, str, true, OSDException::OSDSuccessCode::TRANSACTION_LOCK_RELEASED);
		}
		else
		{
			str = "Object does not exist";
			return StatusAndResult::Status<bool>(lock_flag, str, false, OSDException::OSDErrorCode::TRANSACTION_RELEASE_FAILED);
		}
	}
	//UNCOVERED_CODE_BEGIN:
	catch(OSDException::FileException& e)
	{
		str = "Could not acquire lock due to file exception raise";
		return StatusAndResult::Status<bool>(lock_flag, str, false, OSDException::OSDErrorCode::TRANSACTION_RELEASE_FAILED);
	}
	//UNCOVERED_CODE_END
	catch(...)
	{
		str = "Could not release lock due to exception raise";
		return StatusAndResult::Status<bool>(lock_flag, str, false, OSDException::OSDErrorCode::TRANSACTION_RELEASE_FAILED);
	}
}

/*boost::unordered_map<int, Expired_transaction> TransactionLibrary::get_expired_transactions()
{
	return this->transaction_impl->get_expired_transactions();
}*/

bool TransactionLibrary::start_recovery(std::list<int32_t> component_list)
{
	return this->transaction_impl->start_recovery(component_list);
}

boost::python::list TransactionLibrary::recover_active_record(std::string obj_name , int32_t component)
{
	return this->transaction_impl->recover_active_record(obj_name,component);
}

TransactionImpl::TransactionImpl(
	std::string & transaction_journal_path,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	uint8_t node_id,
	int listening_port
): 
	journal_path(transaction_journal_path),
	transaction_memory_ptr(new transaction_memory::TransactionStoreManager(parser_ptr,node_id, listening_port)),
	journal_ptr(new Journal(transaction_journal_path, transaction_memory_ptr, parser_ptr,node_id)),
	node_id(node_id)
{
/* Constructor of Concrete implementation of Transaction Library
 * Instantiates transaction journal class and transaction Memory
 */
	OSDLOG(INFO, "Constructing TransactionImpl");
	this->stop_requested = false;

	//Performance : Register the transaction stats
        CREATE_AND_REGISTER_STAT(this->trans_lib_stats, "trans_lib_stats", TransactionLib);
}

TransactionImpl::~TransactionImpl()
{
	OSDLOG(DEBUG, "Destructing TransactionImpl");
	this->journal_ptr.reset();

	//Performance : De-Register the stats
        UNREGISTER_STAT(this->trans_lib_stats);
}

std::map<int64_t, recordStructure::ActiveRecord> TransactionImpl::get_transaction_map()
{
	return this->transaction_memory_ptr->get_transaction_map();
}

int64_t TransactionImpl::add_bulk_delete_transaction(std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries, int32_t poll_fd)
{
        OSDLOG(INFO, "Adding Bulk Delete Transactions");
        //std::list<std::string> object_names; //sgoyal:commeting as it not utilized further
        //TODO extract object names from request
        //std::list<std::string> object_names = request->get_object_list();

        if (this->transaction_memory_ptr->check_for_queue_size(transaction_entries.size()))
        {
                OSDLOG(INFO,"Queue does not have enough space to acquire lock for bulk delete");
                this->finish_event(poll_fd);
                return -2;
        }
        std::list<int64_t> bulk_transaction_ids;
        for ( std::list<boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it = transaction_entries.begin() ; it != transaction_entries.end(); it++)
        {
                int64_t id = this->transaction_memory_ptr->acquire_lock((*it));
                if (id == -1 or id == -2)
                {
                        OSDLOG(INFO, "Unable to provide Lock because of conflict.");
                        if ( bulk_transaction_ids.size() > 0 )
                        {
                                OSDLOG(INFO, "Start releasing lock taken for BULK_DELETE");
                                this->transaction_memory_ptr->remove_bulk_delete_entry(bulk_transaction_ids);
                        }
                        this->finish_event(poll_fd);
                        return id;
                }
                int64_t lock_time = time(NULL);
                OSDLOG(DEBUG,  "setting acquire lock time : " << lock_time);
                (*it)->set_time(lock_time);
                (*it)->set_transaction_id(id);
                bulk_transaction_ids.push_back(id);
        }
        try
        {
                this->journal_ptr->add_recovered_data_in_journal(transaction_entries,
                          boost::bind(&TransactionImpl::finish_event, this, poll_fd));
        }
	catch(...)
        {
                return -3;
        }
        return 0;
}

void TransactionImpl::revert_back_trans_data(
		std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries, bool memory_flag)
{
	bool ret = this->transaction_memory_ptr->add_records_in_memory(transaction_entries, memory_flag);
	
}

bool TransactionImpl::accept_transaction_data(
                std::list<recordStructure::ActiveRecord> transaction_entries, int32_t poll_fd)
{
	std::list<recordStructure::ActiveRecord>::iterator it = transaction_entries.begin();
	std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_records;
	for (; it != transaction_entries.end(); it++ )
	{
		boost::shared_ptr<recordStructure::ActiveRecord> record = boost::make_shared<recordStructure::ActiveRecord>(*it);
		transaction_records.push_back(record);
	}
	OSDLOG(DEBUG,  "Adding recovered transaction");
	bool ret = this->transaction_memory_ptr->add_records_in_memory(transaction_records);
	
	if ( ret )
	{
		this->journal_ptr->add_recovered_data_in_journal(transaction_records,
		          boost::bind(&TransactionImpl::finish_event, this, poll_fd));
	}
	else
	{
		OSDLOG(INFO, "Unable to Add recovered transactions");
                this->finish_event(poll_fd);
	}
                return ret;

}


/*bool TransactionImpl::upgrade_accept_transaction_data(
                std::list<recordStructure::ActiveRecord> transaction_entries, int32_t poll_fd)
{
        std::list<recordStructure::ActiveRecord>::iterator it = transaction_entries.begin();
        std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_records;
        for (; it != transaction_entries.end(); it++ )
        {
                boost::shared_ptr<recordStructure::ActiveRecord> record = boost::make_shared<recordStructure::ActiveRecord>(*it);
                transaction_records.push_back(record);
        }
        OSDLOG(DEBUG,  "Adding recovered transaction");
        bool ret = this->transaction_memory_ptr->add_records_in_memory(transaction_records);

        if ( ret )
        {
                this->journal_ptr->add_recovered_data_in_journal(transaction_records,
                          boost::bind(&TransactionImpl::finish_event, this, poll_fd));
        }
        else
        {
                OSDLOG(INFO, "Unable to Add recovered transactions");
                this->finish_event(poll_fd);
        }
        return ret;

}*/





boost::python::dict
         TransactionImpl::extract_trans_journal_data(std::list<int32_t> component_names)
{
	OSDLOG(DEBUG, "Extracts Active transaction records from Journal");
	boost::python::dict transaction_records_map;

	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
        transaction_read_handler::TransactionRecovery recovery_obj(this->journal_path, this->transaction_memory_ptr, parser_ptr);
        try
        {
                recovery_obj.perform_recovery(component_names);
        } 
        catch(OSDException::FileNotFoundException& e)
        {
                return transaction_records_map;
        }
        catch(...)
        {
                OSDLOG(WARNING, "Catching unknown exception");
		return transaction_records_map;
        }
	transaction_records_map = this->extract_transaction_data(component_names);
	return transaction_records_map;
}

boost::python::dict
         TransactionImpl::extract_transaction_data(std::list<int32_t> component_names)
{
     OSDLOG(DEBUG, "Extracts Active transaction records from memory");
     boost::python::dict component_transaction_data;
	std::map<int32_t,std::list<recordStructure::ActiveRecord> > temp_map = 
		this->transaction_memory_ptr->get_component_transaction_map(component_names);
        OSDLOG(DEBUG, "Map size is: " << temp_map.size());
       	for(std::map<int32_t,std::list<recordStructure::ActiveRecord> >::const_iterator it
       				                = temp_map.begin(); it != temp_map.end(); it++)
       	{
               boost::python::list record_list;
	       OSDLOG(INFO, "ID while extracting: " << it->first);

               for ( std::list<recordStructure::ActiveRecord>::const_iterator it1 = it->second.begin();
                                      it1 != it->second.end(); it1++ )
               {
                       record_list.append(*it1);
               }
               component_transaction_data[it->first] = record_list;
        }

        return component_transaction_data;
}

void TransactionImpl::commit_recovered_data(std::list<int32_t> component_list, bool base_version_changed)
{
	OSDLOG(DEBUG, "Commiting the recovered records");
	this->transaction_memory_ptr->commit_recovered_data(component_list, base_version_changed);
}
//
int64_t TransactionImpl::add_transaction(boost::shared_ptr<helper_utils::Request> request, int32_t poll_fd)
{
/* Concrete Implementation of add_transaction
 * @param request: Request class containing all request specific values
 * @returns transaction_id: number corrosponding to transaction lock
 */
	int64_t id;
	OSDLOG(DEBUG,  "Adding transaction");
	boost::shared_ptr<recordStructure::ActiveRecord> record(
			new recordStructure::ActiveRecord(
				request->get_operation_type(),
				request->get_object_name(),
				request->get_request_method(),
				request->get_object_id(),
				int64_t(0)
				)
			);
	id = this->transaction_memory_ptr->acquire_lock(record);
	if (id == -1 or id == -2)
	{
		OSDLOG(INFO, "Unable to provide Lock");
		this->finish_event(poll_fd);
		return id;
	}
	int64_t lock_time = time(NULL);
	OSDLOG(DEBUG,  "setting acquire lock time : " << lock_time);
	record->set_time(lock_time);
	this->journal_ptr->add_entry(record, boost::bind(&TransactionImpl::finish_event, this, poll_fd));
	return id;
}

void TransactionImpl::finish_event(int32_t poll_fd)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t ret = 1;
	OSDLOG(DEBUG,  "Finish_event called for: " << poll_fd);
	::write(poll_fd, &ret, sizeof(ret));
}

void TransactionImpl::dummy_event(int32_t poll_fd)
{
	cool::unused(poll_fd);
	return;
}

//UNCOVERED_CODE_BEGIN:
int64_t TransactionImpl::event_wait()
{
	int32_t poll_fd = -1;
	Py_BEGIN_ALLOW_THREADS
	boost::mutex::scoped_lock lock(this->mutex);
	while (!this->stop_requested && this->waiting_events.empty())
	{
		this->condition.wait(lock);
	}
	if (!this->stop_requested)
	{
		poll_fd = this->waiting_events.front();
		this->waiting_events.pop();
	}
	Py_END_ALLOW_THREADS
	return poll_fd;
}

void TransactionImpl::event_wait_stop()
{
	boost::mutex::scoped_lock lock(this->mutex);
	this->stop_requested = true;
}
//UNCOVERED_CODE_END

bool TransactionImpl::release_lock(int64_t transaction_id, int32_t poll_fd)
{
/* Concrete Implementation of release transaction lock
 * @param transaction_id: number corrosponding to transaction lock
 * @return bool: bool values for success and failure
 */
	OSDLOG(INFO, "Releasing lock: " << transaction_id);
	std::string object_name = this->transaction_memory_ptr->get_name_for_id(transaction_id);
	bool status = this->transaction_memory_ptr->release_lock_by_id(transaction_id);
	if (status == true)
	{
		boost::shared_ptr<recordStructure::CloseRecord> record(new recordStructure::CloseRecord(transaction_id, object_name));
		this->journal_ptr->add_entry(record, boost::bind(&TransactionImpl::finish_event, this, poll_fd));
	}
	else
	{
		this->finish_event(poll_fd);
	}
	return status;
}

bool TransactionImpl::release_lock(std::string object_name, int32_t poll_fd)
{
        OSDLOG(INFO, "Releasing lock: " << object_name);
        element_list id_list;
        if (!this->transaction_memory_ptr->get_id_list_for_name(object_name, id_list))
        {
                this->finish_event(poll_fd);
                return false;
        }
        OSDLOG(DEBUG, "Size of entries for object " << object_name << " in memory is: " << id_list.size());
        element_list::iterator it;
        bool success = true;
        for (it = id_list.begin(); it != (--id_list.end()); ++it)
        {
                element_ptr ptr = *it;
                int64_t transaction_id = ptr->transaction_id;
                this->transaction_memory_ptr->release_lock_by_id(transaction_id);
                boost::shared_ptr<recordStructure::CloseRecord> record(new recordStructure::CloseRecord(transaction_id, object_name));
                this->journal_ptr->add_entry(record, boost::bind(&TransactionImpl::dummy_event, this, 1));
        }
        element_ptr ptr = *it;
        int64_t transaction_id = ptr->transaction_id;
        this->transaction_memory_ptr->release_lock_by_id(transaction_id);
        boost::shared_ptr<recordStructure::CloseRecord> record(new recordStructure::CloseRecord(transaction_id, object_name));
        this->journal_ptr->add_entry(record, boost::bind(&TransactionImpl::finish_event, this, poll_fd));
        return success;         //TODO needs to be changed
}

void TransactionImpl::finish_recovery(transaction_read_handler::TransactionRecovery & recovery_obj)
{
	this->journal_ptr->get_journal_writer()->set_active_file_id(recovery_obj.get_next_file_id());
	this->journal_ptr->get_journal_writer()->process_snapshot();
	recovery_obj.clean_journal_files();
}

bool TransactionImpl::start_recovery(std::list<int32_t> component_list)
{
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
	transaction_read_handler::TransactionRecovery recovery_obj(this->journal_path, this->transaction_memory_ptr, parser_ptr);
	try
	{
		recovery_obj.perform_recovery(component_list);
		this->finish_recovery(recovery_obj);
		return true;
	}
	catch(OSDException::FileNotFoundException& e)
	{
		this->finish_recovery(recovery_obj);
		return true;
	}
	catch(...)
	{
		OSDLOG(WARNING, "Catching unknown exception");
		return false;
	}
}

/*boost::unordered_map<int, ExpiredTransaction> TransactionImpl::get_expired_transactions()
{
	return transaction_memory.get_expired_transactions();
}*/


boost::python::list TransactionImpl::recover_active_record(std::string obj_name , int32_t component)
{
	
	return this->transaction_memory_ptr->recover_active_record(obj_name,component);
}

} // namespace transaction_library
