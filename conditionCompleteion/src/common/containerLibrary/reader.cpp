#include "containerLibrary/reader.h"
#include "libraryUtils/object_storage.h"
#include "libraryUtils/object_storage_exception.h"
#include <iostream>


#define READ_BLOCK(buffer, size, offset) \
	try \
	{ \
		this->file->read(buffer, size, offset); \
	} \
	catch(OSDException::ReadFileException e) \
	{ \
		throw e; \
	}

namespace containerInfoFile
{

Reader::Reader(boost::shared_ptr<FileHandler>  file, 
			boost::shared_ptr<IndexBlockBuilder> index_block_reader,
			boost::shared_ptr<TrailerAndStatBlockBuilder> trailer_stat_block_reader):
			file(file), 
			index_block_reader(index_block_reader),
			trailer_stat_block_reader(trailer_stat_block_reader)
{
	;
}

void Reader::reopen()
{
	OSDLOG(DEBUG, "calling Reader::reopen for file "<<file);
	this->file->reopen();
	this->trailer.reset();
}

boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, \
			boost::shared_ptr<containerInfoFile::Trailer> > Reader::read_stat_trailer_block()
{
	OSDLOG(DEBUG, "calling Reader::read_stat_trailer_block for file "<<file);
	uint64_t buffer_size = sizeof(TrailerAndStatBlockBuilder::StatBlock_) + sizeof(TrailerAndStatBlockBuilder::Trailer_);
	std::vector<char> buffer(buffer_size);

	this->file->read_end(&buffer[0], buffer_size, buffer_size);

	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, \
	                        boost::shared_ptr<containerInfoFile::Trailer> > block_tuple = 
				this->trailer_stat_block_reader->create_trailer_and_stat(&buffer[0]);

	this->trailer = block_tuple.get<1>();
	return block_tuple;
}

boost::shared_ptr<Trailer> Reader::get_trailer()
{
	OSDLOG(DEBUG, "calling Reader::get_trailer for file "<<file);
	if (this->trailer)
	{
		OSDLOG(DEBUG, "calling Reader::get_trailer, trailer already read for file "<<file);
		return this->trailer;
	}
	this->read_stat_trailer_block();
	return this->trailer;
}

// Function to read the index section corresponding to Version 0 file and later translating it to the current version.
void Reader::read_version_0_index_block(std::vector<IndexBlockBuilder::IndexRecord>& buffer, uint32_t start, uint32_t count)
{
	OSDLOG(DEBUG, "Reading Version 0 file and later translating it to the current version. ");
	// Read from file into a buffer composed of IndexRecord_V0 structures
	std::vector<IndexBlockBuilder::IndexRecord_V0> buffer_v0;
	buffer_v0.resize(buffer.size());
	uint32_t size = sizeof(IndexBlockBuilder::IndexRecord_V0) * count;
	uint64_t offset = trailer->index_offset + sizeof(IndexBlockBuilder::IndexRecord_V0) * start;
	this->file->read(reinterpret_cast<char *>(&buffer_v0[0]), size, offset);
	// Iterate over Version 0 buffer_v0 & populate the current format buffer. 
	for (size_t i = 0; i < buffer_v0.size(); ++i) {
		memcpy(buffer[i].hash, buffer_v0[i].hash, sizeof(buffer_v0[i].hash));
		buffer[i].file_state = buffer_v0[i].file_state;
		buffer[i].size = 0; // Populate size as 0 now, will update the actual size later during compaction
	}
	
}

// Functionality to read the index block which also handles the version related complexity too.
// Allows the passing functions / classes to work on IndexRecord only irrespective of the file version.
// NOTE: Always have the IndexBlockBuilder::IndexRecord in accordance with the currently existing latest file format.
// If there are modifications to the index section in the future, then add translation logic from the earlier version
// to the current version. See read_version_0_index_block for reference.
void Reader::read_index_block(std::vector<IndexBlockBuilder::IndexRecord>& buffer, uint32_t start, uint32_t count)
{
	OSDLOG(DEBUG, "In Reader::read_index_block for file, with start and count "<<file<<" "<<start<<" "<<count);
	boost::shared_ptr<Trailer> trailer = this->get_trailer();
	
	switch (trailer->version) {
		// Version 0 file
		case 0 : {
			this->read_version_0_index_block(buffer, start, count);
                        break;
		}
		
		// Current supported version
		case CONTAINER_FILE_SUPPORTED_VERSION : {
			uint32_t size = sizeof(IndexBlockBuilder::IndexRecord) * count;
			uint64_t offset = trailer->index_offset + sizeof(IndexBlockBuilder::IndexRecord) * start;
			this->file->read(reinterpret_cast<char *>(&buffer[0]), size, offset);
                        break;
		}

                default :
			OSDLOG(DEBUG, "In Reader::read_index_block default case, should not have reached here.");
	}

}

// Removed this version of read_index_block & replaced it with the above 1 which handles the version related complexity too.
/*
void Reader::read_index_block(void *buffer, uint32_t start, uint32_t count)
{
	OSDLOG(DEBUG, "calling Reader::read_index_block for file, with start and count "<<file<<" "<<start<<" "<<count);
	boost::shared_ptr<Trailer> trailer = this->get_trailer();

	uint32_t size = sizeof(IndexBlockBuilder::IndexRecord) * count;
	uint64_t offset = trailer->index_offset + sizeof(IndexBlockBuilder::IndexRecord) * start;
	this->file->read(reinterpret_cast<char *>(buffer), size, offset);
}*/

void Reader::read_unsorted_index_block(std::vector<IndexBlockBuilder::IndexRecord> & buffer)
{
	OSDLOG(DEBUG, "calling Reader::read_unsorted_index_block for file "<<file);
	boost::shared_ptr<Trailer> trailer = this->get_trailer();
	buffer.resize(trailer->unsorted_record);
	this->read_index_block(buffer, trailer->sorted_record, trailer->unsorted_record);
}

void Reader::read_data_block(void * buffer, uint32_t start, uint32_t count)
{
	OSDLOG(DEBUG, "calling Reader::read_data_block for file, with start and count "<<file<<" "<<start<<" "<<count);
	boost::shared_ptr<Trailer> trailer = this->get_trailer();
	uint32_t size = sizeof(DataBlockBuilder::ObjectRecord_) * count;
	uint64_t offset = trailer->data_offset + sizeof(DataBlockBuilder::ObjectRecord_) * start;
	this->file->read(reinterpret_cast<char *>(buffer), size, offset);
}

void Reader::read_unsorted_data_block(std::vector<DataBlockBuilder::ObjectRecord_> & buffer)
{
	OSDLOG(DEBUG, "calling Reader::read_unsorted_data_block for file "<<file);
	boost::shared_ptr<Trailer> trailer = this->get_trailer();
	buffer.resize(trailer->unsorted_record);
	this->read_data_block(&buffer[0], trailer->sorted_record, trailer->unsorted_record);
}

void Reader::read_data_index_block(std::vector<DataBlockBuilder::DataIndexRecord> & diRecords)
{
	OSDLOG(DEBUG, "calling Reader::read_data_index_block for file "<<file);
	boost::shared_ptr<Trailer> trailer = this->get_trailer();
	uint32_t num_record = trailer->unsorted_record / objectStorage::DATA_INDEX_INTERVAL;
	if (num_record)
	{
		diRecords.resize(num_record);
		this->file->read(reinterpret_cast<char *>(&diRecords[0]),
				num_record * sizeof(DataBlockBuilder::DataIndexRecord),
				trailer->data_index_offset);
	}
}


Reader::~Reader()
{
	;
}

}
