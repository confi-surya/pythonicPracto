#include <cstring>
#include "accountLibrary/static_data_block_builder.h"

namespace accountInfoFile
{
//UNCOVERED_CODE_BEGIN: 
boost::shared_ptr<StaticDataBlock> StaticDataBlockBuilder::create_block_from_memory(char *buff, uint64_t size)
{
	char *buffer = new char[size];
	memset(buffer, '\0', size);
	memcpy(buffer, buff, size);
	boost::shared_ptr<StaticDataBlock> static_data_block(new StaticDataBlock);
	uint64_t no_of_records = size/sizeof(StaticDataBlockBuilder::StaticBlock_);

	StaticDataBlockBuilder::StaticBlock_ *static_block = reinterpret_cast<StaticDataBlockBuilder::StaticBlock_*>(buffer);

	for(uint64_t i = 0; i < no_of_records; i++)
	{
		static_data_block->add(static_block->ROWID, static_block->name);
		static_block++;
	}

	delete[] buffer;
	return static_data_block;
}
 //UNCOVERED_CODE_END
}
