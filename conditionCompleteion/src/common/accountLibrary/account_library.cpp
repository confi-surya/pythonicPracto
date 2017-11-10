#include "accountLibrary/account_library.h"
#include "cool/cool.h"
#include "Python.h"

using namespace StatusAndResult;
using namespace object_storage_log_setup;

namespace accountInfoFile
{

cool::SharedPtr<config_file::AccountConfigFileParser> const get_config_parser()
{	
	cool::config::ConfigurationBuilder<config_file::AccountConfigFileParser> config_file_builder;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	
//UNCOVERED_CODE_BEGIN:
	if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/account_lib.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/account_lib.conf");
//UNCOVERED_CODE_END
	return config_file_builder.buildConfiguration();
}

AccountLibraryImpl::AccountLibraryImpl()
{
	cool::SharedPtr<config_file::AccountConfigFileParser> parser_ptr = get_config_parser();
	uint32_t async_thread_count = parser_ptr->async_thread_countValue();
	this->stop_requested = false;
	this->acc_file_manager.reset(new accountInfoFile::AccountInfoFileManager);
	this->work.reset(new boost::asio::io_service::work(this->io_service_obj));
	for (uint32_t i = 1; i <= async_thread_count; i++)
	{
		this->thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &this->io_service_obj));
	}
}

AccountLibraryImpl::AccountLibraryImpl(const AccountLibraryImpl &)
{

}

AccountLibraryImpl::~AccountLibraryImpl()
{
		OSDLOG(INFO, "Destructing AccountLibraryImpl");
		this->work.reset();
		this->thread_pool.join_all();
		this->io_service_obj.reset();
		OSDLOG(INFO, "Destructed AccountLibraryImpl");
}


AccountLibrary::~AccountLibrary()
{

}

void AccountLibraryImpl::add_container(
				int64_t event_id,
				Status & result,
				std::string path,
				boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	OSDLOG(INFO, "LibraryImpl::add_container start, path : "<<path<<" , contianer name : "<<container_record->get_name());
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::add_container_internal,
			this, event_id, boost::ref(result), path, container_record));	
	OSDLOG(DEBUG, "LibraryImpl::add_container exit")
}


void AccountLibraryImpl::add_container_internal(
				int64_t event_id,
				Status & result,
				std::string path, 
				boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	OSDLOG(INFO, "add_container_internal start");
	try
	{
		this->acc_file_manager->add_container(path, container_record);
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "add_container_internal : FileNotFoundException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "add_container_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(ERROR, "add_container_internal : UnknownException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "add_container_internal exit.")
	this->finish_event(event_id);
}


void AccountLibraryImpl::list_container(
				int64_t event_id,
				ListContainerWithStatus & result,
				std::string path,
				uint64_t count,
				std::string marker,
				std::string end_marker,
				std::string prefix,
				std::string delimiter)
{
	OSDLOG(INFO, "LibraryImpl::list_container start, path : "<<path
			<<", count param : "<<count<<", marker param : "<<marker<<", end_marker param : "<<end_marker
			<<", prefix param : "<<prefix<<", delimiter param : "<<delimiter);
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::list_container_internal,
			this, event_id, boost::ref(result), path, count, marker, end_marker, prefix, delimiter));
	OSDLOG(DEBUG, "LibraryImpl::list_container exit");

}

void AccountLibraryImpl::list_container_internal(
				int64_t event_id,
				ListContainerWithStatus & result,
				std::string path, 
				uint64_t count,
				std::string marker,
				std::string end_marker,
				std::string prefix,
				std::string delimiter)
{

	OSDLOG(INFO, "list_container_internal start");	
	std::list<recordStructure::ContainerRecord>container_record ;
	try
	{
		
		result.set_container_record(this->acc_file_manager->list_container(path, count, marker, end_marker, prefix, delimiter));
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "list_container_internal : FileNotFoundException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "list_container_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(ERROR, "list_container_internal : UnknownException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "list_container_internal exit");
	this->finish_event(event_id);
}


void AccountLibraryImpl::update_container(
			int64_t event_id,
			Status & result,
			std::string path,
			std::list<recordStructure::ContainerRecord> container_records)	
{
	OSDLOG(INFO, "LibraryImpl::update_container start, path : "<<path);
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::update_container_internal,
						this, event_id, boost::ref(result), path, container_records));
	OSDLOG(DEBUG, "LibraryImpl::update_container exit");	

}

void AccountLibraryImpl::update_container_internal(
				int64_t event_id,
				Status & result,
				std::string path,
				std::list<recordStructure::ContainerRecord> contianer_records)
{
	OSDLOG(INFO, "update_container_internal start");
	try
	{
		this->acc_file_manager->update_container(path, contianer_records);
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "update_container_internal : FileNotFoundException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "update_container_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(ERROR, "update_container_internal : UnknownException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "update_container_internal exit.");
	this->finish_event(event_id);
}

void AccountLibraryImpl::delete_container(
				int64_t event_id,
				Status & result,
				std::string path,
				boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	OSDLOG(INFO, "LibraryImpl::delete_container start, path : "<<path<<" , container name : "<<container_record->get_name());
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::delete_container_internal,
						   this, event_id, boost::ref(result), path, container_record));
	OSDLOG(DEBUG, "LibraryImpl::delete_container exit");
}


void AccountLibraryImpl::delete_container_internal(
				int64_t event_id,
				Status & result,
				std::string path,
				boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	OSDLOG(INFO, "delete_container_internal start.");
	try
	{
		this->acc_file_manager->delete_container(path, container_record);
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}

	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "delete_container_internal : FileNotFoundException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "delete_container_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(ERROR, "delete_container_internal : UnknownException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}

	OSDLOG(INFO, "delete_container_internal exit.");
	this->finish_event(event_id);
}


void AccountLibraryImpl::create_account(
				int64_t event_id,
				Status & result,
				std::string const & temp_path,
				std::string const & path,
				boost::shared_ptr<recordStructure::AccountStat>  account_stat)
{
	OSDLOG(INFO, "LibraryImpl::create_account start, path : "<<path<<" , account name: "<<account_stat->get_account());
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::create_account_internal,
							   this, event_id, boost::ref(result), temp_path, path, account_stat));
	OSDLOG(DEBUG, "LibraryImpl::create_account exit.");
}


void AccountLibraryImpl::create_account_internal(
				int64_t event_id,
				Status & result,
				std::string const & temp_path,
				std::string const & path,
				boost::shared_ptr<recordStructure::AccountStat> account_stat)
{
	OSDLOG(INFO, "create_account_internal start");
	try
	{
		result.set_return_status(this->acc_file_manager->create_account(temp_path, path, account_stat).get_return_status()); //set Status
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "create_account_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(ERROR, "create_account_internal : UnknownException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "create_account_internal exit");
	this->finish_event(event_id);
}


void AccountLibraryImpl::delete_account(
			int64_t event_id,
			Status & result,
			std::string const & temp_path,
			std::string const & path)
{
	OSDLOG(INFO, "LibraryImpl::delete_account start, path : "<<path);
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::delete_account_internal,
							   this, event_id, boost::ref(result), temp_path, path));
	OSDLOG(DEBUG, "LibraryImpl::delete_account exit");

}

void AccountLibraryImpl::delete_account_internal(
				int64_t event_id,
				Status & result,
				std::string const & temp_path,
				std::string const & path)
{
	OSDLOG(INFO, "delete_account_internal start");
	try
	{
		result.set_return_status(this->acc_file_manager->delete_account(temp_path, path).get_return_status()); // set Status
	}
	catch(...)
	{
		OSDLOG(ERROR, "delete_account_internal : UnknownException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "delete_account_internal exit");
	this->finish_event(event_id);
}

void AccountLibraryImpl::get_account_stat(
				int64_t event_id,
				AccountStatWithStatus & result,
				std::string const & temp_path,
				std::string const & path)
{
	OSDLOG(INFO, "LibraryImpl::get_account_stat start, path : "<<path);
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::get_account_stat_internal, this, event_id, boost::ref(result), temp_path, path));
	OSDLOG(DEBUG, "LibraryImpl::get_account_stat exit.");
}


void AccountLibraryImpl::get_account_stat_internal(
				int64_t event_id,
				AccountStatWithStatus & result,
				std::string const & temp_path,
				std::string const & path)
{
	OSDLOG(INFO, "get_account_stat_internal start");
	recordStructure::AccountStat account_stat;
	try
	{
		result = this->acc_file_manager->get_account_stat(temp_path, path);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "get_account_stat_internal : FileNotFoundException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "get_account_stat_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(...)
	{
		OSDLOG(ERROR, "get_account_stat_internal : UnknownException caught")
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "get_account_stat_internal exit");
	this->finish_event(event_id);
}

void AccountLibraryImpl::set_account_stat(
			int64_t event_id,
			Status & result,
			std::string const & path, 
			boost::shared_ptr<recordStructure::AccountStat>  account_stat)
{
	OSDLOG(INFO, "LibraryImpl::set_account_stat start, path : "<<path);
	this->io_service_obj.post(boost::bind(&AccountLibraryImpl::set_account_stat_internal,
			this, event_id, boost::ref(result), path, account_stat));
	OSDLOG(DEBUG, "LibraryImpl::set_account_stat exit");
}

void AccountLibraryImpl::set_account_stat_internal(
			int64_t event_id,
			Status & result,
			std::string const & path,
			boost::shared_ptr<recordStructure::AccountStat> account_stat)
{
	OSDLOG(INFO, "set_account_stat_internal start.");
	try
	{
		this->acc_file_manager->set_account_stat(path, account_stat);
		result.set_return_status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "set_account_stat_internal : FileNotFoundException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}
	catch(OSDException::FileException& e)
	{
		OSDLOG(ERROR, "set_account_stat_internal : FileException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
	}
	catch(OSDException::MetaSizeException& e)
	{
		OSDLOG(ERROR, "set_account_stat_internal : MetaSizeException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_META_SIZE_REACHED);
	}
	catch(OSDException::MetaCountException& e)
	{
		OSDLOG(ERROR, "set_account_stat_internal : MetaCountException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_META_COUNT_REACHED);
	}
	catch(OSDException::ACLMetaSizeException& e)
	{
		OSDLOG(ERROR, "set_account_stat_internal : ACLMetaSizeException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_ACL_SIZE_REACHED);
	}
	catch(OSDException::SystemMetaSizeException& e)
	{
		OSDLOG(ERROR, "set_account_stat_internal : SystemMetaSizeException caught");
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_MAX_SYSTEM_META_SIZE_REACHED);
	}
	catch(...)
	{
		OSDLOG(ERROR, "set_account_stat_internal : UnknownException caught")
		result.set_return_status(OSDException::OSDErrorCode::INFO_FILE_INTERNAL_ERROR);
	}
	OSDLOG(INFO, "set_account_stat_internal exit");
	this->finish_event(event_id);
}
//UNCOVERED_CODE_BEGIN: 
void AccountLibraryImpl::event_wait_stop()
{
		boost::mutex::scoped_lock lock(this->mutex);
		this->stop_requested = true;
}
//UNCOVERED_CODE_END
int64_t AccountLibraryImpl::event_wait()
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

void AccountLibraryImpl::finish_event(int64_t event_id)
{
	boost::mutex::scoped_lock lock(this->mutex);
	OSDLOG(DEBUG, "finish_event called for: " << event_id);
	this->waiting_events.push(event_id);
	this->condition.notify_one();
}


}
