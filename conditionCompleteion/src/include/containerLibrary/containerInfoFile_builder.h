#ifndef __CONTAINER_FILE_BUILDER_H__
#define __CONTAINER_FILE_BUILDER_H__

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include "containerInfoFile.h"
#include "libraryUtils/file_handler.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace containerInfoFile
{

using fileHandler::FileHandler;

class ContainerInfoFileBuilder
{

public:
	ContainerInfoFileBuilder();
	boost::shared_ptr<ContainerInfoFile> create_container_for_create(std::string path, 
									boost::shared_ptr<lockManager::Mutex> mutex); 
	boost::shared_ptr<ContainerInfoFile> create_container_for_modify(std::string path, 
									boost::shared_ptr<lockManager::Mutex> mutex);
	boost::shared_ptr<ContainerInfoFile> create_container_for_reference(std::string path, 
									boost::shared_ptr<lockManager::Mutex> mutex);

private:
	boost::shared_ptr<Writer> create_writer(boost::shared_ptr<FileHandler> file);
	boost::shared_ptr<Reader> create_reader(boost::shared_ptr<FileHandler> file);

	boost::shared_ptr<containerInfoFile::DataBlockBuilder> data_block_reader;
        boost::shared_ptr<containerInfoFile::DataBlockWriter> data_block_writer;

        boost::shared_ptr<IndexBlockBuilder> index_block_reader;
        boost::shared_ptr<IndexBlockWriter> index_block_writer;

        boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_reader;
        boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_writer;
};

}

#endif
