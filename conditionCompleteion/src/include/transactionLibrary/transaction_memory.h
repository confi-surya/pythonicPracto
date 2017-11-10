#ifndef __TRANSACTIONMEMORY_H__
#define __TRANSACTIONMEMORY_H__

#include <boost/shared_ptr.hpp>
#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include "libraryUtils/record_structure.h"
#include "libraryUtils/config_file_parser.h"

//Performance : Header Files
#include "libraryUtils/stats.h"
#include "libraryUtils/statManager.h"
using namespace OSDStats;

typedef boost::shared_ptr<helper_utils::transactionIdElement> element_ptr;
typedef std::list<element_ptr> element_list;
typedef boost::shared_ptr<recordStructure::ActiveRecord> active_record_ptr;

//#define MAX_ID_LIMIT 72057594037927935
#define MAX_ID_LIMIT 63050394783186943

namespace transaction_memory
{

class TransactionLockManager
{
	public:
		TransactionLockManager(cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,uint8_t node_id, int listening_port);
		~TransactionLockManager();
		bool release_lock_by_id(int64_t transaction_id);
		bool get_id_list_for_name(std::string object_name, element_list& id_list);
		std::string get_name_for_id(int64_t transaction_id);
		void internal_release_lock_by_id(int64_t transaction_id);
		int64_t acquire_transaction_lock(boost::shared_ptr<recordStructure::ActiveRecord> record);
		bool add_records_in_memory(
				std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries, bool memory_flag);
		bool is_empty();
		bool check_for_queue_size(int32_t size);
		bool is_full();
		std::vector<int64_t> get_active_id_list();
		std::map<int32_t,std::pair<std::map<int64_t,
				boost::shared_ptr<recordStructure
							::ActiveRecord> >, bool> > get_memory();
		void set_last_transaction_id(int64_t last_transaction_id);
		std::map<int64_t, recordStructure::ActiveRecord> get_transaction_map();
		int32_t get_component_name(std::string object_path);
		int64_t get_unique_id(uint64_t node_id,uint64_t id);
		std::map<int32_t ,std::list<recordStructure::ActiveRecord> > 
					get_component_transaction_map(std::list<int32_t>& component_names);
		void mark_unknown_memory(std::list<int32_t>& component_list);
		void commit_recovered_data(std::list<int32_t>& component_list, bool base_version_changed);
		std::list<std::string> get_bulk_delete_obj_list();
		void remove_bulk_delete_entry(std::list<int64_t>& bulk_transaction_ids);
		boost::python::list recover_active_record(std::string obj_name , int32_t component);
		void init();

	private:
		boost::asio::io_service io_service;
		//boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor;
		int64_t id;
		int listening_port;
		boost::shared_ptr<boost::asio::ip::tcp::socket> sock;
		std::map<std::string, element_list> name_map;
		std::map<int32_t,std::pair<std::map<int64_t,
                       boost::shared_ptr<recordStructure::ActiveRecord> >, bool> > mem_map;
		//New map in which transaction id is the key and component name is the value
		//Below Map is used in case of realease lock by transaction ID case.
                std::map<int64_t,int32_t> comp_map;
		std::string get_transaction_map_in_string();
		std::string get_bulk_delete_obj_list_in_string();
		boost::mutex mtx;
		boost::thread monitor;
		boost::shared_ptr<boost::asio::ip::tcp::endpoint> endpoint;
        	boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor;
		boost::shared_array<char> sock_read_buffer;
		boost::shared_array<char> sock_write_buffer;

		//Performance : variable to register the transaction memory stats
		boost::shared_ptr<StatsInterface> trans_mem_stats;

		void start_listening();
		void accept_handler(const boost::system::error_code &ec);
        	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
		void handle_write(const boost::system::error_code &error);
};

class TransactionStoreManager
{   
	public:
		TransactionStoreManager(cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,uint8_t node_id, int listening_port);
		~TransactionStoreManager();

		int64_t acquire_lock(boost::shared_ptr<recordStructure::ActiveRecord> record);
		void mark_unknown_memory(std::list<int32_t>& component_list);
		void commit_recovered_data(std::list<int32_t>& component_list, bool base_version_changed);
		bool add_records_in_memory(
			std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries, bool memory_flag = false);
		std::map<int32_t,std::pair<std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >, bool> > get_memory();
		std::vector<int64_t> get_active_id_list();	
		bool check_for_queue_size(int32_t size);
		bool release_lock_by_id(int64_t transaction_id);
		bool get_id_list_for_name(std::string object_name, element_list& id_list);
		std::string get_name_for_id(int64_t transaction_id);
		void set_last_transaction_id(int64_t last_transaction_id);
		std::map<int64_t, recordStructure::ActiveRecord> get_transaction_map();
		std::map<int32_t,std::list<recordStructure::ActiveRecord> > 
			get_component_transaction_map(std::list<int32_t>& component_names);
		std::list<std::string> get_bulk_delete_obj_list();
		void remove_bulk_delete_entry(std::list<int64_t>& bulk_transaction_ids);
		boost::python::list recover_active_record(std::string obj_name , int32_t component);

	private:
		boost::shared_ptr<TransactionLockManager> lock_manager_ptr;
};

}// transaction_memory

#endif
