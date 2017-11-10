#ifndef __CONTAINER_LIBRARY_H__
#define __CONTAINER_LIBRARY_H__

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include "status.h"
#include "libraryUtils/record_structure.h"
//#include "containerLibrary/journal_reader.h"
#include "journal_library.h"
#include "memory_writer.h"
#include "file_writer.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"

//Performance : Header Files
#include "libraryUtils/osdStatGather.h"
#include "libraryUtils/statManager.h"
#include "libraryUtils/stats.h"


using namespace StatusAndResult;

using namespace OSDStats;

namespace container_read_handler
{
	class ContainerRecovery;
}

namespace updater_maintainer
{
	class UpdaterFileManager;
}

namespace container_library
{

using recordStructure::JournalOffset;

class ContainerLibrary
{

	private:
		virtual void add_entry_in_journal(
			std::string const & path,
			recordStructure::ObjectRecord & record,
			boost::function<void(void)> const & done
		) = 0;
		virtual void add_recovered_entry_in_journal(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
			boost::function<void(void)> const & done) = 0;
		
	public:
		virtual void create_container(
			std::string const & temp_path,
			std::string const & path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		) = 0;
		virtual void delete_container(
			std::string const & path,
			Status & result,
			int64_t event_id
		) = 0;
		virtual void update_container(
			std::string & path,
			recordStructure::ObjectRecord & data,
			int64_t event_id
		) = 0;
		virtual void update_bulk_delete_records(
                       std::string path,
                       std::list<recordStructure::FinalObjectRecord>& object_names,
                       int64_t event_id
                ) = 0;
		virtual void list_container(
			std::string const & path,
			ListObjectWithStatus & result,
			uint64_t count = 0,
			std::string marker = "",
			std::string end_marker = "",
			std::string prefix = "",
			std::string delimiter = "",
			int64_t event_id = 0
		) = 0;
		virtual void get_container_stat(
			std::string const & path,
			ContainerStatWithStatus &,
			bool request_from_updater,
			int64_t event_id
		) = 0;
		virtual int64_t event_wait() = 0;
		virtual void event_wait_stop() = 0;
		virtual void set_container_stat(
			std::string const & path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		) = 0;
		virtual bool start_recovery(std::list<int32_t>& component_names) = 0;
		virtual void accept_container_data(
			std::list<recordStructure::FinalObjectRecord>& object_records, 
			bool request_from_recovery,
			std::list<int32_t> component_number_list,
			int64_t event_id) = 0;
		virtual void revert_back_cont_data(boost::python::list records, bool memory_flag) = 0;
		virtual void remove_transferred_records(std::list<int32_t>& component_names) = 0;
		virtual void commit_recovered_data(std::list<int32_t>& component_names, bool base_version_changed, int32_t event_id) = 0;
		virtual boost::python::dict 
                        extract_container_data(std::list<int32_t>& component_names,
                                               bool request_from_recovery = false
			) = 0;
		virtual boost::python::dict 
			extract_cont_journal_data(std::list<int32_t>& component_names
			) = 0;
		virtual void flush_all_data(int64_t event_id) = 0;
		virtual ~ContainerLibrary() = 0;
};

class ContainerLibraryImpl: public ContainerLibrary
{
	private:
		boost::asio::io_service io_service_obj;
		boost::shared_ptr<boost::asio::io_service::work> work;
		boost::thread_group thread_pool;
		std::string journal_path;
		boost::shared_ptr<JournalInterface> journal_ptr;
		boost::shared_ptr<MemoryManager> memory_ptr;
		boost::shared_ptr<FileWriter> file_writer_ptr;
		boost::shared_ptr<updater_maintainer::UpdaterFileManager> updater_ptr;
		boost::shared_ptr<containerInfoFile::ContainerInfoFileManager> info_file_ptr;
		uint32_t cont_mem_size;
		uint8_t node_id;

		//Performance : Declare Stat Interface and Framework
                boost::shared_ptr<StatsInterface> cont_lib_stats;

		int64_t event_wait();
		void event_wait_stop();
		void finish_recovery(container_read_handler::ContainerRecovery & rec_obj);
		void add_entry_in_journal(
			std::string const & path,
			recordStructure::ObjectRecord & record,
			boost::function<void(void)> const & done
		);
		void add_bulk_delete_records_in_journal(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
                        boost::function<void(void)> const & done
                );
		void add_recovered_entry_in_journal(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
			boost::function<void(void)> const & done);

		boost::mutex mutex;
		boost::condition condition;
		bool stop_requested;
		std::queue<int64_t> waiting_events;
		void finish_event(int64_t id);
		void finish_event_and_flush_container(int64_t id);
		void finish_event_with_lock(int64_t id);

		void list_container_internal(
			std::string const & path,
			ListObjectWithStatus & result,
			uint64_t count = 0,
			std::string marker = "",
			std::string end_marker = "",
			std::string prefix = "",
			std::string delimiter = "",
		//TODO asrivastava, this needs to be corrected should not be inittialized with value, should not be last variable	
			int64_t event_id = 0
		);

		void get_container_stat_internal(
			std::string const & path,
			ContainerStatWithStatus & result,
			bool request_from_updater,
			int64_t event_id
		);
		void set_container_stat_internal(
			std::string const & path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		);
		uint64_t get_unique_file_id(
		uint64_t node_id, 
		uint64_t file_id
		);

	public:
		void create_container(
			std::string const & temp_path,
			std::string const & path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		);
		void create_container_internal(
			std::string const & temp_path,
			std::string const & path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		);
		void delete_container(
			std::string const & path,
			Status & result,
			int64_t event_id
		);
		void delete_container_internal(
			std::string const & path,
			Status & result,
			int64_t event_id
		);
		void update_container(
			std::string & path,
			recordStructure::ObjectRecord & data,
			int64_t event_id
		);
		void update_bulk_delete_records(
                       std::string path,
                       std::list<recordStructure::FinalObjectRecord>& object_names,
                       int64_t event_id
                );
		void list_container(
			std::string const & path,
			ListObjectWithStatus & result,
			uint64_t count = 0,
			std::string marker = "",
			std::string end_marker = "",
			std::string prefix = "",
			std::string delimiter = "",
			int64_t event_id = 0
		);

/*		static void  get_container_stat_for_updater(
			std::string const & path,
			ContainerStatWithStatus &
		);
*/		void  get_container_stat(
			std::string const & path,
			ContainerStatWithStatus &,
			bool request_from_updater,
			int64_t event_id
		);
		void set_container_stat(
			std::string const & path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		);
		bool start_recovery(std::list<int32_t>& component_names);
		void accept_container_data(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
			bool request_from_recovery,
                        std::list<int32_t> component_number_list,
			int64_t event_id);
		void revert_back_cont_data(boost::python::list records, bool memory_flag);
		void remove_transferred_records(std::list<int32_t>& component_names);
		void commit_recovered_data(std::list<int32_t>& component_names, bool base_version_changed, int32_t event_id);
		boost::python::dict 
			extract_container_data(std::list<int32_t>& component_names, bool request_from_recovery = false);
		boost::python::dict 
                        extract_cont_journal_data(std::list<int32_t>& component_names);
		void flush_all_data(int64_t event_id);
		ContainerLibraryImpl(std::string journal_path,uint8_t node_id);
		~ContainerLibraryImpl();
		
};

class LibraryInterface
{
	public:
		virtual void create_container(
			std::string const & temp_path,
			std::string const path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		) = 0;
		virtual void delete_container(
			std::string const path,
			Status & result,
			int64_t event_id
		) = 0;
		virtual Status update_container(
			std::string path,
			recordStructure::ObjectRecord & record,
			int64_t event_id
		) = 0;
		virtual Status update_bulk_delete_records(
                       std::string path,
                       boost::python::list object_names,
                       int64_t event_id
                ) = 0;
		virtual void list_container(
			std::string const path,
			ListObjectWithStatus & result,
			uint64_t count = 0,
			std::string marker = "",
			std::string end_marker = "",
			std::string prefix = "",
			std::string delimiter = "",
			int64_t event_id = 0
		) = 0;
		virtual void get_container_stat(
			std::string const path,
			ContainerStatWithStatus &,
			bool request_from_updater,
			int64_t event_id
		) = 0;
		virtual void set_container_stat(
			std::string const path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id	
		) = 0;
		virtual bool start_recovery(std::list<int32_t> component_names) = 0;
		virtual ~LibraryInterface() = 0;
		virtual int64_t event_wait() = 0;
		virtual void event_wait_stop() = 0;
		virtual Status accept_container_data(
                        boost::python::list records,
			bool request_from_recovery,
                        std::list<int32_t> component_number_list,
			int64_t event_id ) = 0;
		virtual void revert_back_cont_data(boost::python::list records, bool memory_flag) = 0;
		virtual void remove_transferred_records(std::list<int32_t> component_names) = 0;
		virtual boost::python::dict
                        extract_container_data(std::list<int32_t> component_names
			) = 0;
		virtual boost::python::dict
                        extract_cont_journal_data(std::list<int32_t> component_names
			) = 0;
		virtual Status flush_all_data(int64_t event_id) = 0;
		virtual Status commit_recovered_data(std::list<int32_t> component_names, bool base_version_changed, int32_t event_id) = 0;

};

class LibraryImpl: public LibraryInterface
{
private:
//	std::string journal_path;
	boost::shared_ptr<ContainerLibrary> container_library_ptr;

	//Performance :  Stat Framework declaration
        boost::shared_ptr<OSDStats::OsdStatGatherFramework> statFramework;
        boost::shared_ptr<OSDStats::StatsManager> statsManager;
	int service_node_id;
	
	public:
		void create_container(
			std::string const & temp_path,
			std::string const path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		);
		void delete_container(
			std::string const path,
			Status & result,
			int64_t event_id
		);
		Status update_container(
			std::string path,
			recordStructure::ObjectRecord & record,
			int64_t event_id
		);
		Status update_bulk_delete_records(
                       std::string path,
                       boost::python::list object_names,
                       int64_t event_id
                );
		void  list_container(
			std::string const path,
			ListObjectWithStatus & result,
			uint64_t count = 0,
			std::string marker = "",
			std::string end_marker = "",
			std::string prefix = "",
			std::string delimiter ="",
			int64_t event_id = 0
		);
		void get_container_stat(
			std::string const path,
			ContainerStatWithStatus & result,
			bool request_from_updater,
			int64_t event_id
		);

/*		static void get_container_stat_for_updater(
			std::string const path,
			ContainerStatWithStatus &
		);
*/
		void set_container_stat(
			std::string const path,
			recordStructure::ContainerStat & container_stat_data,
			Status & result,
			int64_t event_id
		);
		bool start_recovery(std::list<int32_t> component_names);
		int64_t event_wait();
		void event_wait_stop();
		Status accept_container_data(
                        boost::python::list records,
			bool request_from_recovery,
                        std::list<int32_t> component_number_list,
			int64_t event_id);
		void revert_back_cont_data(boost::python::list records, bool memory_flag);
		void remove_transferred_records(std::list<int32_t> component_names);
		Status commit_recovered_data(std::list<int32_t> component_names, bool base_version_changed, int32_t event_id);
		boost::python::dict 
                       extract_container_data(std::list<int32_t> component_names);
		boost::python::dict 
                       extract_cont_journal_data(std::list<int32_t> component_names);
		Status flush_all_data(int64_t event_id);
		LibraryImpl(std::string journal_path, int service_node_id);
		~LibraryImpl();

};

} // namespace container_library

#endif
