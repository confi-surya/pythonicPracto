#include "accountLibrary/reader.h"
#include "libraryUtils/object_storage_exception.h"

namespace accountInfoFile
{
Reader::Reader(boost::shared_ptr<FileHandler>  file,
		boost::shared_ptr<StaticDataBlockBuilder> static_data_block_reader,
		boost::shared_ptr<NonStaticDataBlockBuilder> non_static_data_block_reader,
		boost::shared_ptr<TrailerStatBlockBuilder> trailer_stat_block_reader):
		file(file),
		static_data_block_reader(static_data_block_reader),
		non_static_data_block_reader(non_static_data_block_reader),
		trailer_stat_block_reader(trailer_stat_block_reader)
{
	;
}

boost::tuple<boost::shared_ptr<recordStructure::AccountStat>, 
						boost::shared_ptr<TrailerBlock> > Reader::read_stat_trailer_block()
{
	uint64_t buffer_size = sizeof(TrailerStatBlockBuilder::StatBlock_) + sizeof(TrailerStatBlockBuilder::TrailerBlock_);
	std::vector<char> buffer(buffer_size);
	this->file->read_end(&buffer[0], buffer_size, buffer_size);
	boost::tuple<boost::shared_ptr<recordStructure::AccountStat>, 
						boost::shared_ptr<TrailerBlock> > block_tuple =
						this->trailer_stat_block_reader->create_trailer_and_stat(&buffer[0]);

	this->trailer = block_tuple.get<1>();
	return block_tuple;
}

boost::shared_ptr<TrailerBlock> Reader::get_trailer()
{
	if (this->trailer)
	{
		return this->trailer;
	}
	this->read_stat_trailer_block();
	return this->trailer;
}

void Reader::read_static_data_block(void *buffer, uint64_t start, uint64_t count)
{
	OSDLOG(DEBUG, "calling Reader::read_static_data_block for file "<<this->file <<" start : "<<start<<" count : "<<count);
	boost::shared_ptr<TrailerBlock> trailer = this->get_trailer();
	uint64_t size = sizeof(StaticDataBlockBuilder::StaticBlock_) * count;
	uint64_t offset = trailer->static_data_block_offset + sizeof(StaticDataBlockBuilder::StaticBlock_) * start;
	this->file->read(reinterpret_cast<char *>(buffer), size, offset);
}

void Reader::read_unsorted_static_data_block(std::vector<StaticDataBlockBuilder::StaticBlock_> & buffer)
{
	OSDLOG(DEBUG, "calling Reader::read_unsorted_static_data_block for file "<<this->file);
	boost::shared_ptr<TrailerBlock> trailer = this->get_trailer();
	buffer.resize(trailer->unsorted_record);
	this->read_static_data_block(&buffer[0], trailer->sorted_record, trailer->unsorted_record);
	//for(std::vector<StaticDataBlockBuilder::StaticBlock_>::iterator it = buffer.begin(); it != buffer.end(); it++)
	  //	  std::cout<<"ni traile static "<<it->ROWID<<" "<<it->name<<std::endl;
}

void Reader::read_non_static_data_block(void *buffer, uint64_t start, uint64_t count)
{
	OSDLOG(DEBUG, "calling Reader::read_non_static_data_block for file "<<this->file <<" start : "<<start<<" count : "<<count);
	boost::shared_ptr<TrailerBlock> trailer = this->get_trailer();
	uint64_t size = sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * count;
	uint64_t offset = trailer->non_static_data_block_offset + sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * start;
	this->file->read(reinterpret_cast<char *>(buffer), size, offset);
}
//UNCOVERED_CODE_BEGIN:
boost::shared_ptr<NonStaticDataBlock> Reader::read_non_static_data_block(uint64_t start, uint64_t count)
{
	boost::shared_ptr<TrailerBlock> trailer = this->get_trailer();
	uint64_t size = sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * count;
	char *buffer = new char[size];
	uint64_t offset = trailer->non_static_data_block_offset + sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * start;
	try
	{
		this->file->read(buffer, size, offset);
	}
	catch(OSDException::ReadFileException e)
	{
		delete[] buffer;
		throw e;
	}

	boost::shared_ptr<NonStaticDataBlock> non_static_block = 
			this->non_static_data_block_reader->create_block_from_memory(buffer, count);
	delete[] buffer;
	return non_static_block;
}
//UNCOVERED_CODE_END
void Reader::read_unsorted_non_static_data_block(std::vector<NonStaticDataBlockBuilder::NonStaticBlock_> & buffer)
{
	boost::shared_ptr<TrailerBlock> trailer = this->get_trailer();
	buffer.resize(trailer->unsorted_record);
	this->read_non_static_data_block(&buffer[0], trailer->sorted_record, trailer->unsorted_record);
//	for(std::vector<NonStaticDataBlockBuilder::NonStaticBlock_>::iterator it = buffer.begin(); it != buffer.end(); it++)
//		std::cout<<"ni traile "<<it->hash<<" "<<it->put_timestamp<<" "<<it->deleted<<std::endl;
}

void Reader::reopen()
{
		this->file->reopen();
		this->trailer.reset();
}

}
