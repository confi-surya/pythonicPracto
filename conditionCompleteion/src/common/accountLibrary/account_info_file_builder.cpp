#include "accountLibrary/account_info_file_builder.h"
#include <vector>
#include <string>

namespace accountInfoFile
{

AccountInfoFileBuilder::AccountInfoFileBuilder()
{
	OSDLOG(DEBUG, "AccountInfoFileBuilder::AccountInfoFileBuilder() called");
	this->static_data_block_writer.reset(new StaticDataBlockWriter);
	this->static_data_block_reader.reset(new StaticDataBlockBuilder);
	this->non_static_data_block_writer.reset(new NonStaticDataBlockWriter);
	this->non_static_data_block_reader.reset(new NonStaticDataBlockBuilder);
	this->trailer_stat_writer.reset(new TrailerStatBlockWriter);
	this->trailer_stat_reader.reset(new TrailerStatBlockBuilder);
}
boost::shared_ptr<AccountInfoFile> AccountInfoFileBuilder::create_account_for_reference(
						std::string path, boost::shared_ptr<lockManager::Mutex> mutex)
{
	OSDLOG(DEBUG, "create_account_for_reference called for "<<path);
	boost::shared_ptr<FileHandler> file(new FileHandler(path, fileHandler::OPEN_FOR_READ_WRITE));
	boost::shared_ptr<AccountInfoFile> account_infoFile(new AccountInfoFile(
				this->create_reader(file), this->create_writer(file), mutex));
	return account_infoFile;
}

boost::shared_ptr<AccountInfoFile> AccountInfoFileBuilder::create_account_for_create(
				std::string path, boost::shared_ptr<lockManager::Mutex> mutex)
{
	OSDLOG(DEBUG, "create_account_for_create called for "<<path);
	boost::shared_ptr<FileHandler> file(new FileHandler(path, fileHandler::OPEN_FOR_CREATE));
	boost::shared_ptr<AccountInfoFile> account_infoFile(new AccountInfoFile(
				this->create_reader(file), this->create_writer(file), mutex));
	return account_infoFile;
}


boost::shared_ptr<AccountInfoFile> AccountInfoFileBuilder::create_account_for_modify(
			std::string path, boost::shared_ptr<lockManager::Mutex> mutex)
{
	OSDLOG(DEBUG, "create_account_for_modify called for "<<path);
	boost::shared_ptr<FileHandler> file_for_read(new FileHandler(path, fileHandler::OPEN_FOR_READ));
	boost::shared_ptr<FileHandler> file_for_write(new FileHandler(path, fileHandler::OPEN_FOR_WRITE));
	boost::shared_ptr<AccountInfoFile> account_infoFile(new AccountInfoFile(
				this->create_reader(file_for_read), this->create_writer(file_for_write), mutex));
	return account_infoFile;
	
}

boost::shared_ptr<Writer> AccountInfoFileBuilder::create_writer(boost::shared_ptr<FileHandler> file)
{
	boost::shared_ptr<Writer> writer(new Writer(file,
						this->static_data_block_writer,
						this->non_static_data_block_writer,
						this->non_static_data_block_reader,
						this->trailer_stat_writer,
						this->trailer_stat_reader));
	return writer;
}

boost::shared_ptr<Reader> AccountInfoFileBuilder::create_reader(boost::shared_ptr<FileHandler> file)
{
	boost::shared_ptr<Reader> reader(new Reader(file,
						this->static_data_block_reader,
						this->non_static_data_block_reader, 
						this->trailer_stat_reader));
	return reader;
}


}
