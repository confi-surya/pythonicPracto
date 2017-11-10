#ifndef __TRANSACTIONLIBRARY_H__
#define __TRANSACTIONLIBRARY_H__

#include <boost/shared_ptr.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>

#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/helper.h"
#include "libraryUtils/journal.h"
#include "libraryUtils/record_structure.h"
#include "transactionLibrary/status.h"
#include "transactionLibrary/transaction_memory.h"
#include "transactionLibrary/transaction_write_handler.h"
#include "transactionLibrary/transaction_read_handler.h"
#include "transactionLibrary/journal_library.h"

//Performance : Header files
#include "libraryUtils/osdStatGather.h"
#include "libraryUtils/statManager.h"
#include "libraryUtils/stats.h"

using namespace OSDStats;

using namespace object_storage_log_setup;

namespace transaction_library{

class BaseTransactionImpl
{
	public:
		BaseTransactionImpl();
		virtual ~BaseTransactionImpl();

		virtual int64_t add_transaction(
			boost::shared_ptr<helper_utils::Request> request,
			int32_t poll_fd
		) = 0;
                virtual bool accept_transaction_data(
                     std::list<recordStructure::ActiveRecord> transaction_entries, 
		     int32_t poll_fd
		) = 0;
                virtual StatusAndResult::Status<bool> upgrade_accept_transaction_data(
                     //std::list<recordStructure::ActiveRecord> transaction_entries, 
		     boost::python::list,
		     int32_t poll_fd
		) = 0;
		virtual void revert_back_trans_data(
	                std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries, 
			bool memory_flag
		) = 0;
		virtual void commit_recovered_data(std::list<int32_t> component_list, bool base_version_changed) = 0;

		virtual boost::python::dict
			extract_transaction_data(std::list<int32_t> component_names) = 0;
		virtual boost::python::dict
         		extract_trans_journal_data(std::list<int32_t> component_names) = 0;
                //
		virtual int64_t add_bulk_delete_transaction(
			std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries, 
			int32_t poll_fd) = 0;
		virtual bool release_lock(int64_t transaction_id, int32_t poll_fd) = 0;
		virtual bool release_lock(std::string object_name, int32_t poll_fd) = 0;
		virtual std::map<int64_t, recordStructure::ActiveRecord> get_transaction_map() = 0;
//		virtual boost::unordered_map<int, ExpiredTransaction> get_expired_transactions() = 0;
		virtual bool start_recovery(std::list<int32_t> component_list) = 0;
		virtual boost::python::list  recover_active_record(std::string obj_name , int32_t component)=0;
		virtual int64_t event_wait() = 0;
		virtual void event_wait_stop() = 0;
};

class TransactionLibrary
{
	public:
		TransactionLibrary(std::string transaction_journal_path, int service_node_id, int listening_port);
		~TransactionLibrary();
		StatusAndResult::Status<int64_t> add_bulk_delete_transaction(
			boost::python::list active_records, int32_t poll_fd
		);
		StatusAndResult::Status<int64_t> add_transaction(helper_utils::Request request, int32_t poll_fd);
		StatusAndResult::Status<bool> accept_transaction_data(
   	              boost::python::list records, int32_t poll_fd
		);
                StatusAndResult::Status<bool> upgrade_accept_transaction_data(
                      boost::python::list records, int32_t poll_fd
                );

		void revert_back_trans_data(
                        boost::python::list records, 
                        bool memory_flag
                );
		StatusAndResult::Status<bool> commit_recovered_data(std::list<int32_t> component_list, bool base_version_changed);
		
		boost::python::dict
                        extract_transaction_data(std::list<int32_t> component_names
		);
		boost::python::dict
         		extract_trans_journal_data(std::list<int32_t> component_names
		);
		//
		StatusAndResult::Status<bool> release_lock(int64_t transaction_id, int32_t poll_fd);
		StatusAndResult::Status<bool> release_lock(std::string object_name, int32_t poll_fd);
//		boost::unordered_map<int, ExpiredTransaction> get_expired_transactions();
		//std::map<int64_t, recordStructure::ActiveRecord> get_transaction_map();
		boost::python::dict get_transaction_map();
		bool start_recovery(std::list<int32_t> component_list);	
		boost::python::list  recover_active_record(std::string obj_name , int32_t component);
		int64_t event_wait();
		void event_wait_stop();

	private:
		boost::shared_ptr<BaseTransactionImpl> transaction_impl;

		//Performance : Variable for stat framework initialization
                boost::shared_ptr<OSDStats::OsdStatGatherFramework> statFramework;
                boost::shared_ptr<OSDStats::StatsManager> statsManager;
		int service_node_id;
};

class TransactionImpl: public BaseTransactionImpl
{
	public:
		TransactionImpl(
			std::string & transaction_journal_path,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
			uint8_t node_id,
			int listening_port
		);
		~TransactionImpl();
		int64_t add_transaction(
			boost::shared_ptr<helper_utils::Request> request,
			int32_t poll_fd
		);
		int64_t add_bulk_delete_transaction(
			std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries, 
			int32_t poll_fd
		);
		void revert_back_trans_data(
                        std::list<boost::shared_ptr<recordStructure::ActiveRecord> > transaction_entries, 
                        bool memory_flag
                );
		bool accept_transaction_data(
                std::list<recordStructure::ActiveRecord> transaction_entries, 
		int32_t poll_fd 
		);
                StatusAndResult::Status<bool> upgrade_accept_transaction_data(
                //std::list<recordStructure::ActiveRecord> transaction_entries,
		boost::python::list,
                int32_t poll_fd
                );
		void commit_recovered_data(std::list<int32_t> component_list, bool base_version_changed);
		boost::python::dict
                        extract_transaction_data(std::list<int32_t> component_names
		);
		boost::python::dict
         		extract_trans_journal_data(std::list<int32_t> component_names
		);
		bool release_lock(int64_t transaction_id, int32_t poll_fd);
		bool release_lock(std::string object_name, int32_t poll_fd);
		std::map<int64_t, recordStructure::ActiveRecord> get_transaction_map();
//		boost::unordered_map<int, ExpiredTransaction> get_expired_transactions();
		bool start_recovery(std::list<int32_t> component_list);
		boost::python::list  recover_active_record(std::string obj_name , int32_t component);
		void finish_recovery(transaction_read_handler::TransactionRecovery & recovery_obj);
		int64_t event_wait();
		void event_wait_stop();
	private:
		std::string journal_path;
		boost::shared_ptr<transaction_memory::TransactionStoreManager> transaction_memory_ptr;
		boost::shared_ptr<Journal> journal_ptr;
		boost::mutex mutex;
		boost::condition condition;
		bool stop_requested;
		std::queue<int64_t> waiting_events;
		void finish_event(int32_t poll_fd);
		void dummy_event(int32_t poll_fd);
		uint8_t node_id;

		//Performance : Variable for registering tranaction library for stat dump
		boost::shared_ptr<StatsInterface> trans_lib_stats;

};

cool::SharedPtr<config_file::ConfigFileParser> const get_config_parser();

}// transaction_library namespace

#endif
