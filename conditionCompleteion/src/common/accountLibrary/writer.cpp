#include "accountLibrary/writer.h"
#include "libraryUtils/object_storage_exception.h"

#include <iostream>
namespace accountInfoFile
{

Writer::Writer(boost::shared_ptr<fileHandler::FileHandler>  file,
		boost::shared_ptr<StaticDataBlockWriter> static_data_block_writer,
		boost::shared_ptr<NonStaticDataBlockWriter> non_static_data_block_writer,
		boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader,
		boost::shared_ptr<TrailerStatBlockWriter> trailer_stat_block_writer,
		boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_block_reader):
		file(file),
		static_data_block_writer(static_data_block_writer),
		non_static_data_block_writer(non_static_data_block_writer),
		non_static_data_block_reader(non_static_data_block_reader),
		trailer_stat_block_writer(trailer_stat_block_writer),
		trailer_stat_block_reader(trailer_stat_block_reader)
{

	;
}


void Writer::write_static_data_block(boost::shared_ptr<StaticDataBlock> static_data_block,
					boost::shared_ptr<NonStaticDataBlock> non_static_data_block,
					boost::shared_ptr<recordStructure::AccountStat> stat,
					boost::shared_ptr<TrailerBlock> trailer)
{
	uint64_t static_record_offset = trailer->static_data_block_offset + (sizeof(StaticDataBlockBuilder::StaticBlock_) *
						(trailer->sorted_record + trailer->unsorted_record - static_data_block->get_size()));
	uint64_t non_static_record_offset = trailer->non_static_data_block_offset + 
			(sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * 
			(trailer->sorted_record + trailer->unsorted_record - non_static_data_block->get_size()));

	block_buffer_size_tuple static_data_tuple = this->static_data_block_writer->get_writable_block(static_data_block);
	block_buffer_size_tuple non_static_data_tuple = this->non_static_data_block_writer->get_writable_block(non_static_data_block);
	block_buffer_size_tuple trailer_stat_tuple = this->trailer_stat_block_writer->get_writable_block(stat, trailer);
	
	try
	{
		this->file->write(static_data_tuple.get<0>(), static_data_tuple.get<1>(), static_record_offset); // use old trailer
		this->file->write(non_static_data_tuple.get<0>(), non_static_data_tuple.get<1>(), non_static_record_offset);
		this->file->sync();  //ensure stat and trailer go in one sync
		this->file->write(trailer_stat_tuple.get<0>(), trailer_stat_tuple.get<1>(), trailer->stat_block_offset);
		this->file->sync();
	}
	catch(OSDException::WriteFileException e)
	{
		delete[] static_data_tuple.get<0>();
		delete[] non_static_data_tuple.get<0>();
		delete[] trailer_stat_tuple.get<0>();
		throw e;
	}

	delete[] static_data_tuple.get<0>();
	delete[] non_static_data_tuple.get<0>();
	delete[] trailer_stat_tuple.get<0>();
}
//UNCOVERED_CODE_BEGIN: 
void Writer::write_non_static_data_block(boost::shared_ptr<NonStaticDataBlock> non_static_data_block, boost::shared_ptr<TrailerBlock> trailer, uint64_t non_static_static_position)
{
                uint64_t non_static_record_offset = trailer->non_static_data_block_offset +
                                                (sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * non_static_static_position);

        block_buffer_size_tuple non_static_data_tuple = this->non_static_data_block_writer->get_writable_block(non_static_data_block);

        try
        {
                this->file->write(non_static_data_tuple.get<0>(), non_static_data_tuple.get<1>(), non_static_record_offset);
                this->file->sync();
        }
        catch(OSDException::WriteFileException e)
        {
                delete[] non_static_data_tuple.get<0>();
                throw e;
        }

        delete[] non_static_data_tuple.get<0>();
}
//UNCOVERED_CODE_END
void Writer::write_non_static_data_block(void *buffer, boost::shared_ptr<TrailerBlock> trailer,
		uint64_t start, uint64_t count)
{
	uint64_t size = sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * count;
	uint64_t offset = trailer->non_static_data_block_offset + sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * start;
	this->file->write(reinterpret_cast<char *>(buffer), size, offset);
}


void Writer::write_stat_trailer_block(boost::shared_ptr<recordStructure::AccountStat> stat,
                                        boost::shared_ptr<TrailerBlock> trailer)
{
        block_buffer_size_tuple trailer_stat_tuple = this->trailer_stat_block_writer->get_writable_block(stat, trailer);

        try
        {
                this->file->write(trailer_stat_tuple.get<0>(), trailer_stat_tuple.get<1>(), trailer->stat_block_offset);
                this->file->sync();
        }
        catch(OSDException::WriteFileException e)
	{
                delete[] trailer_stat_tuple.get<0>();
                throw e;
	}
        delete[] trailer_stat_tuple.get<0>();
}


void Writer::write_file(boost::shared_ptr<recordStructure::AccountStat> stat, boost::shared_ptr<TrailerBlock> trailer )
{
	
	block_buffer_size_tuple trailer_stat_tuple = this->trailer_stat_block_writer->get_writable_block(stat, trailer);

	try
	{
		this->file->write(trailer_stat_tuple.get<0>(), trailer_stat_tuple.get<1>(), trailer->stat_block_offset);
		this->file->sync();
	}
	catch(OSDException::WriteFileException e)
	{
		delete[] trailer_stat_tuple.get<0>();
		throw e;
	}
	delete[] trailer_stat_tuple.get<0>();
}

void Writer::write_record(StaticDataBlockBuilder::StaticBlock_ const * static_record,
				NonStaticDataBlockBuilder::NonStaticBlock_ const * non_static_record)
{
	this->static_append_writer->append(static_record, sizeof(StaticDataBlockBuilder::StaticBlock_));
	this->non_static_append_writer->append(non_static_record, sizeof(NonStaticDataBlockBuilder::NonStaticBlock_));
}

void Writer::flush_record()
{
	this->static_append_writer->flush();
	this->non_static_append_writer->flush();
}

void Writer::begin_compaction(boost::shared_ptr<TrailerBlock> trailer, uint64_t max_size)
{
	this->file->open_tmp_file(".compact");
	uint64_t buffer_size = std::min(max_size * sizeof(StaticDataBlockBuilder::StaticBlock_), 5 * 1024 * 1024UL);
	this->static_append_writer.reset(new AppendWriter(this->file,
						trailer->static_data_block_offset, buffer_size));
	buffer_size = std::min(max_size * sizeof(NonStaticDataBlockBuilder::NonStaticBlock_), 5 * 1024 * 1024UL);
	this->non_static_append_writer.reset(new AppendWriter(this->file,
						trailer->non_static_data_block_offset, buffer_size));
}

void Writer::finish_compaction()
{
	this->static_append_writer.reset();
	this->non_static_append_writer.reset();
	this->file->rename_tmp_file(".compact");
	this->file->reopen();
}

}
