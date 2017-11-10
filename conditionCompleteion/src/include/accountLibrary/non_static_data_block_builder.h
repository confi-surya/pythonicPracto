#ifndef __NON_STATIC_DATA_BLOCK_BUILDER_H__
#define __NON_STATIC_DATA_BLOCK_BUILDER_H__

#include <boost/shared_ptr.hpp>

#include "non_static_data_block.h"
#include "config.h"

namespace accountInfoFile
{

class NonStaticDataBlockBuilder
{
public:
	struct NonStaticBlock_
	{
		char hash[HASH_LENGTH + 1];
		char put_timestamp[TIMESTAMP_LENGTH + 1];
		char delete_timestamp[TIMESTAMP_LENGTH + 1];
		uint64_t object_count;
		uint64_t bytes_used;
		bool deleted;
	};

	boost::shared_ptr<NonStaticDataBlock> create_block_from_memory(char *buffer, uint64_t no_of_records);
};

}

#endif
