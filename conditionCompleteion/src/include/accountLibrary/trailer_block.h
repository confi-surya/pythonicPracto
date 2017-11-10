#ifndef __TRAILER_BLOCK_H__
#define __TRAILER_BLOCK_H__

#include <boost/shared_ptr.hpp>

#include "static_data_block_builder.h"
#include "non_static_data_block_builder.h"

namespace accountInfoFile
{

class TrailerBlock
{

public:
	uint64_t sorted_record;
	uint64_t unsorted_record;
	uint64_t static_data_block_offset;
	uint64_t non_static_data_block_offset;
	uint64_t stat_block_offset;

	TrailerBlock(uint64_t sorted_record, uint64_t unsorted_record, uint64_t static_data_block_offset, 
			uint64_t non_static_data_block_offset, 	uint64_t stat_block_offset):
			sorted_record(sorted_record), unsorted_record(unsorted_record),
			static_data_block_offset(static_data_block_offset), 
			non_static_data_block_offset(non_static_data_block_offset), 
			stat_block_offset(stat_block_offset)
	{
		;
	}

	TrailerBlock():sorted_record(0), unsorted_record(0), static_data_block_offset(0), 
			non_static_data_block_offset(0), stat_block_offset(0) 
	{
		;
	}

	TrailerBlock(uint64_t num_record)
				: sorted_record(0), unsorted_record(0)
	{
		num_record = ((num_record + 10000 - 1) / 10000) * 10000 + 10000; // TODO remove magic number
	this->static_data_block_offset = 0;
	this->non_static_data_block_offset = this->static_data_block_offset + num_record * 
	sizeof(StaticDataBlockBuilder::StaticBlock_);
	this->stat_block_offset = this->non_static_data_block_offset + 
				sizeof(NonStaticDataBlockBuilder::NonStaticBlock_) * num_record;
	}
};

}

#endif
