#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <cool/debug.h>
#include "accountLibrary/account_info_file_manager.h"

namespace accountInfoFile
{

AccountInfoFileManager::AccountInfoFileManager()
{
	OSDLOG(DEBUG, "AccountInfoFileManager::AccountInfoFileManager() called");
	this->account_infoFile_builder.reset(new accountInfoFile::AccountInfoFileBuilder);
	//this->lock_manager.reset(new lockManager::LockManager(10));
		this->lock_manager.reset(new lockManager::LockManager(1024));   //TODO: discussion required to change this value from 1024 to 10
									// currently 1024 lock is set to avoid threads getting blocked.
}

void AccountInfoFileManager::add_container(std::string path, boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	OSDLOG(DEBUG,  "Manager: add_container is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	boost::shared_ptr<AccountInfoFile> account_infoFile = this->account_infoFile_builder->create_account_for_modify(this->get_file_name(path), mutex);
	account_infoFile->add_container(container_record);
	OSDLOG(DEBUG,  "Manager: add_container exit");
}

void AccountInfoFileManager::update_container(std::string path, std::list<recordStructure::ContainerRecord> container_records)
{
	OSDLOG(DEBUG,  "Manager: update_container is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	boost::shared_ptr<AccountInfoFile> account_infoFile = account_infoFile_builder->create_account_for_modify(this->get_file_name(path), mutex);
	account_infoFile->update_container(container_records);
	OSDLOG(DEBUG,  "Manager: update_container exit");
}

void AccountInfoFileManager::delete_container(std::string path, boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	OSDLOG(DEBUG,  "Manager: delete_container is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	boost::shared_ptr<AccountInfoFile> account_infoFile = this->account_infoFile_builder->create_account_for_modify(this->get_file_name(path), mutex);
	account_infoFile->delete_container(container_record);	
	OSDLOG(DEBUG,  "Manager: delete_container exit");
}


std::list<recordStructure::ContainerRecord> AccountInfoFileManager::list_container(std::string path, 
						uint64_t count, std::string marker, std::string end_marker, std::string prefix, std::string delimiter)
{
	OSDLOG(DEBUG,  "Manager: list_container is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::ReadLock lock(mutex);
	boost::shared_ptr<AccountInfoFile> account_infoFile = this->account_infoFile_builder->create_account_for_reference(this->get_file_name(path), mutex);
	return account_infoFile->list_container(count, marker, end_marker, prefix, delimiter);
}

AccountStatWithStatus AccountInfoFileManager::get_account_stat(std::string const & temp_path, std::string const & path)
{
	OSDLOG(DEBUG,  "Manager: get_account_stat is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::ReadLock lock(mutex);
	std::string deleted_file_path = this->get_temp_file_path(temp_path, path, true); //pass true to get deleted acc file path
	if(boost::filesystem::exists(boost::filesystem::path(deleted_file_path)))
        {
                OSDLOG(INFO,  "Manager: get_account_stat Account info-file is deleted.");
                return AccountStatWithStatus(OSDException::OSDErrorCode::INFO_FILE_DELETED);
        }
	boost::shared_ptr<AccountInfoFile> account_infoFile = this->account_infoFile_builder->create_account_for_reference(this->get_file_name(path), mutex);
	boost::shared_ptr<recordStructure::AccountStat>  account_stat = account_infoFile->get_account_stat();
	OSDLOG(DEBUG,  "Account stat: account:" << account_stat->get_account() << ", container_count: " << account_stat->get_container_count() << ", object_count: " << account_stat->get_object_count() << ", bytes_used: " << account_stat->get_bytes_used());
	OSDLOG(DEBUG,  "Manager: get_account_stat exit");
	return AccountStatWithStatus(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS, *account_stat);
}

void AccountInfoFileManager::set_account_stat(std::string const & path, 
					boost::shared_ptr<recordStructure::AccountStat>  account_stat)
{
	OSDLOG(DEBUG,  "Manager: set_account_stat is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	boost::shared_ptr<AccountInfoFile> account_infoFile = account_infoFile_builder->create_account_for_modify(this->get_file_name(path), mutex);
	account_infoFile->set_account_stat(account_stat);
	OSDLOG(DEBUG,  "Manager: set_account_stat exit");

}

Status AccountInfoFileManager::create_account(
					std::string const & temp_path,
					std::string const & path,
					boost::shared_ptr<recordStructure::AccountStat>  account_stat)
{
	OSDLOG(INFO,  "Manager: create_account is starting");
	std::string file_path = this->get_file_name(path);
	std::string temp_file_path = this->get_temp_file_path(temp_path, path);
	std::string deleted_file_path = this->get_temp_file_path(temp_path, path, true); //pass true to get deleted acc file path
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	if(boost::filesystem::exists(boost::filesystem::path(file_path)))
	{
		OSDLOG(WARNING,  "Account info-file Already Exists.");
		boost::shared_ptr<AccountInfoFile> account_infoFile = this->account_infoFile_builder->create_account_for_modify(this->get_file_name(path), mutex);
		account_infoFile->set_account_stat(account_stat, true);
		return Status(OSDException::OSDSuccessCode::INFO_FILE_ACCEPTED);
	}
	else if( boost::filesystem::exists(boost::filesystem::path(temp_file_path)))
	{
		OSDLOG(WARNING,  "Temporary acccount info-file already Exists.");
		return Status(OSDException::OSDErrorCode::INFO_FILE_ALREADY_EXIST);
	}
	else if(boost::filesystem::exists(boost::filesystem::path(deleted_file_path)))
	{
		OSDLOG(WARNING,  "Account info-file is already deleted.");
		return Status(OSDException::OSDErrorCode::INFO_FILE_DELETED);
	}
	else
	{
		OSDLOG(DEBUG,  "Account info-file directory is creating");
		create_dir_if_not_exist(path);	
	}
	boost::shared_ptr<AccountInfoFile> account_infoFile = this->account_infoFile_builder->create_account_for_create(temp_file_path, mutex);
	account_infoFile->create_account(account_stat);
	//TODO [Amit]->remove the this line in stable branch
	char const * const test_mode = ::getenv("TMP_FILE_CREATED");
	PASSERT(!test_mode, "Temp file created");	
	this->move_account_info_file(temp_file_path, file_path);
	OSDLOG(DEBUG,  "Manager: create_account exit");
	return Status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS); 
}

Status AccountInfoFileManager::delete_account(std::string const & temp_path, std::string const & path)
{
	OSDLOG(DEBUG,  "Manager: delete_account is starting");
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	std::string file_path = this->get_file_name(path);
	std::string deleted_file_path = this->get_temp_file_path(temp_path, path, true); //pass true to get deleted acc file path
	if(boost::filesystem::exists(boost::filesystem::path(file_path)))
	{
		this->move_account_info_file(file_path, deleted_file_path);
	}
	else if(boost::filesystem::exists(boost::filesystem::path(deleted_file_path)))
	{
		OSDLOG(WARNING,  "Manager: account info-file is already deleted.");
		return Status(OSDException::OSDErrorCode::INFO_FILE_DELETED);
		
	}
	else
	{
		OSDLOG(ERROR,  "Manager: account info-file not found.");
		return Status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
	}

	OSDLOG(DEBUG,  "Manager: delete_account exit");
	return Status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
}


void AccountInfoFileManager::create_dir_if_not_exist(std::string path)
{
	boost::filesystem::path dir_path(path);
	if(!(boost::filesystem::exists(dir_path)))
	{
		boost::filesystem::create_directories(dir_path); //TODO:exception handling
	}
}

void AccountInfoFileManager::move_account_info_file(std::string source_file, std::string destination_file)
{
	//move file : TODO: boost exception handling
	boost::filesystem::rename(source_file, destination_file);
	OSDLOG(INFO,  "Manager: renaming done, source file :"<<source_file<<", target file :"<<destination_file);
	//TODO [Amit]->remove the this line in stable branch
	char const * const test_mode = ::getenv("INFO_FILE_CREATED");
	PASSERT(!test_mode, "Info file created");
}

std::string AccountInfoFileManager::get_temp_file_path(std::string temp_path, std::string path, bool deleted_file_path)
{
	std::vector<std::string> path_tokens;
	boost::split(path_tokens, path, boost::is_any_of("/"));
	if(path_tokens.size() > 0)
	{
		std::string temp_file_parent_dir;
		std::string temp_file_name;
		std::string sub_dir;
		if(deleted_file_path)
		{
			sub_dir = "del"; // delete account info-file directory
		}
		else
		{
			sub_dir = "temp";  // create account temporary info-file directory 
		}
		temp_file_parent_dir = temp_path + "/" + sub_dir + "/";
		this->create_dir_if_not_exist(temp_file_parent_dir);
		temp_file_name = temp_file_parent_dir + path_tokens[ path_tokens.size()- 1 ] + ".infoFile";//create tempfile
		OSDLOG(DEBUG,  "Manager: Tempory account info-file path "<<temp_file_name);
		return temp_file_name;
	}
        else
	{
		OSDLOG(ERROR,  "AccountInfoFileManager::get_temp_file_path Invalid File Path   "<<path);
		return "";
	}
}

std::string AccountInfoFileManager::get_file_name(std::string path)
{

	std::vector<std::string> path_tokens;
	boost::split(path_tokens, path, boost::is_any_of("/"));
	if(path_tokens.size() > 0)
	{
		std::string file_name;
		file_name = path + "/" + path_tokens[ path_tokens.size()- 1 ] + ".infoFile"; // create file name
		OSDLOG(DEBUG,  "Manager: file path "<<file_name);
		return file_name;
	}
	else
	{
		OSDLOG(ERROR,  "AccountInfoFileManager::get_file_name Invalid File Path   "<<path);
                return "";

	}
}

}
