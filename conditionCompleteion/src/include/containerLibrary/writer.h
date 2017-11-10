#ifndef __WRITER_H__
#define __WRITER_H__

#include <boost/shared_ptr.hpp>

//#include "containerLibrary/containerInfoFile.h"
#include "libraryUtils/file_handler.h"
#include "trailer_stat_block_writer.h"
#include "data_block_writer.h"
#include "index_block_writer.h"
#include "trailer_stat_block_builder.h"
#include "data_block_iterator.h"
#include "memory_writer.h"

namespace containerInfoFile
{

using fileHandler::FileHandler;
using fileHandler::AppendWriter;
using container_library::RecordList;

class Writer
{
public:
	Writer(boost::shared_ptr<FileHandler>  file, 
				boost::shared_ptr<DataBlockWriter> data_block_writer,
				boost::shared_ptr<IndexBlockWriter> index_block_writer,
				boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_block_writer,
				boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_block_builder);
	void write_file(
			RecordList * data, 
			boost::shared_ptr<IndexBlock> index, 
			boost::shared_ptr<recordStructure::ContainerStat> container_stat,
			boost::shared_ptr<Trailer> trailer);

	void write_file(
			boost::shared_ptr<recordStructure::ContainerStat> container_stat,
			boost::shared_ptr<Trailer> trailer);

	void write_data_index_block(boost::shared_ptr<Trailer> trailer,
			std::vector<DataBlockBuilder::DataIndexRecord> & diRecords);
	void write_record(DataBlockBuilder::ObjectRecord_ const * data,
			IndexBlockBuilder::IndexRecord const * index);
	void flush_record();
	void begin_compaction(boost::shared_ptr<Trailer> trailer, uint64_t estimated_size);
	void finish_compaction();

private:
	boost::shared_ptr<FileHandler> file;
	boost::shared_ptr<DataBlockWriter> data_block_writer;
	boost::shared_ptr<IndexBlockWriter> index_block_writer;
	boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_block_writer;
	boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_block_builder;
	boost::shared_ptr<containerInfoFile::Trailer> trailer;
	boost::shared_ptr<recordStructure::ContainerStat> stat;

	boost::shared_ptr<AppendWriter> data_append_writer;
	boost::shared_ptr<AppendWriter> index_append_writer;
};

}

#endif
