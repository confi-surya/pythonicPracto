#ifndef __MEMORY_WRITER_H__
#define __MEMORY_WRITER_H__

#include <iostream>
#include <map>
#include <list>
#include <string>

#include "libraryUtils/record_structure.h"

#define NUMBER_OF_COMPONENTS 512

namespace container_library
{

using recordStructure::JournalOffset;
typedef std::list<std::pair<JournalOffset, recordStructure::ObjectRecord> > RecordList;


class SecureObjectRecord
{
	RecordList record_list;
	RecordList tmp_record_list;
	JournalOffset last_commit_offset;
	recordStructure::ContainerStat stat;

public:
	SecureObjectRecord(recordStructure::ObjectRecord const & record_obj,
			JournalOffset prev_off, JournalOffset sync_off);
	SecureObjectRecord(const SecureObjectRecord &obj);
	RecordList * get_flush_records();
	boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > get_list_records();
	bool release_flush_records();
	JournalOffset get_last_commit_offset() const;
	void add_record(recordStructure::ObjectRecord const & record_obj,
			JournalOffset sync_off);
	void revert_record(recordStructure::ObjectRecord const & record_obj,
                        JournalOffset offset);
	bool remove_record(JournalOffset offset);
	recordStructure::ContainerStat get_stat();
	bool is_flush_ongoing();
	RecordList get_unscheduled_records();
	bool is_object_record_exist(recordStructure::ObjectRecord const & record_obj);
};

class MemoryManager
{
private:
	std::map<int32_t, std::pair<std::map<std::string, SecureObjectRecord>, bool> > memory_map;
	std::set<std::string> unknown_flag_set;
	uint32_t count;
	uint32_t recovered_count;
	boost::mutex mutex;
	boost::mutex status_set_mutex;
	void add_record_in_map(std::string const & path,
			recordStructure::ObjectRecord const & record_obj,
			JournalOffset prev_off, JournalOffset sync_off,bool memory_flag);
	bool recovery_data_flag;
public:
	MemoryManager();
//		void flushMemory(std::string const & accountName = "", std::string const & containerName = "");
	uint32_t get_record_count();
	uint32_t get_container_count();
	const std::list<std::string> get_cont_list_in_memory(int32_t component_number);
	void add_entry(std::string const & path,
			recordStructure::ObjectRecord const & record_obj,
			JournalOffset last_commit_offset,
			JournalOffset sync_off,
			bool memory_flag);
	void revert_back_cont_data(recordStructure::FinalObjectRecord & final_obj_record, JournalOffset offset, bool memory_flag);
	void remove_transferred_records(std::list<int32_t>& component_names);
	void add_entries(std::list<std::pair<bool, std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > > &,
			JournalOffset prev_off, JournalOffset sync_off);
	std::map<int32_t, std::pair<std::map<std::string, SecureObjectRecord>, bool> >& get_memory_map();
	RecordList * get_flush_records(std::string const & path, bool & unknown_status, bool request_from_updater = false);
	void release_flush_records(std::string const & path);
	boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > get_list_records(std::string const & path);
	void release_list_records(std::string const & path);
	JournalOffset get_last_commit_offset();
	void flush_containers(boost::function<void(std::string const & path)> write_func);
	void reset_mem_size();
	void remove_entry(std::string const & path, JournalOffset offset);
        void remove_components(std::list<int32_t> component_list);
	recordStructure::ContainerStat get_stat(std::string const & path);
	bool delete_container(std::string const & path);
	bool is_flush_running(std::string const & path);
	void reset_in_memory_count(std::string const & path, uint32_t decrement_count);
	bool is_unknown_flag_contains(std::string const & path);
	void add_unknown_flag_key(std::string path , const recordStructure::ObjectRecord& obj);
	void remove_unknown_flag_key(std::string path);
	int32_t get_component_name(std::string path);
	bool commit_recovered_data(std::list<int32_t>& component_list, bool base_version_changed);
	std::map<int32_t, std::list<recordStructure::TransferObjectRecord> > 
			get_component_object_records(std::list<int32_t>& component_names, bool request_from_recovery);
	uint32_t get_recovered_record_count();
	bool flush_all_data();
	void mark_unknown_memory(std::list<int32_t>& component_list);
	bool is_object_exist(std::string const & path,
                                recordStructure::ObjectRecord const & record_obj);
	void set_recovery_flag();
	bool get_recovery_flag();
	
};


} //namespace containerLibrary

#endif
