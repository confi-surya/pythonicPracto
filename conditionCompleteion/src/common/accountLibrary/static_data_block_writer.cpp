#include "accountLibrary/static_data_block_writer.h"

#include "accountLibrary/static_data_block_builder.h"

namespace accountInfoFile
{

block_buffer_size_tuple StaticDataBlockWriter::get_writable_block(boost::shared_ptr<StaticDataBlock> static_data_block)
{
	uint64_t block_size = static_data_block->get_size()*sizeof(StaticDataBlockBuilder::StaticBlock_);
	
	char * buffer = new char[block_size];
	memset(buffer, '\0', block_size);

	char * nextMarker = buffer;

	std::vector<StaticDataBlock::StaticDataBlockRecord>::iterator it = static_data_block->get_iterator();
	uint64_t size = static_data_block->get_size();

	StaticDataBlockBuilder::StaticBlock_ *static_block = new StaticDataBlockBuilder::StaticBlock_;

	for(uint64_t i = 0; i < size; i++)
	{
		memset(reinterpret_cast<char*>(static_block), '\0', sizeof(StaticDataBlockBuilder::StaticBlock_));
		StaticDataBlock::StaticDataBlockRecord record = *it++;
		static_block->ROWID = record.ROWID;
		memcpy(
			static_block->name,
			record.name.c_str(),
			record.name.length() > CONTAINER_NAME_LENGTH ? CONTAINER_NAME_LENGTH : record.name.length());
		memcpy(nextMarker, reinterpret_cast<char*>(static_block), sizeof(StaticDataBlockBuilder::StaticBlock_));
		nextMarker += sizeof(StaticDataBlockBuilder::StaticBlock_);
	}

	delete static_block;
	return boost::make_tuple(buffer, block_size);
}

}
