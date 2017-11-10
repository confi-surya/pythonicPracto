#ifndef __ACCOUNT_FILE_MANAGER_H__
#define __ACCOUNT_FILE_MANAGER_H__

#include <boost/shared_ptr.hpp>
#include <list>

#include "accountLibrary/status.h"
#include "record_structure.h"
#include "accountLibrary/account_info_file_builder.h"
#include "libraryUtils/lock_manager.h"

namespace accountInfoFile
{
using StatusAndResult::Status;
using StatusAndResult::AccountStatWithStatus;

class AccountInfoFileManager
{
public:
	AccountInfoFileManager();
	void add_container(std::string path,  boost::shared_ptr<recordStructure::ContainerRecord> container_record);
	std::list<recordStructure::ContainerRecord> list_container(std::string path, 
			uint64_t count, std::string marker, std::string end_marker, std::string prefix, std::string delimiter);
        AccountStatWithStatus get_account_stat(std::string const & temp_path, std::string const & path );
        void set_account_stat(std::string const & path, boost::shared_ptr<recordStructure::AccountStat>  account_stat);
	Status create_account(
			std::string const & temp_path,
			std::string const & path,
			boost::shared_ptr<recordStructure::AccountStat> account_stat);
	Status delete_account(std::string const & temp_path, std::string const & path);
	
	//use this interface is used by account updater
	void update_container(std::string path, std::list<recordStructure::ContainerRecord> contianer_records);

	void delete_container(std::string path, boost::shared_ptr<recordStructure::ContainerRecord> container_record);

private:
	void create_dir_if_not_exist(std::string path);
	void move_account_info_file(std::string source_file, std::string destination_file);
	std::string get_file_name(std::string path);
	std::string get_temp_file_path(std::string temp_path, std::string path, bool deleted_file_path=false);
 
	boost::shared_ptr<AccountInfoFileBuilder> account_infoFile_builder;
	boost::shared_ptr<lockManager::LockManager> lock_manager;
};

}
#endif
