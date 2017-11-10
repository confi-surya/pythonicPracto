#ifndef __INDEX_BLOCK_BUILDER_H__
#define __INDEX_BLOCK_BUILDER_H__

#include <boost/shared_ptr.hpp>

#include "index_block.h"

namespace containerInfoFile
{

class IndexBlockBuilder
{
public:
	struct IndexRecord
	{
		char hash[33];
		uint64_t file_state;
		uint64_t size;
	};

	// Maintaining the previous version of IndexRecord for backward compatibility
	struct IndexRecord_V0
	{
		char hash[33];
		uint64_t file_state;
	};

	boost::shared_ptr<IndexBlock> create_index_block_from_memory(char *buffer, uint64_t number_of_records);

};

}

#endif
