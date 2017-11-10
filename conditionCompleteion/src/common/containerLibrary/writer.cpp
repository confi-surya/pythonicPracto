#include "cool/debug.h"
#include "containerLibrary/writer.h"
#include "containerLibrary/reader.h"
#include "libraryUtils/object_storage_exception.h"

namespace containerInfoFile
{

Writer::Writer(boost::shared_ptr<FileHandler>  file, 
		boost::shared_ptr<DataBlockWriter> data_block_writer,
		boost::shared_ptr<IndexBlockWriter> index_block_writer,
		boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_block_writer,
		boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_block_builder):
		file(file), 
		data_block_writer(data_block_writer),
		index_block_writer(index_block_writer),
		trailer_stat_block_writer(trailer_stat_block_writer),
		trailer_stat_block_builder(trailer_stat_block_builder)
{
	;
}

void Writer::write_file(RecordList * data,
			boost::shared_ptr<IndexBlock> index, 
			boost::shared_ptr<recordStructure::ContainerStat> stat,
			boost::shared_ptr<Trailer> trailer)
{
	OSDLOG(DEBUG, "Writer::write_file Writing Index, Stat and Trailer for file "<<file);
	uint64_t data_offset = trailer->data_offset + (sizeof(DataBlockBuilder::ObjectRecord_) *
			(trailer->sorted_record + trailer->unsorted_record - data->size()));
	uint64_t index_offset = trailer->index_offset + (sizeof(IndexBlockBuilder::IndexRecord) *
			(trailer->sorted_record + trailer->unsorted_record - index->get_size()));

	block_buffer_size_tuple data_tuple = this->data_block_writer->get_writable_block(data);
	block_buffer_size_tuple index_tuple = this->index_block_writer->get_writable_block(index);
	block_buffer_size_tuple write_block_tuple = this->trailer_stat_block_writer->get_writable_block(stat, trailer);
	//std::cout<<"unsorted record  "<<trailer->unsorted_record<<std::endl;

	// add code to write to tmp file if void block requires renewing
	try
	{
		this->file->write(data_tuple.get<0>(), data_tuple.get<1>(), data_offset);
		this->file->write(index_tuple.get<0>(), index_tuple.get<1>(), index_offset);
		this->file->sync();  //ensure stat and trailer go in one sync
		this->file->write(write_block_tuple.get<0>(), write_block_tuple.get<1>(), trailer->stat_offset);
		this->file->sync();
	}
	catch(OSDException::FileException e)
	{
		OSDLOG(WARNING, "Exception occured while writing Index, Stat and Trailer for file "<<file);
		delete[] data_tuple.get<0>();
		delete[] index_tuple.get<0>();
		delete[] write_block_tuple.get<0>();

		throw e;
	}

	delete[] data_tuple.get<0>();
	delete[] index_tuple.get<0>();
	delete[] write_block_tuple.get<0>();
}

// bug in this code trailer will be corrupted, 
void Writer::write_file(boost::shared_ptr<recordStructure::ContainerStat> stat, boost::shared_ptr<Trailer> trailer)
{

	OSDLOG(DEBUG, "Writer::write_file Writing Stat and Trailer for file "<<file);
	block_buffer_size_tuple write_block_tuple = this->trailer_stat_block_writer->get_writable_block(stat, trailer);

	try
	{
		this->file->write(write_block_tuple.get<0>(), write_block_tuple.get<1>(), trailer->stat_offset);
		this->file->sync();
	}
	catch(OSDException::WriteFileException e)
	{
		OSDLOG(DEBUG, "Exceution occured while writing Stat and Trailer for file "<<file);
		delete[] write_block_tuple.get<0>();
		throw e;
	}

	delete[] write_block_tuple.get<0>();
}

void Writer::write_data_index_block(boost::shared_ptr<Trailer> trailer,
		std::vector<DataBlockBuilder::DataIndexRecord> & diRecords)
{
	OSDLOG(DEBUG, "Writer::write_data_index_block Writing Data Index block for file "<<file);
	this->file->write(reinterpret_cast<char *>(&diRecords[0]),
			diRecords.size() * sizeof(DataBlockBuilder::DataIndexRecord),
			trailer->data_index_offset);
}

void Writer::write_record(DataBlockBuilder::ObjectRecord_ const * data,
		IndexBlockBuilder::IndexRecord const * index)
{
	this->data_append_writer->append(data, sizeof(DataBlockBuilder::ObjectRecord_));
	this->index_append_writer->append(index, sizeof(IndexBlockBuilder::IndexRecord));
}

void Writer::flush_record()
{
	OSDLOG(DEBUG, "Writer::flush_record called for file "<<file);
	this->data_append_writer->flush();
	this->index_append_writer->flush();
}

void Writer::begin_compaction(boost::shared_ptr<Trailer> trailer, uint64_t max_size)
{
	OSDLOG(INFO, "Writer::begin_compaction for file and max size "<<file<<" "<<max_size);
	this->file->open_tmp_file(".compact");
	uint64_t buffer_size = std::min(max_size * sizeof(DataBlockBuilder::ObjectRecord_), 5 * 1024 * 1024UL);
	this->data_append_writer.reset(new AppendWriter(this->file,
			trailer->data_offset, buffer_size));
	buffer_size = std::min(max_size * sizeof(IndexBlockBuilder::IndexRecord), 5 * 1024 * 1024UL);
	this->index_append_writer.reset(new AppendWriter(this->file,
			trailer->index_offset, buffer_size));
}

void Writer::finish_compaction()
{
	OSDLOG(DEBUG, "Writer::finish_compaction for file "<<file);
	this->data_append_writer.reset();
	this->index_append_writer.reset();
	this->file->rename_tmp_file(".compact");
	this->file->reopen();
}

}
