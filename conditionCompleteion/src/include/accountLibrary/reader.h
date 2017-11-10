#ifndef __READER_H__
#define __READER_H__

#include <boost/shared_ptr.hpp>

#include "libraryUtils/file_handler.h"
#include "static_data_block_builder.h"
#include "non_static_data_block_builder.h"
#include "trailer_stat_block_builder.h"
#include "record_structure.h"

namespace accountInfoFile
{

class AccountFile;
using fileHandler::FileHandler;

class Reader
{
public:
	Reader(boost::shared_ptr<FileHandler>  file,
		boost::shared_ptr<StaticDataBlockBuilder> static_data_block_reader,
		boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader,
		boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_block_reader);
	boost::tuple<boost::shared_ptr<recordStructure::AccountStat>, 
						boost::shared_ptr<TrailerBlock> > read_stat_trailer_block();
	boost::shared_ptr<TrailerBlock> get_trailer();
	void reopen();

	void read_static_data_block(void *buffer, uint64_t start, uint64_t count);
	void read_unsorted_static_data_block(std::vector<StaticDataBlockBuilder::StaticBlock_> & buffer);
	boost::shared_ptr<NonStaticDataBlock> read_non_static_data_block(uint64_t start, uint64_t count);
	void read_non_static_data_block(void *buffer, uint64_t start, uint64_t count);
	void read_unsorted_non_static_data_block(std::vector<NonStaticDataBlockBuilder::NonStaticBlock_> & buffer);

		//boost::shared_ptr<DataIndexBlock>read_data_index_block();

private:

	boost::shared_ptr<FileHandler> file;
	boost::shared_ptr<StaticDataBlockBuilder> static_data_block_reader;
	boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader;
	boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_block_reader;
	boost::shared_ptr<TrailerBlock> trailer;
};

}

#endif

