#ifndef __TRAILER_BLOCK_H__
#define __TRAILER_BLOCK_H__

#include <boost/shared_ptr.hpp>
#include "libraryUtils/object_storage.h"
#include "libraryUtils/record_structure.h"
#include "data_block_builder.h"
#include "index_block_builder.h"
#include "config.h"

namespace containerInfoFile
{
using recordStructure::JournalOffset;

class Trailer
{
public:
	uint64_t sorted_record;
	uint64_t unsorted_record;
	uint64_t index_offset;
	uint64_t data_index_offset;
	uint64_t data_offset;
	uint64_t stat_offset;
	JournalOffset journal_offset;
	uint64_t version;

	Trailer(uint64_t sorted_record, uint64_t unsorted_record,
			uint64_t index_offset, uint64_t data_index_offset,
			uint64_t data_offset, uint64_t stat_offset,
			uint64_t jouanrl_file_id, int64_t journal_offset, uint64_t version):
			sorted_record(sorted_record), unsorted_record(unsorted_record), 
			index_offset(index_offset), data_index_offset(data_index_offset),
			data_offset(data_offset), stat_offset(stat_offset), version(version)
	{
		this->journal_offset = std::make_pair(jouanrl_file_id, journal_offset);
	}

	Trailer(uint64_t num_record, uint64_t version = CONTAINER_FILE_SUPPORTED_VERSION)
		: sorted_record(0), unsorted_record(0), version(version)
	{
		num_record = ((num_record + 10000 - 1) / 10000) * 10000 + 10000; // TODO remove magic number
		this->index_offset = 0;
		this->data_index_offset = this->index_offset + num_record * sizeof(IndexBlockBuilder::IndexRecord);
		this->data_offset = this->data_index_offset + sizeof(DataBlockBuilder::DataIndexRecord) * num_record / objectStorage::DATA_INDEX_INTERVAL; // TODO remove magic number
		this->stat_offset = this->data_offset + sizeof(DataBlockBuilder::ObjectRecord_) * num_record;
	}

	Trailer():sorted_record(0), unsorted_record(0), index_offset(0), data_index_offset(0), data_offset(0), stat_offset(0)
	{
		this->version = CONTAINER_FILE_SUPPORTED_VERSION;
	}
};

}

#endif
