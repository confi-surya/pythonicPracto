#ifndef __WRITER_H__
#define __WRITER_H__

#include <boost/shared_ptr.hpp>

#include "libraryUtils/file_handler.h"
#include "status.h"
#include "static_data_block_writer.h"
#include "non_static_data_block_writer.h"
#include "non_static_data_block_builder.h"
#include "trailer_stat_block_writer.h"
#include "trailer_stat_block_builder.h"

namespace accountInfoFile
{

using fileHandler::AppendWriter;

class Writer
{

public:
	Writer(boost::shared_ptr<fileHandler::FileHandler>  file,
		boost::shared_ptr<StaticDataBlockWriter> static_data_block_writer,
		boost::shared_ptr<NonStaticDataBlockWriter> non_static_data_block_writer,
		boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader,
		boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_block_writer,
		boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_block_reader);

	void write_file(
		boost::shared_ptr<recordStructure::AccountStat> account_stat,
		boost::shared_ptr<TrailerBlock> trailer);

	void write_non_static_data_block(boost::shared_ptr<NonStaticDataBlock> non_static_data_block,
					boost::shared_ptr<TrailerBlock> trailer,
					uint64_t non_static_static_position);

	void write_non_static_data_block(void *buffer, boost::shared_ptr<TrailerBlock> trailer,
			uint64_t start, uint64_t count);

	void write_stat_trailer_block(boost::shared_ptr<recordStructure::AccountStat> account_stat,
					boost::shared_ptr<TrailerBlock> trailer);

	void write_static_data_block(boost::shared_ptr<StaticDataBlock> static_data_block, 
					boost::shared_ptr<NonStaticDataBlock> non_static_data_block,
					boost::shared_ptr<recordStructure::AccountStat> account_stat,
					boost::shared_ptr<TrailerBlock> trailer);
	void write_record(StaticDataBlockBuilder::StaticBlock_ const * static_record,
						NonStaticDataBlockBuilder::NonStaticBlock_ const * non_static_record);
		void flush_record();
		void begin_compaction(boost::shared_ptr<TrailerBlock> trailer, uint64_t estimated_size);
		void finish_compaction();

private:
	boost::shared_ptr<fileHandler::FileHandler> file;
	boost::shared_ptr<StaticDataBlockWriter> static_data_block_writer;
	boost::shared_ptr<NonStaticDataBlockWriter> non_static_data_block_writer;
	boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader;
	boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_block_writer;
	boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_block_reader;

	boost::shared_ptr<AppendWriter> static_append_writer;
	boost::shared_ptr<AppendWriter> non_static_append_writer;
};

}

#endif
