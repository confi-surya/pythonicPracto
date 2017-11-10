#ifndef __STATIC_DATA_BLOCK_BUILDER_H__
#define __STATIC_DATA_BLOCK_BUILDER_H__

#include <boost/shared_ptr.hpp>
#include "config.h"
#include "static_data_block.h"

namespace accountInfoFile
{

//static const uint16_t CONTAINER_NAME_LENGTH = 256;

class StaticDataBlockBuilder
{
public:
	struct StaticBlock_
	{
		uint64_t ROWID;
		char name[CONTAINER_NAME_LENGTH + 1];
	};
	
	boost::shared_ptr<StaticDataBlock> create_block_from_memory(char *buffer, uint64_t size); // buffer size

};

}

#endif
