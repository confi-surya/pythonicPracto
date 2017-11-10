#include "accountLibrary/non_static_data_block_builder.h"
#include <iostream>

namespace accountInfoFile
{
//UNCOVERED_CODE_BEGIN: 
boost::shared_ptr<NonStaticDataBlock> NonStaticDataBlockBuilder::create_block_from_memory(char *buffer, uint64_t no_of_records)
{
	boost::shared_ptr<NonStaticDataBlock> non_static_data_block(new NonStaticDataBlock);

	NonStaticDataBlockBuilder::NonStaticBlock_ *non_static_block = reinterpret_cast<NonStaticDataBlockBuilder::NonStaticBlock_*>(buffer);
	
	for(uint64_t i = 0; i < no_of_records; i++)
	{
		non_static_data_block->add(
					non_static_block->hash,
					non_static_block->put_timestamp,
					non_static_block->delete_timestamp,
					non_static_block->object_count,
					non_static_block->bytes_used,
					non_static_block->deleted);
		non_static_block++;
	}

	return non_static_data_block;
}
//UNCOVERED_CODE_END
}

