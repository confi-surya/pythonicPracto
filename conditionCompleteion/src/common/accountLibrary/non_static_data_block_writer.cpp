#include "accountLibrary/non_static_data_block_writer.h"
#include "accountLibrary/non_static_data_block_builder.h"
#include <iostream>
#include <cstring>

namespace accountInfoFile
{

block_buffer_size_tuple NonStaticDataBlockWriter::get_writable_block(boost::shared_ptr<NonStaticDataBlock> non_static_data_block)
{
	uint64_t size = non_static_data_block->get_size();
	char * buffer = new char[size * sizeof(NonStaticDataBlockBuilder::NonStaticBlock_)];
	
	char * nextMarker = buffer;

	std::vector<NonStaticDataBlock::NonStaticDataBlockRecord>::iterator it = non_static_data_block->get_iterator();

	NonStaticDataBlockBuilder::NonStaticBlock_ *non_static_block = new NonStaticDataBlockBuilder::NonStaticBlock_;

	for(uint64_t i = 0; i < size; i++)
	{
		memset(reinterpret_cast<char*>(non_static_block), '\0', sizeof(NonStaticDataBlockBuilder::NonStaticBlock_));

		NonStaticDataBlock::NonStaticDataBlockRecord record = *it++;
		memcpy(
			non_static_block->hash,
			record.hash.c_str(),
			record.hash.length() > HASH_LENGTH ? HASH_LENGTH : record.hash.length());
		memcpy(
			non_static_block->put_timestamp,
			record.put_timestamp.c_str(),
			record.put_timestamp.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : record.put_timestamp.length());
		memcpy(
			non_static_block->delete_timestamp,
			record.delete_timestamp.c_str(),
			record.delete_timestamp.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : record.delete_timestamp.length());
		non_static_block->object_count = record.object_count;
		non_static_block->bytes_used = record.bytes_used;
		non_static_block->deleted = record.deleted;
		
		memcpy(nextMarker, reinterpret_cast<char*>(non_static_block), sizeof(NonStaticDataBlockBuilder::NonStaticBlock_));
		nextMarker += sizeof(NonStaticDataBlockBuilder::NonStaticBlock_);
	}

	delete non_static_block;
	return boost::make_tuple(buffer, size * sizeof(NonStaticDataBlockBuilder::NonStaticBlock_));
}


/* TODO: remove map implementation
block_buffer_size_tuple NonStaticDataBlockWriter::get_writable_map_block(boost::shared_ptr<NonStaticDataBlock> non_static_data_block)
{
	uint64_t block_size = non_static_data_block->get_map_size()*sizeof(NonStaticDataBlockBuilder::NonStaticBlock_);

		char * buffer = new char[block_size + sizeof(NonStaticDataBlockBuilder::NonStaticMetaData_)];
	memset(buffer, '\0', block_size + sizeof(NonStaticDataBlockBuilder::NonStaticMetaData_));

		NonStaticDataBlockBuilder::NonStaticMetaData_ metadata;
		metadata.size = block_size;

		memcpy(buffer, reinterpret_cast<char*>(&metadata), sizeof(uint64_t));
		char * nextMarker = buffer;
		nextMarker += sizeof(NonStaticDataBlockBuilder::NonStaticMetaData_);

		std::map<std::string, NonStaticDataBlock::NonStaticDataBlockRecord>::iterator it = non_static_data_block->get_map_iterator();
		uint64_t size = non_static_data_block->get_map_size();

		NonStaticDataBlockBuilder::NonStaticBlock_ *non_static_block = new NonStaticDataBlockBuilder::NonStaticBlock_;

		for(uint64_t i = 0; i < size; i++)
		{
		memset(reinterpret_cast<char*>(non_static_block), '\0', sizeof(NonStaticDataBlockBuilder::NonStaticBlock_));

				NonStaticDataBlock::NonStaticDataBlockRecord record = it->second;
		it++;
		//TODO: use length() only after implementation of hash generation function, remove length()+1 ()
				memcpy(non_static_block->hash, record.hash.c_str(), record.hash.length()+1);
				non_static_block->put_timestamp = record.put_timestamp;
				non_static_block->delete_timestamp = record.delete_timestamp;
				non_static_block->object_count = record.object_count;
				non_static_block->bytes_used = record.bytes_used;
				non_static_block->deleted = record.deleted;

				memcpy(nextMarker, reinterpret_cast<char*>(non_static_block),sizeof(NonStaticDataBlockBuilder::NonStaticBlock_));
				nextMarker += sizeof(NonStaticDataBlockBuilder::NonStaticBlock_);
		}

		delete non_static_block;
		return boost::make_tuple(buffer, block_size + sizeof(NonStaticDataBlockBuilder::NonStaticMetaData_));
}
*/

}
