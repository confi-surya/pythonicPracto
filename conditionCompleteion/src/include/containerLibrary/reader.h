#ifndef __READER_H__
#define __READER_H__

#include <boost/shared_ptr.hpp>

#include "libraryUtils/record_structure.h"
#include "libraryUtils/file_handler.h"
#include "data_block_builder.h"
#include "trailer_stat_block_builder.h"

namespace containerInfoFile
{

class ContainerFile;
using fileHandler::FileHandler;

class Reader
{
public:
	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, \
			boost::shared_ptr<containerInfoFile::Trailer> > read_stat_trailer_block();
	boost::shared_ptr<Trailer> get_trailer();
	void reopen();

	Reader(boost::shared_ptr<FileHandler>  file, 
		boost::shared_ptr<IndexBlockBuilder> index_block_reader,
		boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_block_reader);
	~Reader();

	void read_data_block(void *buffer, uint32_t start, uint32_t count);
	void read_unsorted_data_block(std::vector<DataBlockBuilder::ObjectRecord_> & buffer);
	void read_index_block(void *buffer, uint32_t start, uint32_t count);
	void read_index_block(std::vector<IndexBlockBuilder::IndexRecord> & buffer, uint32_t start, uint32_t count);
	void read_unsorted_index_block(std::vector<IndexBlockBuilder::IndexRecord> & buffer);
	void read_data_index_block(std::vector<DataBlockBuilder::DataIndexRecord> & diRecords);

private:
	void read_version_0_index_block(std::vector<IndexBlockBuilder::IndexRecord> & buffer, uint32_t start, uint32_t count);
	boost::shared_ptr<FileHandler> file;
	boost::shared_ptr<IndexBlockBuilder> index_block_reader;
	boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_block_reader;
	boost::shared_ptr<containerInfoFile::Trailer> trailer;
};

}

#endif
