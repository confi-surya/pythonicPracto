#include <assert.h>

#include "libraryUtils/object_storage_log_setup.h"
#include "containerLibrary/memory_writer.h"

namespace container_library
{


SecureObjectRecord::SecureObjectRecord(recordStructure::ObjectRecord const & record_obj,
		JournalOffset prev_off, JournalOffset sync_off)
	: last_commit_offset(prev_off)
{
	this->stat.bytes_used = 0;
	this->stat.object_count = 0;
	this->add_record(record_obj, sync_off);
	OSDLOG(DEBUG, "Last commit offset in SecureObjectRecord is " << this->last_commit_offset);
}

SecureObjectRecord::SecureObjectRecord(const SecureObjectRecord &obj)
{
        record_list = obj.record_list;
        tmp_record_list = obj.tmp_record_list;
        last_commit_offset = obj.last_commit_offset;
        stat = obj.stat;
} 

void SecureObjectRecord::revert_record(recordStructure::ObjectRecord const & record_obj,
                        JournalOffset offset)
{
	OSDLOG(DEBUG,"Reverting record");
	this->record_list.push_back(make_pair(offset, record_obj));
}

void SecureObjectRecord::add_record(recordStructure::ObjectRecord const & record_obj,
		JournalOffset sync_off)
{
	if(record_obj.get_deleted_flag() == OBJECT_DELETE)
	{
		OSDLOG(DEBUG, "Object delete case while adding in memory : " << record_obj.get_name());
		this->stat.bytes_used -= record_obj.get_size();
		this->stat.object_count--;
	}
	else if (record_obj.get_deleted_flag() == OBJECT_ADD)
	{
		OSDLOG(DEBUG, "Object add case while adding in memory : " << record_obj.get_name());
		this->stat.bytes_used += record_obj.get_size();
		this->stat.object_count++; 
	}
	else if (record_obj.get_deleted_flag() == OBJECT_MODIFY)
	{
		OSDLOG(DEBUG, "Object modify case while adding in memory : " << record_obj.get_name());
		this->stat.bytes_used += (record_obj.get_size() - record_obj.get_old_size());
		// no changes in object count 
	}
	this->record_list.push_back(make_pair(sync_off, record_obj));
}

bool SecureObjectRecord::remove_record(JournalOffset offset)
{
	while (!this->record_list.empty())
	{
		if (this->record_list.begin()->first < offset)
		{
			if ( this->record_list.begin()->second.get_deleted_flag() == OBJECT_ADD )
			{
				this->stat.bytes_used -= this->record_list.begin()->second.get_size();
				this->stat.object_count--;
			}
			if ( this->record_list.begin()->second.get_deleted_flag() == OBJECT_DELETE )
                        {
                                this->stat.bytes_used += this->record_list.begin()->second.get_size();
                                this->stat.object_count++;
                        }
			if ( this->record_list.begin()->second.get_deleted_flag() == OBJECT_MODIFY )
                        {
                                this->stat.bytes_used -= (this->record_list.begin()->second.get_size()
								- this->record_list.begin()->second.get_old_size());
                        }
			this->record_list.pop_front();
			OSDLOG(INFO, "Record popped while End Container Flush");
		}
		else
		{
			return false;
		}
	}
	return true;
}

RecordList * SecureObjectRecord::get_flush_records()
{
	if (!this->tmp_record_list.empty())
	{
		return reinterpret_cast<RecordList *>(NULL);
	}
	else
	{
		// We need to maintain objects until completion of flush container becuase objects 
		// list must contains the objects.
		this->tmp_record_list.swap(this->record_list);
		return &this->tmp_record_list;
	}
}

bool SecureObjectRecord::is_flush_ongoing()
{
	return (!this->tmp_record_list.empty());
}

bool SecureObjectRecord::release_flush_records()
{
	OSDLOG(INFO,"Releasing flushed records from SecureObjectRecord");
	this->last_commit_offset = (--this->tmp_record_list.end())->first;
	std::list<std::pair<JournalOffset, recordStructure::ObjectRecord> >::iterator it = this->tmp_record_list.begin();
	for (; it != this->tmp_record_list.end(); ++it)
	{
		if(it->second.get_deleted_flag() == OBJECT_DELETE)
		{
			OSDLOG(DEBUG,"Releasing Object with DELETE flag" << it->second.get_name());
			this->stat.bytes_used += it->second.get_size();
			this->stat.object_count++;
		}
		else if (it->second.get_deleted_flag() == OBJECT_ADD)
		{
			OSDLOG(DEBUG,"Releasing Object with ADD flag" << it->second.get_name());
			this->stat.bytes_used -= it->second.get_size();
			this->stat.object_count--;
		}
		else if (it->second.get_deleted_flag() == OBJECT_MODIFY)
		{
			OSDLOG(DEBUG,"Releasing Object with MODIFY flag" << it->second.get_name());
			this->stat.bytes_used -= (it->second.get_size() - it->second.get_old_size());
		}

	}
	OSDLOG(DEBUG, "After releasing records from memory, object count stat is " << this->stat.object_count << " and bytes used stat is " << this->stat.bytes_used);
	this->tmp_record_list.clear();
	return this->record_list.empty();
}

JournalOffset SecureObjectRecord::get_last_commit_offset() const
{
	return this->last_commit_offset;
}


/**
 * Removes and returns all object records
 */
RecordList SecureObjectRecord::get_unscheduled_records()
{
	RecordList tmp_record_list;
        if (this->record_list.empty())
        {
                return tmp_record_list;
        }
        else
        {
                // We need to maintain objects until completion of flush container becuase objects 
                // list must contains the objects.
                tmp_record_list.swap(this->record_list);
		return tmp_record_list;
        }
}


boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > SecureObjectRecord::get_list_records()
{
	OSDLOG(DEBUG,"Start listing Object from memory");
	boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records(new std::vector<recordStructure::ObjectRecord>());
	records->reserve(this->tmp_record_list.size() + this->record_list.size());
	RecordList::const_iterator iter = this->tmp_record_list.begin();
	for (; iter != this->tmp_record_list.end(); iter++)
	{
		OSDLOG(DEBUG,"Listing Object from tmp_record_list");
		records->push_back(iter->second);
	}
	iter = this->record_list.begin();
	for (; iter != this->record_list.end(); iter++)
	{
		OSDLOG(DEBUG,"Listing Object from record_list");
		records->push_back(iter->second);
	}
	return records;
}

recordStructure::ContainerStat SecureObjectRecord::get_stat()
{
	OSDLOG(DEBUG, "While HEADing from memory, Object count is " << this->stat.object_count);
	return this->stat;
}

bool SecureObjectRecord::is_object_record_exist(recordStructure::ObjectRecord const & record_obj)
{
	OSDLOG(INFO,"Checking object record in memory");
	RecordList::iterator it = this->record_list.begin();
	for ( ; it != this->record_list.end(); it++ )
	{
		if ( record_obj.get_name() == (*it).second.get_name())
		{
			if ( record_obj.get_deleted_flag() == (*it).second.get_deleted_flag())
			{
				if ( record_obj.get_size() == (*it).second.get_size() )
				{
					OSDLOG(DEBUG, "Object record already present in memory");
					return true;
				}
			}
			
		}
	}
	return false;
}

MemoryManager::MemoryManager():count(0),recovered_count(0),recovery_data_flag(false)
{
}
/**
 * @brief Calculate component Name from container path.
 * @param path A string in the format (hash(account)/hash(account/container))
*/

int32_t MemoryManager::get_component_name(std::string path)
{
  	//Calculate Component Number
        OSDLOG(DEBUG, "Path_length: " << path);
	int32_t component_number = 0 ;
	// Finding the position of "/"
	int index = path.rfind('/');
	if ( index > 0 )
	{
                uint64_t comp_number = 0 ;  
		// get the substring of path after the "/" to get the hash(account/container)
		std::string acc_con_hash = path.substr(index+1);
		// Getting first 8 character of hash(account/container)
		std::string x = acc_con_hash.substr(0,8);
		std::stringstream ss;
		// Converting the above 8 charater to integer
		ss << std::hex << x;
    		ss >> comp_number;
		// Modulo of NUMBER_OF_COMPONENTS to get the component name
		component_number= comp_number % NUMBER_OF_COMPONENTS;
		static char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
		if(osd_config)
			component_number=1;
                OSDLOG(INFO, "MemoryManager::get_component_name: " << component_number);
		return component_number;
	}
	else 
	{
		OSDLOG(INFO, "Invalid Path : " << path);
		return -1;
	}
}


void MemoryManager::reset_mem_size()
{
	this->count = 0;
}

uint32_t MemoryManager::get_container_count()
{
	uint32_t size=0;
        for ( std::map<int32_t, std::pair<
                std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.begin(); 
                        it1 != this->memory_map.end(); it1++ )
        {
		if ( it1->second.second )
                {
			size += it1->second.first.size();
		}
        }
        return size;
}

uint32_t MemoryManager::get_record_count()
{
	return this->count;
}


/**
 * Get all objects of containers for which request raised
 * Used for transfer. Objects are deleted from this class.
 */
std::map<int32_t, std::list<recordStructure::TransferObjectRecord> >
       MemoryManager::get_component_object_records(std::list<int32_t>& component_names, bool request_from_recovery)
{

	OSDLOG(INFO,"Extracting Records from memory");
	this->mutex.lock();
	//boost::mutex::scoped_lock lock(this->mutex);
	std::map<int32_t, std::list<recordStructure::TransferObjectRecord> >
				component_object_map;
	std::map<int32_t, std::pair<
                        std::map<std::string, SecureObjectRecord>, bool> >
                                ::iterator it;
	std::map<std::string, SecureObjectRecord>::iterator it1;
	//Swap data from main memory of Secure Object Record
	for ( std::list<int32_t>::iterator list_it = component_names.begin(); 
					list_it != component_names.end(); list_it++)
	{
		it = this->memory_map.find((*list_it));
		if ( it != this->memory_map.end() )
		{
		   //List of Container Name cont_name
		   std::list<std::string> cont_names;
		   std::list<recordStructure::TransferObjectRecord> record_list;
		   for (it1 = it->second.first.begin(); it1 != it->second.first.end(); ++it1 )
		   {
		     RecordList object_record_list
						= it1->second.get_unscheduled_records(); 
		     if ( object_record_list.size() > 0 )
		     {
			for(RecordList::iterator obj_it = 
				object_record_list.begin(); 
				obj_it != object_record_list.end(); obj_it++)
			{
			      recordStructure::FinalObjectRecord record_obj(it1->first, ((*obj_it).second));
			      if ( request_from_recovery )
			      {
					OSDLOG(DEBUG, "Request from container recovery process.");
					if ( record_obj.get_object_record().get_deleted_flag() == OBJECT_ADD )
                        		{
	                	                record_obj.get_object_record().set_deleted_flag(OBJECT_UNKNOWN_ADD);
        		                }
		                        else
                		        {
                                		if( record_obj.get_object_record().get_deleted_flag() == OBJECT_DELETE )
                                		{
		                                        record_obj.get_object_record().set_deleted_flag(OBJECT_UNKNOWN_DELETE);
                		                }
                        		}	
			      }
			      recordStructure::TransferObjectRecord transfer_record(record_obj, (*obj_it).first.first, (*obj_it).first.second);
			      record_list.push_back(transfer_record);
			}
			if ( it->second.second )
				this->count -= object_record_list.size();
			else
				this->recovered_count -= object_record_list.size();
		      }
			
			 if ( it1->second.is_flush_ongoing() ) {
				cont_names.push_back(it1->first);
			}
		    }
		    while(cont_names.size() > 0) 
		    {
                                OSDLOG(INFO, "Waiting for flushing completion for component : " << it->first); 
				this->mutex.unlock();
                                usleep(1000);
				this->mutex.lock();
				it = this->memory_map.find(*list_it);
				if ( it != this->memory_map.end() )
				{
					std::list<std::string>::iterator tmp_it = cont_names.begin();
					while(tmp_it != cont_names.end())
					{
						if ( it->second.first.find(*tmp_it) == it->second.first.end() )
						{
							std::string cont = (*tmp_it);
							tmp_it++;
							cont_names.remove(cont);
						}
						else
						{
							tmp_it++;
						}
					}
				}
				else
				{
					break;
				}
		    }			

 		   component_object_map.insert(std::make_pair((*list_it),record_list));
	     }
		    
	}
	this->mutex.unlock();   
	return component_object_map;
}

//
std::map<int32_t, std::pair<std::map<std::string, SecureObjectRecord>, bool> >& MemoryManager::get_memory_map()
{
	return this->memory_map;
}

/**
 * Returns list of names of all container
 */
const std::list<std::string> MemoryManager::get_cont_list_in_memory(int32_t component_number)
{
	std::list<std::string> cont_list;
        std::map<int32_t, std::pair<
                        std::map<std::string, SecureObjectRecord>, bool> >
                                ::iterator it = this->memory_map.find(component_number);
	if ( it != this->memory_map.end() )
        {
                for (std::map<std::string, SecureObjectRecord>
                                ::iterator it1 = it->second.first.begin(); it1 != it->second.first.end(); it1++)
                {
                        cont_list.push_back(it1->first);
                }

        }
        return cont_list;
}

/**
 * Check path in _unknown_flag list
 */
bool MemoryManager::is_unknown_flag_contains(std::string const & path)
{
	boost::mutex::scoped_lock lock(this->status_set_mutex);
	if(this->unknown_flag_set.find(path) != this->unknown_flag_set.end())
	{
		return true;
	}
	return false;
}

void MemoryManager::add_unknown_flag_key(std::string path, recordStructure::ObjectRecord const & obj)
{
	if (obj.get_deleted_flag() == OBJECT_UNKNOWN_ADD || obj.get_deleted_flag() == OBJECT_UNKNOWN_DELETE) // 255 254
	{
		boost::mutex::scoped_lock lock(this->status_set_mutex);
		this->unknown_flag_set.insert(path);
		OSDLOG(INFO, "Adding UNKNOWN status entry in memory for " << path << "/" << obj.get_name());
	}

}

void MemoryManager::remove_unknown_flag_key(std::string path)
{
	boost::mutex::scoped_lock lock(this->status_set_mutex);
	std::set<std::string>::iterator it = this->unknown_flag_set.find(path);
	if (it != this->unknown_flag_set.end())
	{
		OSDLOG(INFO, "Removing UNKNOWN status entry from memory for " << path);
		this->unknown_flag_set.erase(it);	
	}	
}

void MemoryManager::revert_back_cont_data(recordStructure::FinalObjectRecord & final_obj_record, JournalOffset offset, bool memory_flag)
{
	OSDLOG(INFO, "Reverting back container data in memory");
	boost::mutex::scoped_lock lock(this->mutex);
	std::string path = final_obj_record.get_path();
	int32_t component_name = this->get_component_name(path);
	recordStructure::ObjectRecord object_record = final_obj_record.get_object_record();
	std::map<int32_t, std::pair<std::map<string,SecureObjectRecord>,bool> >::iterator it1 = this->memory_map.find(component_name);
	if ( it1 != this->memory_map.end() )               
        {
		std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
		if ( it2 != it1->second.first.end() )
                {
			OSDLOG(DEBUG,"Reverting record for container : " << it2->first);
                        it2->second.revert_record(object_record, offset);
                }
		it1->second.second = memory_flag;
		if (it1->second.second){
                        this->count += 1;
                }
                else{
                        this->recovered_count += 1;
                }
	}
}

void MemoryManager::remove_transferred_records(std::list<int32_t>& component_names)
{
	OSDLOG(INFO,"Removing transferred records");
	boost::mutex::scoped_lock lock(this->mutex);
	std::list<int32_t>::iterator it1 = component_names.begin();
	std::map<int32_t, std::pair<std::map<string,SecureObjectRecord>,bool> >::iterator it2;
	for ( ; it1 != component_names.end(); it1++ )
	{
		it2 = this->memory_map.find(*it1);
		if ( it2 != this->memory_map.end() )
		{
			this->memory_map.erase(it2);
			OSDLOG(DEBUG,"Record found in memory while remove transferred. : After Erasing Mem Size is" << this->memory_map.size());
		}
		OSDLOG(DEBUG,"Record not found in memory while remove transferred.");
	}
}


void MemoryManager::add_record_in_map(std::string const & path,
		recordStructure::ObjectRecord const & record_obj,
		JournalOffset prev_off, JournalOffset sync_off, bool memory_flag)
{
	OSDLOG(INFO, "Adding entry in memory for " << path << "/" << record_obj.get_name());
	int32_t component_name = this->get_component_name(path);
        OSDLOG(DEBUG,"MemoryManager::add_record_in_map  path: " << path << "component: " << component_name);
        std::map<int32_t, std::pair<std::map<string,SecureObjectRecord>,bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
        {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        it2->second.add_record(record_obj, sync_off);
                }
                else
                {
                        OSDLOG(DEBUG,"MemoryManager::add_record_in_map  path not found:  " << path );
                        it1->second.first.insert(std::make_pair(path,
                                SecureObjectRecord(record_obj, prev_off, sync_off)));
                }
         }
         else
         {
                OSDLOG(DEBUG,"MemoryManager::add_record_in_map:  Component not found:  " << component_name );
		std::pair<std::map<std::string,SecureObjectRecord>,bool> newpair;
		std::map<std::string,SecureObjectRecord> newcomp;
		newcomp.insert(std::make_pair(path,SecureObjectRecord(record_obj, prev_off, sync_off)));
		newpair = std::make_pair(newcomp,memory_flag);
                this->memory_map.insert(std::make_pair(component_name,newpair));
         }
	
	if ( memory_flag )
	{
		this->count += 1;
	}
	else
	{
		this->recovered_count += 1;
	}

	// add flag status if any UNKNOWN entry found in object record
	this->add_unknown_flag_key(path, record_obj);
}


bool MemoryManager::is_flush_running(std::string const & path)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t component_name = this->get_component_name(path);
        std::map<int32_t,std::pair< 
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
         {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        if(it2->second.is_flush_ongoing())
                        {       
                                return true;
                        }
                }
         }
         return false;

}

/**
 * Delete container if not being flushed
 * Returns : false if being flushed else true
 */
bool MemoryManager::delete_container(std::string const & path)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t component_name = get_component_name(path);
        std::map<int32_t, std::pair<
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
         {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        if(it2->second.is_flush_ongoing())
                        {       
                                return false;
                        }
                        else
                        {
                                it1->second.first.erase(it2);
				if ( it1->second.first.size() == 0 )
					this->memory_map.erase(it1);
                                return true;
                        }
                }
                else
                {
                        OSDLOG(DEBUG, "Container " << path << " not in memory.");
                        return true;
                }
         }
         else
         {
                OSDLOG(DEBUG, "Container " << path << " not in memory.");
                return true;
         }

}

void MemoryManager::remove_entry(std::string const & path, JournalOffset offset)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t component_name = this->get_component_name(path);
        std::map<int32_t, std::pair< 
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
         {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        if(it2->second.remove_record(offset))
                        {       
                                it1->second.first.erase(it2);
                        }
                }
         }
}

void MemoryManager::remove_components(std::list<int32_t> component_list)
{
        boost::mutex::scoped_lock lock(this->mutex);

        std::list<int32_t>::iterator it1 = component_list.begin();

        for (; it1 != component_list.end(); it1++)
        {
		OSDLOG(DEBUG,"Component list iteration begins.");
                std::map<int32_t, std::pair<std::map<string,SecureObjectRecord>, bool> >::iterator it2 = this->memory_map.find(*it1);
		if ( it2 != this->memory_map.end() )
		{
			OSDLOG(DEBUG, "Component removed from the memory.");
			this->memory_map.erase(it2);
		}
	}
}

void MemoryManager::mark_unknown_memory(std::list<int32_t>& component_list)
{
	boost::mutex::scoped_lock lock(this->mutex);
	std::map<int32_t, std::pair<
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.begin();

        for(; it1 != this->memory_map.end(); it1++ )
        {
		std::list<int32_t>::iterator list_it 
			= std::find(component_list.begin(),component_list.end(),it1->first);	
		if (list_it == component_list.end())
		{ 			
		    it1->second.second = false;
        	}
	}
	
}

bool MemoryManager::is_object_exist(std::string const & path,
				recordStructure::ObjectRecord const & record_obj
)

{
	boost::mutex::scoped_lock lock(this->mutex);
	OSDLOG(INFO, "Searching entry in memory for " << path << "/" << record_obj.get_name());
        int32_t component_name = this->get_component_name(path);
        OSDLOG(DEBUG,"MemoryManager::Checking entry for path: " << path << "component: " << component_name);
        std::map<int32_t, std::pair<std::map<string,SecureObjectRecord>,bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
        {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() )
                {
			return it2->second.is_object_record_exist(record_obj);
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	
}

void MemoryManager::add_entry(std::string const & path,
		recordStructure::ObjectRecord const & record_obj,
		JournalOffset offset,
		JournalOffset sync_offset,
		bool memory_flag
)
{
	boost::mutex::scoped_lock lock(this->mutex);
	this->add_record_in_map(path, record_obj, offset, sync_offset,memory_flag);
}

void MemoryManager::add_entries(std::list<std::pair<bool,std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > > & records, JournalOffset prev_off, JournalOffset sync_off)
{
	boost::mutex::scoped_lock lock(this->mutex);
	OSDLOG(DEBUG,"Start adding entries in memory ");
	std::list<std::pair<bool,std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > >::iterator it = records.begin();
	for ( ; it != records.end(); it++ )
	{
		std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> >::iterator iter = (*it).second.begin();
		for (; iter != it->second.end(); iter++)
		{
			this->add_record_in_map((*iter)->get_path(), (*iter)->get_object_record(), prev_off, sync_off, (*it).first);
		}
	}
}

RecordList * MemoryManager::get_flush_records(std::string const & path, bool & unknown_status, bool request_from_updater)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t component_name = this->get_component_name(path);
	
        std::map<int32_t, std::pair<
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
         {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        if(this->is_unknown_flag_contains(path))
                        {       
                                unknown_status = true;
                                this->remove_unknown_flag_key(path);
                        }
                        if (request_from_updater and !unknown_status)
                        {
                                return NULL;
                        }
                        return it2->second.get_flush_records();
                }
                else
                {
                        return reinterpret_cast<RecordList *>(NULL);
                }
         }
	 else
	 {
		return reinterpret_cast<RecordList *>(NULL);
	 }
}

void MemoryManager::release_flush_records(std::string const & path)
{
	boost::mutex::scoped_lock lock(this->mutex);
	OSDLOG(DEBUG, "Releasing records from memory after flushing for container " << path);
	int32_t component_name = this->get_component_name(path);
        std::map<int32_t, std::pair<
                        std::map<string,SecureObjectRecord> ,bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
         {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        if(it2->second.release_flush_records())
                        {       
                                it1->second.first.erase(it2);
	                        if ( ! it1->second.first.size() > 0 )
                                {
                                        this->memory_map.erase(it1);
                                }
				else
				{
					OSDLOG(INFO,"Could not erase from memory map");
				}
				
                        }
			else
			{
				OSDLOG(DEBUG, "Not erasing this path " << it2->first);
			}
			
                }
		else
		{
			OSDLOG(DEBUG, "Path not found in mem map");
		}
         }
	else
	{
		OSDLOG(DEBUG, "Component not found in mem map");
	}
}

JournalOffset MemoryManager::get_last_commit_offset()
{
	JournalOffset min_offset = std::make_pair(std::numeric_limits<uint64_t>::max(), 0);
	boost::mutex::scoped_lock lock(this->mutex);
	for (std::map<int32_t, std::pair<
                        std::map<std::string, SecureObjectRecord>, bool> >
                                ::iterator it1 = this->memory_map.begin(); 
                                        it1 != this->memory_map.end(); ++it1)
        {
                for (std::map<std::string, SecureObjectRecord>
                                ::iterator it2 = it1->second.first.begin(); it2 != it1->second.first.end(); it2++)
                {
                        if (it2->second.get_last_commit_offset() < min_offset)
                        {       
                                min_offset = it2->second.get_last_commit_offset();
                        }
                }
                        
        }
        return min_offset;
}

boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > MemoryManager::get_list_records(std::string const & path)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t component_name = this->get_component_name(path);
	OSDLOG(DEBUG," Start List from Memory for component : " << component_name << " path : " << path);
        std::map<int32_t, std::pair<
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() && it1->second.second )
         {
		OSDLOG(DEBUG,"Start List from Memory in Loop");
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
			OSDLOG(DEBUG,"Start LIst from Memory in SecureObjectRecord");
                        return  it2->second.get_list_records();
                }
                else
                {
                        OSDLOG(DEBUG,"MemoryManager::get_list_records: path not found:" << path );
                        return boost::shared_ptr<std::vector<recordStructure::ObjectRecord> >();
                }
         }
	 else
	 {
                OSDLOG(DEBUG,"MemoryManager::get_list_records:  Component not found:" << component_name );
		return boost::shared_ptr<std::vector<recordStructure::ObjectRecord> >();
	 }
}

void MemoryManager::reset_in_memory_count(std::string const & path, uint32_t decrement_count)
{
	cool::unused(path);
	boost::mutex::scoped_lock lock(this->mutex);
	this->count = this->count - decrement_count;
	if(this->count <= 0)
	{
		this->count = 0;
	}
}

void MemoryManager::flush_containers(boost::function<void(std::string const & path)> write_func)
{
	boost::mutex::scoped_lock lock(this->mutex);
	OSDLOG(INFO,"Start Flush Container from memory ");
	for (std::map<int32_t, std::pair<
                        std::map<std::string, SecureObjectRecord>, bool> >
                                ::iterator it1 = this->memory_map.begin(); 
                                        it1 != this->memory_map.end(); ++it1)
        {
	   if(it1->second.second) 
	   {
                for (std::map<std::string, SecureObjectRecord>
                                ::iterator it2 = it1->second.first.begin(); it2 != it1->second.first.end(); it2++)
                {
                        write_func(it2->first);
                }
	   }
                        
        }
        this->reset_mem_size();
}

recordStructure::ContainerStat MemoryManager::get_stat(std::string const & path)
{
	boost::mutex::scoped_lock lock(this->mutex);
	int32_t component_name = this->get_component_name(path);
        OSDLOG(DEBUG,"MemoryManager::get_stat:  path: " << path << "component: " << component_name);
        std::map<int32_t, std::pair<
                        std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.find(component_name);
        if ( it1 != this->memory_map.end() )
         {
                std::map<std::string, SecureObjectRecord>::iterator it2 = it1->second.first.find(path);
                if ( it2 != it1->second.first.end() ) 
                {
                        return  it2->second.get_stat();
                }
                else
                {
                        OSDLOG(DEBUG, "Container " << path << " not in memory.");
                        recordStructure::ContainerStat stat;
                        stat.bytes_used = 0;
                        stat.object_count = 0;
                        return stat;
                }
         }
	 else
	 {
		OSDLOG(DEBUG, "Container " << path << " not in memory. component not in memory : "<< component_name);
                recordStructure::ContainerStat stat;
                stat.bytes_used = 0;
                stat.object_count = 0;
                return stat;
	 }
}


uint32_t MemoryManager::get_recovered_record_count()
{
        return this->recovered_count;
}

void MemoryManager::set_recovery_flag()
{
	this->recovery_data_flag = true;
}

bool MemoryManager::get_recovery_flag()
{
	bool ret = this->recovery_data_flag;
	this->recovery_data_flag = false;
	return ret;
}

bool MemoryManager::commit_recovered_data(std::list<int32_t>& component_list, bool base_version_changed)
{
	boost::mutex::scoped_lock lock(this->mutex);
	bool ret = false;
	OSDLOG(INFO, "Committing recovered data");
	std::map<int32_t, std::pair<
                       std::map<string,SecureObjectRecord>, bool> >::iterator it1 = this->memory_map.begin();
	for ( ; it1 != this->memory_map.end(); it1++ )
	{
	   if ( ! it1->second.second )
	   {
		std::list<int32_t>::iterator comp_it = std::find(component_list.begin(), component_list.end(), it1->first);
		if ( comp_it != component_list.end() )
		{
			OSDLOG(DEBUG,"committing recovered data for component : " << it1->first);
			it1->second.second = true;
			ret = true;
		}
		else
		{
			if ( base_version_changed )
			{
				OSDLOG(DEBUG,"Base verion changed. Removing recovered data for component : " << it1->first);
				this->memory_map.erase(it1);
			}
		}
	    }	
	}
	return ret;
}

} // namespace ContainerLibrary

