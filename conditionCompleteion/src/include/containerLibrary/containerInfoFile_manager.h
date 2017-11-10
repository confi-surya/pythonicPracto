#ifndef __CONTAINER_FILE_MANAGER_H__
#define __CONTAINER_FILE_MANAGER_H__

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "status.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/lock_manager.h"
#include "containerInfoFile_builder.h"
#include "containerInfoFile.h"
#include "cool/shellCommandsExecution.h"

using namespace StatusAndResult;

namespace containerInfoFile
{

class ContainerInfoFileManager
{

public:
	Status flush_container(
			std::string path,
			RecordList * record_list,
			bool unknown_status,
			boost::function<void(void)> const & release_flush_records,
			boost::function<bool(void)> mem_delete
			);
	void get_object_list(
			std::string const & path,
			std::list<recordStructure::ObjectRecord> * list,
			boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records,
			uint64_t const count, 
			std::string const & marker,
			std::string const & end_marker,
			std::string const & prefix,
			std::string const & delimiter);

	ContainerInfoFileManager(uint8_t node_id);

        void  get_container_stat(
			std::string const & path,
			recordStructure::ContainerStat* container_stat,
			boost::function<recordStructure::ContainerStat(void)> const & get_stat);

	void get_container_stat_after_compaction(
					std::string const & path,
					recordStructure::ContainerStat* containter_stat,
					RecordList * record_list,
					boost::function<void(void)> const & release_flush_records,
					boost::function<bool(void)> mem_delete
					);

        void set_container_stat(std::string const & path,
					recordStructure::ContainerStat  container_stat);

        Status add_container(
			std::string const & temp_path,
			std::string const & path,
			recordStructure::ContainerStat const container_stat,
			JournalOffset const offset);
        bool delete_container(std::string const & path, boost::function<bool(void)> mem_delete);
        //Status startCompaction(std::string const & path);
       // Status startRecovery(std::string const & path);
       ~ContainerInfoFileManager();
private:
        boost::shared_ptr<ContainerInfoFileBuilder> container_infoFile_builder;
	boost::shared_ptr<lockManager::LockManager> lock_manager;
	uint8_t node_id;
	std::string get_file_name(std::string path);
	std::string get_temp_file_path(std::string temp_path, std::string path);
	void create_dir_if_not_exist(std::string path);
	void move_container_info_file(std::string source_file, std::string destination_file);
};

}

#endif

