#include <boost/filesystem.hpp>
#include <boost/thread.hpp> 
#include <time.h>
#include <boost/asio/time_traits.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> 
#include <boost/system/error_code.hpp>

#include "containerLibrary/updater_info.h"
#include "libraryUtils/object_storage_log_setup.h"


namespace updater_maintainer
{

UpdaterFileManager::UpdaterFileManager(
	boost::shared_ptr<container_library::MemoryManager> memory_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
) : memory_ptr(memory_ptr)
{
	OSDLOG(DEBUG, "Starting Account Updater Journal Manager");
	this->timer.reset(new boost::asio::deadline_timer(this->io_service_obj));
	this->path = parser_ptr->updater_journal_pathValue();
	this->max_updated_cont_list_size = parser_ptr->max_updater_list_sizeValue();
	this->flag = false;
	this->timeout = parser_ptr->updater_timeoutValue();
	if (!boost::filesystem::exists(this->path))
		boost::filesystem::create_directories(this->path);
	this->start_timer();
}

UpdaterFileManager::~UpdaterFileManager()
{
	OSDLOG(DEBUG, "Stopping Account Updater Journal Manager");
	
	this->io_service_obj.stop();
	this->timer->cancel();
	this->monitoring_thread->join();
	delete this->monitoring_thread;
	OSDLOG(DEBUG, "Stopped Account Updater Journal Manager");
}

void UpdaterFileManager::run_timer_thread()
{
	OSDLOG(DEBUG, "Starting timer thread");
	this->io_service_obj.run();
}

void UpdaterFileManager::start_timer()
{
	OSDLOG(DEBUG, "Starting timer");
	this->scheduled_timer();
	OSDLOG(DEBUG, "Scheduled timer done");
//	this->monitoring_thread = boost::thread(&UpdaterFileManager::run_timer_thread, this);
	this->monitoring_thread = new boost::thread(&UpdaterFileManager::run_timer_thread, this);
}


void UpdaterFileManager::write_in_file(int32_t component_number, std::string cont_path)
{
	boost::mutex::scoped_lock lock(this->mtx1);
	OSDLOG(DEBUG, "Writing in Account Updater Journal File: " << cont_path);
	std::map<int32_t,boost::shared_ptr<updater_maintainer::UpdaterPointerManager> >
		::iterator it = this->updater_map.find(component_number); 
	if ( it == this->updater_map.end())
	{
		boost::shared_ptr<updater_maintainer::UpdaterPointerManager> new_updater_mgr;
		new_updater_mgr.reset(new updater_maintainer::UpdaterPointerManager(component_number, cont_path, this->path));
		this->updater_map.insert(std::make_pair(component_number, new_updater_mgr));
	}
	it = this->updater_map.find(component_number);
	if ( it != this->updater_map.end())
	{
		it->second->write_in_file(component_number, cont_path, this->max_updated_cont_list_size, 
										this->path, this->memory_ptr );
	}
	else
	{
		OSDLOG(WARNING,"Container has not been updated in Info file");
	}
}

void UpdaterFileManager::reset_timer(const boost::system::error_code& error)
{
	boost::unique_lock<boost::shared_mutex> lock(this->mtx2);
	if(error == boost::asio::error::operation_aborted)
	{
		OSDLOG(INFO, "Deadline Timer Operation aborted");
		return;
	}

	if ( ! this->updater_map.size() > 0 )
	{
		OSDLOG(DEBUG, "Timer expired");
		this->scheduled_timer();
	}
	else
	{
		this->updater_map_lookup();
		this->scheduled_timer();
	}
}

void UpdaterFileManager::scheduled_timer()
{
	OSDLOG(DEBUG, "Lock acquired by Timer Thread, async timeout of " << UPDATER_THREAD_SLEEP_TIME);
	this->timer->expires_from_now(boost::posix_time::seconds(UPDATER_THREAD_SLEEP_TIME));
	this->timer->async_wait(boost::bind(&UpdaterFileManager::reset_timer, this, _1));
	OSDLOG(DEBUG, "Async timer set");
}
	

void UpdaterFileManager::updater_map_lookup()
{

	boost::mutex::scoped_lock lock(this->mtx1);
	std::map<int32_t,boost::shared_ptr<updater_maintainer::UpdaterPointerManager> >
			::iterator it = this->updater_map.begin();
	for ( ; it != this->updater_map.end(); it++ )
	{
		
		time_t curr_time = time(0);
		int32_t timediff = (int32_t)difftime(curr_time, it->second->get_creation_time());
		if ( timediff >= this->timeout )
		{
			
			OSDLOG(DEBUG, "Time for component : "<< (it->first) <<" has been expired. As timeout was : "<<
					this->timeout << " seconds and timediff is : "<< timediff << " seconds");
			it->second->rotate_updater_file(it->first, this->path, this->memory_ptr);
		}
		
	}
}

void UpdaterFileManager::remove_component_records(std::list<int32_t> component_numbers)
{
	boost::mutex::scoped_lock lock(this->mtx1);
	std::list<int32_t>::iterator it = component_numbers.begin();
	for ( ; it != component_numbers.end(); it++)
	{
		std::map<int32_t,boost::shared_ptr<updater_maintainer::UpdaterPointerManager> >
				                        ::iterator it1 = this->updater_map.find(*it);
		if ( it1 != this->updater_map.end())
		{
			this->updater_map.erase(it1);
		}
	}
}

/*
 * Below function checks for UpdaterPointer object for specific component
 * if UpdaterPointer object for the specific component is not present, it
 * creates it and insert it to the updater table( updter_map )
 */

void UpdaterFileManager::check_for_updater_file(int32_t component_number, std::string cont_path)
{
	boost::mutex::scoped_lock lock(this->mtx1);
	std::map<int32_t,boost::shared_ptr<updater_maintainer
					::UpdaterPointerManager> >
                                                ::iterator it = 
							this->updater_map.find(component_number);
	if ( it != this->updater_map.end())
	{
		return;
	}
	else
	{
		OSDLOG(INFO, "Creating updater pointer in case of update_container for container : "<< cont_path );
		boost::shared_ptr<updater_maintainer::UpdaterPointerManager> new_updater_mgr;
                new_updater_mgr.reset(new updater_maintainer
				::UpdaterPointerManager(component_number, cont_path, this->path));
                this->updater_map.insert(std::make_pair(component_number, new_updater_mgr));
	}
}


std::string UpdaterPointerManager::get_container_name(std::string cont_path)
{
	std::vector<std::string> results;
	if (std::count(cont_path.begin(), cont_path.end(), '/') >= 2)
	{
		boost::algorithm::split(results, cont_path, boost::algorithm::is_any_of("/"));
		return (*(results.end() - 3) + "/" + *(--results.end()));
	}
	return " ";	
}


void UpdaterPointerManager::dump_mem_in_file(int32_t component_number, boost::shared_ptr<container_library::MemoryManager> memory_ptr)
{
	std::list<std::string> cont_list = memory_ptr->get_cont_list_in_memory(component_number);
	std::list<std::string>::iterator it = cont_list.begin();
	if(!cont_list.empty() and (this->file_handler != NULL))
	{
		for ( ; it != cont_list.end(); ++it)
		{
			std::string cont_name = this->get_container_name(*it)+",";
			if( std::find(this->cont_hash_list.begin(), this->cont_hash_list.end(),cont_name) 
				== this->cont_hash_list.end())
			{
				OSDLOG(DEBUG, "Before closing, writing container in account journal file: " << cont_name);
				this->file_handler->write(const_cast<char *>(cont_name.c_str()), cont_name.size());
				this->cont_hash_list.push_back(cont_name);
			}
		}
		this->file_handler->sync();
	}
}


const std::string UpdaterPointerManager::get_current_time()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	return buf;
}

void UpdaterPointerManager::create_new_file(int32_t component_number, std::string path)
{
	try
	{
		const std::string cur_time = this->get_current_time();
		OSDLOG(DEBUG, "Opening new file: " << path + "/" + boost::lexical_cast<std::string>(component_number) + "/" + cur_time);
		this->creation_time = time(0);
		if (!boost::filesystem::exists(path + "/" + boost::lexical_cast<std::string>(component_number)))
	                boost::filesystem::create_directories(path + "/" + boost::lexical_cast<std::string>(component_number));
		this->file_handler.reset(new fileHandler::FileHandler(path + "/" + boost::lexical_cast<std::string>(component_number) + "/" + cur_time, fileHandler::OPEN_FOR_APPEND));
	}
	catch(...)
	{
		OSDLOG(FATAL, "Unable to create Updater File");
		PASSERT(false, "Unable to create Updater File");
	}
}

UpdaterPointerManager::UpdaterPointerManager(
			 int32_t component_number,
			 std::string cont_path,
			 std::string path
			 )  
{
	this->create_new_file(component_number, path);
}

UpdaterPointerManager::~UpdaterPointerManager()
{
}

void UpdaterPointerManager::set_creation_time(const time_t creation_time)
{
	this->creation_time = creation_time;
}

void UpdaterPointerManager::add_cont_hash(std::string cont_hash)
{
	this->cont_hash_list.push_back(cont_hash);
}

void UpdaterPointerManager::set_file_descriptor(boost::shared_ptr<fileHandler::FileHandler> fd)
{
	this->file_handler = fd;
}

const time_t UpdaterPointerManager::get_creation_time()
{
	return this->creation_time;
}

std::list<std::string> UpdaterPointerManager::get_cont_hash_list()
{
	return this->cont_hash_list;
}

boost::shared_ptr<fileHandler::FileHandler> UpdaterPointerManager::get_file_handler()
{
	return this->file_handler;
}

void UpdaterPointerManager::rotate_updater_file(int32_t component_number, std::string path, 
						boost::shared_ptr<container_library::MemoryManager> memory_ptr)
{
	OSDLOG(INFO,"Rotating File for component : " << component_number);
	this->dump_mem_in_file(component_number, memory_ptr);
	this->create_new_file(component_number, path);
	this->cont_hash_list.clear();
}

void UpdaterPointerManager::write_in_file(int32_t component_number, 
					  std::string cont_path, 
					  uint32_t  max_cont_hash_list_size, 
					  std::string path,
					  boost::shared_ptr<container_library::MemoryManager> memory_ptr)
{
	boost::mutex::scoped_lock lock(this->mutex);
	OSDLOG(INFO,"Writting account updater file for container : " << cont_path);
	std::string cont_name = this->get_container_name(cont_path);
	if(cont_name != " ")
        {
                cont_name = cont_name + ",";
                if (this->cont_hash_list.size() >= max_cont_hash_list_size)
                {
                        OSDLOG(DEBUG, "Size of updater memory is " << this->cont_hash_list.size() << ". Creating new file");
                        const boost::system::error_code error;
                        this->rotate_updater_file(component_number, path, memory_ptr);
                }
                if ( (this->file_handler != NULL) && (std::find(this->cont_hash_list.begin(), this->cont_hash_list.end(),cont_name)
			== this->cont_hash_list.end()))
                {
                        OSDLOG(DEBUG, "Size of updater memory is " << this->cont_hash_list.size() << ". No rotation required");
                        this->add_cont_hash(cont_name);
                        this->file_handler->write(const_cast<char *>(cont_name.c_str()), cont_name.size());
                        this->file_handler->sync();
                }
        }
        else
        {
                OSDLOG(WARNING, "Bad container name, number of slash mismatch");
        }
	

}


} // namespace updater_maintainer
