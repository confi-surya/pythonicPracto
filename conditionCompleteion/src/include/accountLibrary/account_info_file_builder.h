#ifndef __ACCOUNT_INFO_FILE_BUILDER_H__
#define __ACCOUNT_INFO_FILE_BUILDER_H__

#include <boost/filesystem.hpp>
#include "libraryUtils/file_handler.h"
#include "account_info_file.h"

namespace accountInfoFile
{

class AccountInfoFileBuilder
{

public:
	AccountInfoFileBuilder();
		boost::shared_ptr<AccountInfoFile> create_account_for_create(
			std::string path, boost::shared_ptr<lockManager::Mutex> mutex);
		boost::shared_ptr<AccountInfoFile> create_account_for_modify(
			std::string path, boost::shared_ptr<lockManager::Mutex> mutex);
	boost::shared_ptr<AccountInfoFile> create_account_for_reference(
			std::string path, boost::shared_ptr<lockManager::Mutex> mutex);
	bool remove_account_file(std::string path);

private:
	boost::shared_ptr<Writer> create_writer(boost::shared_ptr<FileHandler> file);
	boost::shared_ptr<Reader> create_reader(boost::shared_ptr<FileHandler> file);

	boost::shared_ptr<StaticDataBlockWriter> static_data_block_writer;
	boost::shared_ptr<StaticDataBlockBuilder> static_data_block_reader;
	boost::shared_ptr<NonStaticDataBlockWriter> non_static_data_block_writer;
	boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader;
	boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_writer;
	boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_reader;
};

}

#endif
