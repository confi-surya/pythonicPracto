#include <iostream>
#include <assert.h> 
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/bind.hpp"

#include "transactionLibrary/transaction_memory.h"
#include "libraryUtils/object_storage_log_setup.h"
//typedef boost::shared_ptr<helper_utils::transactionIdElement> element_ptr;
//typedef std::list<element_ptr> edlement_list;

namespace transaction_memory
{

TransactionStoreManager::TransactionStoreManager(
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,uint8_t node_id, int listening_port
):
	lock_manager_ptr(new TransactionLockManager(parser_ptr,node_id, listening_port))
{
/* Manager class to manage transaction memory and Locks
*/
	OSDLOG(DEBUG, "TransactionStoreManager constructor called");
}

TransactionStoreManager::~TransactionStoreManager()
{
	OSDLOG(DEBUG, "Destructing TransactionStoreManager");
}

bool TransactionStoreManager::add_records_in_memory
		(std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries, bool memory_flag )
{
	return this->lock_manager_ptr->add_records_in_memory(transaction_entries, memory_flag);
}


//
int64_t TransactionStoreManager::acquire_lock(boost::shared_ptr<recordStructure::ActiveRecord> record)
{
/* Method checks whether transaction memory is full or not
 *  * also it acquires lock if memory is free
 *   */

	if (this->lock_manager_ptr->is_full())
	{
		OSDLOG(INFO, "Lock queue is full");
		return -2;  // lock acquisition request rejected
	}
	return this->lock_manager_ptr->acquire_transaction_lock(record);
}

void TransactionStoreManager::mark_unknown_memory(std::list<int32_t>& component_list)
{
	this->lock_manager_ptr->mark_unknown_memory(component_list);	
}

void TransactionStoreManager::remove_bulk_delete_entry(std::list<int64_t>& bulk_transaction_id)
{
        this->lock_manager_ptr->remove_bulk_delete_entry(bulk_transaction_id);
}

bool TransactionStoreManager::check_for_queue_size(int32_t size)
{
        return this->lock_manager_ptr->check_for_queue_size(size);
}

bool TransactionStoreManager::release_lock_by_id(int64_t transaction_id)
{
	return this->lock_manager_ptr->release_lock_by_id(transaction_id);
}

bool TransactionStoreManager::get_id_list_for_name(std::string object_name, element_list& id_list)
{
	return this->lock_manager_ptr->get_id_list_for_name(object_name, id_list);
}

std::string TransactionStoreManager::get_name_for_id(int64_t transaction_id)
{
	return this->lock_manager_ptr->get_name_for_id(transaction_id);
}

void TransactionStoreManager::set_last_transaction_id(int64_t transaction_id)
{
	this->lock_manager_ptr->set_last_transaction_id(transaction_id);
}

std::map<int32_t,std::pair<std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >, bool> > TransactionStoreManager::get_memory()
{
	return this->lock_manager_ptr->get_memory();
}

std::vector<int64_t> TransactionStoreManager::get_active_id_list()
{
	return this->lock_manager_ptr->get_active_id_list();
}

void TransactionStoreManager::commit_recovered_data(std::list<int32_t>& component_list, bool base_version_changed)
{
	this->lock_manager_ptr->commit_recovered_data(component_list, base_version_changed);
}

std::map<int32_t,std::list<recordStructure::ActiveRecord> >  
        TransactionStoreManager::get_component_transaction_map(std::list<int32_t>& component_names)
{
	return this->lock_manager_ptr->get_component_transaction_map(component_names);
}
std::map<int64_t, recordStructure::ActiveRecord> TransactionStoreManager::get_transaction_map()
{
	return this->lock_manager_ptr->get_transaction_map();
}

std::list<std::string> TransactionStoreManager::get_bulk_delete_obj_list()
{
        return this->lock_manager_ptr->get_bulk_delete_obj_list();
}

boost::python::list TransactionStoreManager::recover_active_record(std::string obj_name , int32_t component)
{
	return this->lock_manager_ptr->recover_active_record(obj_name,component);
}

TransactionLockManager::TransactionLockManager(
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	uint8_t node_id,
	int listening_port
) : 
	id(this->get_unique_id(node_id,0)),
	listening_port(listening_port)
{
	OSDLOG(DEBUG, "TransactionLockManager constructor called");
	this->init();
	this->monitor = boost::thread(boost::bind(&TransactionLockManager::start_listening, this));

	//Performance : Register Transaction Memrory Stats
        CREATE_AND_REGISTER_STAT(this->trans_mem_stats, "trans_mem", TransactionMem);
}

int64_t TransactionLockManager::get_unique_id(uint64_t node_id,uint64_t id)
{
        uint64_t unique_id;
        uint64_t tmp = MAX_ID_LIMIT & id;
        node_id = node_id << 55;
        unique_id = tmp | node_id;
        return unique_id;
}


TransactionLockManager::~TransactionLockManager()
{
	OSDLOG(DEBUG, "Destructing TransactionLockManager");
//	this->sock->close();
//	this->io_service.stop();
//	this->monitor.join();
	boost::thread::id tid;
        if(this->monitor.get_id() != tid)
	{
	this->io_service.stop();
	this->monitor.join();
	}
	//Performance : De-Register Stats
        UNREGISTER_STAT(this->trans_mem_stats);
}

std::map<int32_t,std::list<recordStructure::ActiveRecord> > 
	TransactionLockManager::get_component_transaction_map(std::list<int32_t>& component_names)
{
	boost::mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO, "Extracting transaction data from memory");
	std::map<int32_t,
		 std::list<recordStructure::ActiveRecord> > component_transaction_map;
	std::list<int32_t>::iterator comp_it;
	std::map<int32_t,std::pair<std::map<int64_t,
                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1;
	for ( comp_it = component_names.begin(); comp_it != component_names.end(); 
			comp_it++ )
	{
	    it1 = mem_map.find(*comp_it);
	    if ( it1 != mem_map.end() )
  	    {
		for(std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >
                                ::iterator it3 = it1->second.first.begin(); it3 != it1->second.first.end(); it3++)
                {
			std::map<int32_t,std::list<recordStructure::ActiveRecord> >
                                 ::iterator it2 = component_transaction_map.find(*(comp_it));
	        	if ( it2 == component_transaction_map.end())
        		{
				std::list<recordStructure::ActiveRecord> new_list;
				component_transaction_map.insert(std::make_pair(*(comp_it),new_list));
				it2 = component_transaction_map.find(*(comp_it));
			}
			recordStructure::ActiveRecord record(*(it3->second));
		        it2->second.push_back(record);
			if (this->comp_map.find(record.get_transaction_id()) != this->comp_map.end())
				this->comp_map.erase(record.get_transaction_id());
			if ( this->name_map.find(record.get_object_name()) != this->name_map.end()){
				OSDLOG(INFO, "Removing object from name_map");
				this->name_map.erase(record.get_object_name());
			}
	        }
		mem_map.erase(it1);
	    }
	 }
	 return component_transaction_map;
}

//

std::map<int64_t, recordStructure::ActiveRecord> TransactionLockManager::get_transaction_map()
{
        std::map<int64_t, recordStructure::ActiveRecord> transaction_map;
	std::map<int32_t,std::pair<std::map<int64_t,
                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1;
        for (it1 = this->mem_map.begin(); it1 != this->mem_map.end(); it1++ )
        {
	    if (it1->second.second)
	    {
                std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it2;
                for(it2 = it1->second.first.begin();it2 != it1->second.first.end(); it2++)
                {
 	               recordStructure::ActiveRecord record(*(it2->second));
        	       int64_t id = it2->first;
               	       transaction_map.insert(std::pair<int64_t, recordStructure::ActiveRecord>(id, record));
                }
	    }
        }
        return transaction_map;
}

int32_t TransactionLockManager::get_component_name(std::string object_path)
{
	//Calculate Component Name
	OSDLOG(DEBUG,"Path for object is : " << object_path);
	int32_t component_number = 0;
	uint64_t comp_number = 0;
        int index = object_path.rfind('/');
        std::string acc_con_hash = object_path.substr(0,index);
	index = acc_con_hash.rfind('/');
	acc_con_hash = acc_con_hash.substr(index+1);
	std::string x = acc_con_hash.substr(0,8);
        std::stringstream ss;
        ss << std::hex << x;            
        ss >> comp_number;
        component_number= comp_number % 512;
	OSDLOG(DEBUG,"Component Name of Transaction Library : " << component_number);
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        if (osd_config)
                component_number=1;
        return component_number;
}

bool TransactionLockManager::add_records_in_memory(                               
			 std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries, bool memory_flag)
{
   try
   {
	boost::mutex::scoped_lock lock(this->mtx);
	std::list<boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it;
	for ( it = transaction_entries.begin() ; it != transaction_entries.end(); it++)
	{
		std::string object_name=(*it)->get_object_name();
		//Calculate Component Name
		int32_t component_name=this->get_component_name(object_name);
		OSDLOG(DEBUG,"Adding Recovered Data in Component Name : " << component_name);
		std::map<int32_t,std::pair<std::map<int64_t,
		                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >
							::iterator it1=this->mem_map.find(component_name);
		if ( it1 != mem_map.end())
		{
			int64_t transaction_id = (*it)->get_transaction_id();
			it1->second.first.insert(std::make_pair(transaction_id,(*it)));
		}
		else
		{
			std::pair<std::map<int64_t,boost::shared_ptr<recordStructure::ActiveRecord> >, bool>
						new_pair;
			std::map<int64_t,boost::shared_ptr<recordStructure::ActiveRecord> > new_map;
			new_map.insert(std::make_pair((*it)->get_transaction_id(),(*it)));
			new_pair = std::make_pair(new_map,memory_flag);
			this->mem_map.insert(std::make_pair(component_name,new_pair));
		}		
		if ( memory_flag )
		{
			OSDLOG(INFO,"Memory Flag is true" );
			std::string obj_name = (*it)->get_object_name();
			int64_t trans_id = (*it)->get_transaction_id();
			element_ptr new_node(new helper_utils::transactionIdElement);
                        new_node->transaction_id = (*it)->get_transaction_id();
                        new_node->operation = (*it)->get_operation_type();
                        if ( this->name_map.find(obj_name) != this->name_map.end() )
                        {
				OSDLOG(INFO,"Inserting another lock for object " << obj_name  );
                                this->name_map.find(obj_name)->second.push_back(new_node);
                        }
                        else
                        {
                                element_list list;
                                list.push_back(new_node);
                                this->name_map.insert(std::make_pair(obj_name, list));
                        }
			if ( this->comp_map.find(trans_id) == this->comp_map.end() )
			{
				this->comp_map.insert(std::make_pair(trans_id, component_name));
			}
		}
	}
	return true;
    }
    catch(...)
    {
        return false;
    }
}

// 

void TransactionLockManager::mark_unknown_memory(std::list<int32_t>& component_list)
{
	boost::mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO, "In mark_unknown_memory");
	std::map<int32_t,std::pair<std::map<int64_t,
		boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >
							::iterator it1 = this->mem_map.begin();

        for(; it1 != this->mem_map.end(); it1++ )
        {
                std::list<int32_t>::iterator list_it
                        = std::find(component_list.begin(),component_list.end(),it1->first);
                if (list_it == component_list.end())
                {
			it1->second.second = false;
			std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >
					::iterator it2 = it1->second.first.begin();
			for ( ; it2 != it1->second.first.end() ; ++it2 )
			{
				std::string obj_name = it2->second->get_object_name();
				if (this->name_map.find(obj_name) != this->name_map.end())
					this->name_map.erase(obj_name);
				if (this->comp_map.find(it2->first) != this->comp_map.end())
					this->comp_map.erase(it2->first);
			}
                }
        }

}
//

std::list<std::string> TransactionLockManager::get_bulk_delete_obj_list()
{
	std::list<std::string> obj_list;
	std::map<int32_t,std::pair<std::map<int64_t,
                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >
                                                        ::iterator it1 = this->mem_map.begin();
	
        for ( ; it1 != this->mem_map.end(); ++it1)
	{
        	std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it
				= it1->second.first.begin();
		for ( ; it != it1->second.first.end(); ++it )
		{
                	if(it->second->get_request_method() == "BULK_DELETE")
               		{
                        	obj_list.push_back(it->second->get_object_name());
                	}
        	}
	}
        return obj_list;
}

void TransactionLockManager::remove_bulk_delete_entry(std::list<int64_t>& bulk_transaction_ids)
{
        boost::mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO, "In remove_bulk_delete_records");
	std::map<int64_t,int32_t>::iterator comp_it;
 	std::list<int64_t>::iterator bulk_it = bulk_transaction_ids.begin();
	//Start iterating bulk_transction_id list
        for ( ; bulk_it != bulk_transaction_ids.end(); ++bulk_it )
        {
           comp_it = this->comp_map.find((*bulk_it));
	   if ( comp_it != this->comp_map.end() )
	   {
		//ID found in component map
		std::map<int32_t,std::pair<std::map<int64_t,
                	boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >
                                                        ::iterator it1 = this->mem_map.find(comp_it->second);
		if ( it1 != this->mem_map.end() )
		{
			//component found in Memory map
			std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >
				::iterator it =	it1->second.first.find((*bulk_it));
			
			if ( it != it1->second.first.end() )
			{	
				//ID found in Memory map
                		if ( it->second->get_request_method() == "BULK_DELETE" )
                		{
					//Erase from Name map
                        		this->name_map.erase(it->second->get_object_name());
					//Erase Active Record from Memory map
					it1->second.first.erase(it);
					if ( it1->second.first.size() == 0 )
					{
						//Erase component from Memory map if no active record remaining 
						// for that component
        	                		this->mem_map.erase(comp_it->second);
					}
					//Erasing entry from Component map
					this->comp_map.erase(comp_it);
                		}
        		}
		}
	    }
	}
}

int64_t TransactionLockManager::acquire_transaction_lock(boost::shared_ptr<recordStructure::ActiveRecord> record)
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, element_list>::iterator it;

//	boost::shared_ptr<recordStructure::ActiveRecord> record(new recordStructure::ActiveRecord);
//	::memcpy(record.get(), act_record, sizeof(recordStructure::ActiveRecord));

	std::string object_name = record->get_object_name();
	OSDLOG(INFO, "TransactionLockManager::acquire_transaction_lock for " << object_name);

	if (this->name_map.find(object_name) != this->name_map.end())
	{
		element_list list(this->name_map.find(object_name)->second);
		int lock_operation = list.front()->operation;
		int incoming_operation = record->get_operation_type();

		if (lock_operation == helper_utils::READ && incoming_operation == helper_utils::WRITE)
		{
			OSDLOG(INFO, "Write lock rejected as read locks are existing for " << object_name);
			return -1;
		}
		else if (lock_operation == helper_utils::WRITE && incoming_operation == helper_utils::WRITE)
		{
			OSDLOG(INFO, "Write lock rejected as write lock is already acquired for " << object_name);
			return -1;
		}
		else if (lock_operation == helper_utils::WRITE && incoming_operation == helper_utils::READ)
		{
			OSDLOG(INFO, "Read lock rejected as write lock is already acquired for " << object_name);
			return -1;
		}
	}
	else
	{
		element_list list;
		this->name_map.insert(std::make_pair(object_name, list));
	}

	element_ptr new_node(new helper_utils::transactionIdElement);
	if (record->get_transaction_id())
	{
		new_node->transaction_id = record->get_transaction_id();
		this->set_last_transaction_id(record->get_transaction_id());
	}
	else
	{
		new_node->transaction_id = ++(this->id);
		record->set_transaction_id(new_node->transaction_id);
	}
	new_node->operation = record->get_operation_type();
	this->name_map.find(object_name)->second.push_back(new_node);
	int32_t component_name=this->get_component_name(object_name);
	OSDLOG(DEBUG,"Adding Original Data in Component Name : " << component_name);
	std::map<int32_t,std::pair<std::map<int64_t,
                        boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1 = this->mem_map.find(component_name);
        if (it1 != this->mem_map.end())
        {
                std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it2 
                                        = it1->second.first.find(new_node.get()->transaction_id);
                if (it2 != it1->second.first.end())
                {
			/* Although this condition will never occur
        	         * But if it occurs it is a bug. so logging it in the logger.
	                 * TODO: log this message in logger, as soon as it is prepared. 
				 currently using STDOUT*/
                        OSDLOG(ERROR, "Transaction id " << new_node->transaction_id << "is already in memory map ");
                        return -1;
                }
                it1->second.first.insert(std::make_pair(new_node->transaction_id, record));            
                this->comp_map.insert(std::make_pair(new_node->transaction_id,it1->first));      
        }
        else
        {
		std::map<int64_t,boost::shared_ptr<recordStructure::ActiveRecord> > newcomp;
		newcomp.insert(std::make_pair(new_node->transaction_id, record));
		std::pair<std::map<int64_t,boost::shared_ptr<recordStructure::ActiveRecord> >, bool> new_pair
			= std::make_pair(newcomp,true);
                this->mem_map.insert(std::make_pair(component_name,new_pair));
                this->comp_map.insert(std::make_pair(new_node->transaction_id,component_name));      
        }
        OSDLOG(DEBUG, "Element with ID " << new_node->transaction_id << " added in memory. Mem size is " << this->comp_map.size());
        return new_node->transaction_id;
}

bool TransactionLockManager::get_id_list_for_name(std::string object_name, element_list& id_list)
{	
	std::map<std::string, element_list>::iterator it;
	it = this->name_map.find(object_name);
	OSDLOG(DEBUG, "Getting ID list for object " << object_name);

	if (it != this->name_map.end())
	{
		id_list = it->second;
		return true;
	}
	OSDLOG(INFO, "Lock for object " << object_name << " not found");
	return false;
}

void  TransactionLockManager::internal_release_lock_by_id(int64_t transaction_id)
{
	std::map<int64_t,int32_t>::iterator it = this->comp_map.find(transaction_id);
        if ( it != this->comp_map.end())
        {
                std::map<int32_t,std::pair<std::map<int64_t,
                                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1 = this->mem_map.find(it->second);
                it1->second.first.erase(transaction_id);
		if ( it1->second.first.size() == 0 )
			this->mem_map.erase(it1);
                this->comp_map.erase(transaction_id);
        }
}

std::string TransactionLockManager::get_name_for_id(int64_t transaction_id)
{
	std::map<int64_t,int32_t>::iterator it = this->comp_map.find(transaction_id);
        if (it != this->comp_map.end())
        {
		std::map<int32_t,std::pair<std::map<int64_t,
                                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1 = this->mem_map.find(it->second);
                if (it1 != mem_map.end() )
                {
                        std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it2 = it1->second.first.find(transaction_id);
                        if(it2 != it1->second.first.end())
                        {
                                boost::shared_ptr<recordStructure::ActiveRecord> record(it2->second);
                                return it2->second->get_object_name();
                        }
                        return "error";
                }
                return "error";
        }                       
        return "error";
}

bool TransactionLockManager::release_lock_by_id(int64_t transaction_id)
{
	boost::mutex::scoped_lock lock(this->mtx);
	OSDLOG(DEBUG, "TransactionLockManager::release_lock_by_id for " << transaction_id);
	std::map<int64_t,int32_t>::iterator it = this->comp_map.find(transaction_id);
	if ( it == this->comp_map.end())
	{
		OSDLOG(DEBUG, "ID " << transaction_id << " not found in memory, returning");
		return false;
	}
	std::map<int32_t,std::pair<std::map<int64_t,
        	boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1 = this->mem_map.find(it->second);
	std::map<int64_t,boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it2;
	boost::shared_ptr<recordStructure::ActiveRecord> record;
        if (it1 != mem_map.end() )
        {
                it2 = it1->second.first.find(transaction_id);
                if(it2 == it1->second.first.end())
                {
                        OSDLOG(DEBUG, "ID " << transaction_id << " not found in memory, returning");
                        return false;
                }

        }
        else 
        {
                OSDLOG(DEBUG, "ID " << transaction_id << " not found in memory, returning");
                return false;
        }
        record = it2->second;
	std::string object_name = record->get_object_name();
	std::map<std::string, element_list>::iterator it_name_map;
	it_name_map = this->name_map.find(object_name);
	element_list::iterator list_it;

	if (list_it == it_name_map->second.end())
	{
		OSDLOG(DEBUG, "Transaction id not found in name map");
	}

	for (list_it = it_name_map->second.begin(); list_it != it_name_map->second.end(); ++list_it)
	{
		if ((*list_it)->transaction_id == transaction_id)
		{
			it_name_map->second.erase(list_it++);
			if(it_name_map->second.empty())
			{
				this->name_map.erase(object_name);
			}
			break;
		}
	}
	it1->second.first.erase(transaction_id);
	if ( it1->second.first.size() == 0 )
		this->mem_map.erase(it1->first);
        this->comp_map.erase(transaction_id);
	OSDLOG(INFO, "Lock released from memory for object " << object_name << ". Mem size is " << this->comp_map.size());
	return true;
}


std::map<int32_t,std::pair<std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >, bool> > TransactionLockManager::get_memory()
{
	int size=0;
	std::map<int32_t,std::pair<std::map<int64_t,
                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1 = this->mem_map.begin();
        for (; it1 != mem_map.end(); it1++ )
        {
                size += it1->second.first.size();
        }
        OSDLOG(DEBUG, "Getting size of memory from Lock Manager " << size);

	return this->mem_map;
}

std::vector<int64_t> TransactionLockManager::get_active_id_list()
{
	std::vector<int64_t> tmp_id_list;
	OSDLOG(DEBUG, "Getting " << this->comp_map.size() << " number of active IDs");
	if (this->comp_map.size() <= 1 )
        {
                return tmp_id_list;
        }
        for ( std::map<int64_t,int32_t>::iterator it = this->comp_map.begin();
                                                        it != this->comp_map.end(); ++it)
        {
                OSDLOG(DEBUG, "Pushing ID " << it->first << " in active ID list");
                tmp_id_list.push_back(it->first);
        }
        return tmp_id_list;
}

void TransactionLockManager::set_last_transaction_id(int64_t transaction_id)
{
	if (this->id < transaction_id)
	{
		this->id = transaction_id;
	}
}

bool TransactionLockManager::is_full()
{
	return this->comp_map.size() > 1000000;
}

bool TransactionLockManager::check_for_queue_size(int32_t size)
{
        return (this->mem_map.size() + size) > 1000000;
}

std::string TransactionLockManager::get_bulk_delete_obj_list_in_string()
{
        std::string obj_list_str = "$$$$";
        std::list<std::string> obj_list = this->get_bulk_delete_obj_list();
        std::list<std::string>::iterator it = obj_list.begin();
        for ( ; it != obj_list.end(); ++it )
        {
                obj_list_str += (*it) + ":";
        }
        obj_list_str += "$$$$";
        obj_list_str.insert(0, boost::lexical_cast<std::string>(obj_list_str.length()));
        return obj_list_str;
}

std::string TransactionLockManager::get_transaction_map_in_string()
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::string map_str;
	std::map<int32_t,std::pair<std::map<int64_t,
                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >
						::iterator it1 = this->mem_map.begin();
        for(; it1 != this->mem_map.end(); it1++ )
        {
	    if ( it1->second.second )
	    {
                std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it2;
                for ( it2 = it1->second.first.begin(); it2 != it1->second.first.end(); it2++ )
                {
                        recordStructure::ActiveRecord record(*(it2->second));
			int64_t trans_id = it2->first;
                        map_str += "$$$$" + boost::lexical_cast<std::string>(trans_id) + "," 
								+ record.get_object_name() + ",";
                        map_str += record.get_object_id() + "," + record.get_request_method() + ",";
                        map_str += boost::lexical_cast<std::string>(record.get_time()) + "####";
                }
	     }
        }

	int length = map_str.length();
	if (length == 0){
		map_str += "$$$$";
		length += 4;
	}
	map_str += "&&&&";
	length += 4;
	map_str.insert(0, boost::lexical_cast<std::string>(length));
	return map_str;
}

void TransactionLockManager::accept_handler(const boost::system::error_code &error)
{
        if(!error)
        {
		OSDLOG(DEBUG, "New connection accepted socket: " <<this->sock->remote_endpoint().address().to_string());
		this->sock_read_buffer.reset(new char[2]);
                this->sock->async_read_some(boost::asio::buffer(this->sock_read_buffer.get(), 1),
                boost::bind(&TransactionLockManager::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        }
        else
        {
		OSDLOG(ERROR, "Error in accepting connection error no: " <<error.value() <<" error message: " <<error.message());
                this->acceptor->async_accept(*(this->sock), boost::bind(&TransactionLockManager::accept_handler, this, boost::asio::placeholders::error));
        }
}

void TransactionLockManager::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
        if(!error && this->sock_read_buffer.get())
        {
		OSDLOG(DEBUG, "read successful for socket: " <<this->sock->remote_endpoint().address().to_string() <<" bytes_transferred: " <<bytes_transferred);
		this->sock_read_buffer[1] = '\0';
                int message_type = 0;
                try
                {
                        message_type = boost::lexical_cast<int>(this->sock_read_buffer.get());
                }
                catch( const boost::bad_lexical_cast & )
                {
                        OSDLOG(ERROR, "Unable to cast message : " <<this->sock_read_buffer.get() << "to integer");
                }

		this->sock_read_buffer.reset();

		OSDLOG(DEBUG, "message type: " <<message_type);
                std::string map_str = " ";
                switch(message_type)
                {
                        case 1:
				{
                                map_str = this->get_bulk_delete_obj_list_in_string();
				int size = map_str.length();
                                this->sock_write_buffer.reset(new char[size+1]);
                                memcpy(this->sock_write_buffer.get(), map_str.c_str(), size);
                                this->sock_write_buffer[size] = '\0';
				boost::asio::async_write(*(this->sock), boost::asio::buffer(this->sock_write_buffer.get()), boost::asio::transfer_all(), boost::bind(&TransactionLockManager::handle_write, this, boost::asio::placeholders::error));
				}
                                break;
                        case 2:
				{
                                map_str = this->get_transaction_map_in_string();
				int size = map_str.length();
                                this->sock_write_buffer.reset(new char[size+1]);
                                memcpy(this->sock_write_buffer.get(), map_str.c_str(), size);
                                this->sock_write_buffer[size] = '\0';
				boost::asio::async_write(*(this->sock), boost::asio::buffer(this->sock_write_buffer.get()), boost::asio::transfer_all(), boost::bind(&TransactionLockManager::handle_write, this, boost::asio::placeholders::error));
				}
                                break;
                        default:
                                OSDLOG(ERROR, "Unknown Request found from trigrred recovery ");
                                this->sock->close();
        			this->acceptor->async_accept(*(this->sock), boost::bind(&TransactionLockManager::accept_handler, this, boost::asio::placeholders::error));
                }
		
        }
	else
	{
		OSDLOG(ERROR, "error while reading from socket :" <<this->sock->remote_endpoint().address().to_string() <<" error no: " <<error.value() <<" error message: " <<error.message());
		this->sock_read_buffer.reset();

                this->sock->close();
        	this->acceptor->async_accept(*(this->sock), boost::bind(&TransactionLockManager::accept_handler, this, boost::asio::placeholders::error));
	}
}

void TransactionLockManager::handle_write(const boost::system::error_code &error)
{
	if(error)
	{
		OSDLOG(ERROR, "while sending data : " << error.value() << " .Message: " << error.message());
	}

	OSDLOG(DEBUG, "write on socket: " <<this->sock->remote_endpoint().address().to_string() <<" successuful");
	this->sock_write_buffer.reset();

	this->sock->close();
        this->acceptor->async_accept(*(this->sock), boost::bind(&TransactionLockManager::accept_handler, this, boost::asio::placeholders::error));
}

// OBB-633 fix
// Problem #1	: port already in use exception
//       Cause	: It is because of two recovery processes ran simultaneouly and the process who tries to bind port later, receives "port already in use exception",
//       	  which was not handled, due to which core was generating.
//       Fix	: boost::system::system_error exception is handled
//       To Do	: Need to identify, why two recovery processes are initiating, even though single request for recovery process is submitting.
// Problem #2	: Core dump
// 	Cause	: start_listening thread which is a member function of TransactionLockManager class, was accessing object data members even after his 
// 		  object destruction.
// 	Fix	: object will not get destroyed until start_listening thread alive.
// 		  previously synchronous socket accept, read and write calls was used in start_listening thread, which are replaced with asynchronous accept, 
// 		  read and write calls. If we dont do these changes then start_listening thread may block on above calls and object will not get destroyed.
// 		  According to fix, object destruction should wait for thread completion, so synchronous calls are replaced with asysnchronous calls.
void TransactionLockManager::start_listening()
{
	OSDLOG(DEBUG, "TransactionLockManager::start_listening thread started ");
	OSDLOG(DEBUG, "TransactionLockManager started listening on port: " <<this->listening_port);

	this->acceptor->async_accept(*(this->sock), boost::bind(&TransactionLockManager::accept_handler, this, boost::asio::placeholders::error));

	OSDLOG(DEBUG, "asynchronus connection acceptor is initiated and io_service is started.");
	this->io_service.run();
	
	OSDLOG(DEBUG, "asynchronus communication is stopped as io_service is stopped.");

}

// OBB-696
void TransactionLockManager::init()
{
	try
	{
//		boost::asio::io_service io_service;
		this->endpoint.reset(new boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), this->listening_port));
		// boost::asio::ip::tcp::acceptor acceptor(this->io_service, endpoint);
		this->acceptor.reset(new boost::asio::ip::tcp::acceptor(this->io_service, *(this->endpoint)));
		boost::asio::socket_base::reuse_address option(false);
		this->acceptor->set_option(option);
//	boost::asio::ip::tcp::socket sock(io_service);
		this->sock.reset(new boost::asio::ip::tcp::socket(this->io_service));
		this->acceptor->listen();
      	}catch(boost::system::system_error &ex)
      	{
        	OSDLOG(ERROR, "OBB-633 occured");
        	// throw ex;
		exit(1);
      	}
}

void TransactionLockManager::commit_recovered_data(std::list<int32_t>& component_list, bool base_version_changed)
{
	boost::mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO,"Committing recovered data in main memory");
	std::map<int32_t,std::pair<std::map<int64_t,
                  boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it1 = this->mem_map.begin();
        for ( ; it1 != this->mem_map.end(); it1++ )
        {
                std::list<int32_t>::iterator comp_it = std::find(component_list.begin(), component_list.end(), it1->first);
                if ( comp_it != component_list.end() )
                {
			if ( it1->second.second )
			{
				continue;
			}
                        it1->second.second = true;
			std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >
					::iterator it2 = it1->second.first.begin();
			for ( ; it2 != it1->second.first.end(); ++it2 )
			{
				std::string obj_name = it2->second->get_object_name();
				std::map<std::string, element_list>::iterator name_it = this->name_map.find(obj_name);
				element_ptr new_node(new helper_utils::transactionIdElement);
				new_node->transaction_id = it2->second->get_transaction_id();
				new_node->operation = it2->second->get_operation_type();
				if ( name_it != this->name_map.end() )
				{
					OSDLOG(INFO, "Lock present for object. Checking ID");
					element_list::iterator list_it = name_it->second.begin();
					if ( list_it != name_it->second.end() )
					{
						while((*list_it)->transaction_id != new_node->transaction_id)
						{
							list_it++;
							if (list_it == name_it->second.end()) {
								break;
							}
						}
						if (list_it == name_it->second.end() )
						{
							OSDLOG(INFO,"Inserting another lock for object " << obj_name  );
							name_it->second.push_back(new_node);
						}
					}
				}
				else
				{
					element_list list;
					list.push_back(new_node);
			                this->name_map.insert(std::make_pair(obj_name, list));	
				}
				if (this->comp_map.find(it2->second->get_transaction_id()) 
									== this->comp_map.end())
				{
					OSDLOG(INFO, "Inserting entry in comp_map");
					this->comp_map.insert(
						std::make_pair(it2->second->get_transaction_id(), it1->first));
				}
			}
                }
                else
                {
                        if ( base_version_changed )
                        {
                                this->mem_map.erase(it1);
                        }
                }
        }
}

boost::python::list TransactionLockManager::recover_active_record(std::string obj_name , int32_t component)
{

	boost::python::list rec_obj_list;
	element_list list_rec;
	OSDLOG(INFO,"TransactionLockManager::recover_active_record");
	if(!(obj_name.empty()))
	{
		if((name_map.find(obj_name))!= name_map.end())
		{
			list_rec = name_map[obj_name];
			std::list<element_ptr>::iterator elem_it ;
			std::map<int32_t,std::pair<std::map<int64_t,
		                boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator it_mem;

			for(elem_it = list_rec.begin();elem_it != list_rec.end();elem_it++)
			{
				int64_t trans_id = (*elem_it)->transaction_id;
				it_mem = mem_map.find(component);	
				if(it_mem != mem_map.end())
				{
					std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >
        	                                ::iterator it_rec = it_mem->second.first.begin();
					for ( ; it_rec != it_mem->second.first.end(); ++it_rec )
					{
						recordStructure::ActiveRecord rec_tmp = *(it_rec->second);
						if(trans_id == (rec_tmp.get_transaction_id()))
						{
							rec_obj_list.append(rec_tmp);
							break;		
						}

					}		
				
				}
			}
			if((boost::python::len(rec_obj_list ))== 0)
			{
				OSDLOG(ERROR,"TransactionLockManager::recover_active_record no transcation id corresponding to object name ");
			}
		}		
		else
		{
			 OSDLOG(ERROR,"TransactionLockManager::recover_active_record object name is not found in name map");
		}
	}	
	else
	{
		OSDLOG(ERROR,"TransactionLockManager::recover_active_record object name is not correct");
	}

	return rec_obj_list;

}

} // namespace transaction_memory
