#include "containerLibrary/containerInfoFile_builder.h"

// TODO to much rendundant code clean up
namespace containerInfoFile
{

ContainerInfoFileBuilder::ContainerInfoFileBuilder()
{
	OSDLOG(DEBUG, "ContainerInfoFileBuilder::ContainerInfoFileBuilder called");
	this->data_block_reader.reset(new containerInfoFile::DataBlockBuilder);
	this->data_block_writer.reset(new containerInfoFile::DataBlockWriter);

	this->index_block_reader.reset(new IndexBlockBuilder);
	this->index_block_writer.reset(new IndexBlockWriter);

	this->trailer_stat_reader.reset(new TrailerAndStatBlockBuilder);
	this->trailer_stat_writer.reset(new TrailerStatBlockWriter);
}
boost::shared_ptr<ContainerInfoFile> ContainerInfoFileBuilder::create_container_for_reference(
							std::string path, 
							boost::shared_ptr<lockManager::Mutex> mutex)
{
	OSDLOG(DEBUG, "ContainerInfoFileBuilder::create_container_for_reference called for " << path);
	boost::shared_ptr<FileHandler> file(new FileHandler(path, fileHandler::OPEN_FOR_READ));

	boost::shared_ptr<ContainerInfoFile> containerInfoFile1(new ContainerInfoFile(
			this->create_reader(file), this->create_writer(file), mutex));
	return containerInfoFile1;
}

boost::shared_ptr<ContainerInfoFile> ContainerInfoFileBuilder::create_container_for_modify(
		std::string path,
		boost::shared_ptr<lockManager::Mutex> mutex)
{
	OSDLOG(DEBUG, "ContainerInfoFileBuilder::create_container_for_modify called for " << path);
	boost::shared_ptr<FileHandler> file_for_read(new FileHandler(path, fileHandler::OPEN_FOR_READ));
	boost::shared_ptr<FileHandler> file_for_write(new FileHandler(path, fileHandler::OPEN_FOR_WRITE));

	boost::shared_ptr<ContainerInfoFile> containerInfoFile1(new ContainerInfoFile(
			this->create_reader(file_for_read), this->create_writer(file_for_write), mutex));
	return containerInfoFile1;
}

boost::shared_ptr<ContainerInfoFile> ContainerInfoFileBuilder::create_container_for_create(
		std::string path,
		boost::shared_ptr<lockManager::Mutex> mutex)
{
	OSDLOG(DEBUG, "ContainerInfoFileBuilder::create_container_for_create called for " << path);
	boost::shared_ptr<FileHandler> file(new FileHandler(path, fileHandler::OPEN_FOR_CREATE));
	boost::shared_ptr<ContainerInfoFile> containerInfoFile1(new ContainerInfoFile(
			this->create_reader(file), this->create_writer(file), mutex));
	return containerInfoFile1;
}

boost::shared_ptr<Reader> ContainerInfoFileBuilder::create_reader(boost::shared_ptr<FileHandler> file)
{
	boost::shared_ptr<Reader> reader(new Reader(file, 
			this->index_block_reader, this->trailer_stat_reader));
	return reader;
}

boost::shared_ptr<Writer> ContainerInfoFileBuilder::create_writer(boost::shared_ptr<FileHandler> file)
{
	boost::shared_ptr<Writer> writer(new Writer(file, this->data_block_writer,
			this->index_block_writer, this->trailer_stat_writer, this->trailer_stat_reader));
	return writer;
}

}
