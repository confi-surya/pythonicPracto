/*
 * =====================================================================================
 *
 *       Filename:  index_block_writer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/30/2014 12:38:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "containerLibrary/index_block_writer.h"
#include "containerLibrary/index_block_builder.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace containerInfoFile
{

block_buffer_size_tuple IndexBlockWriter::get_writable_block(boost::shared_ptr<IndexBlock> index)
{
	uint64_t block_size = index->get_size()*sizeof(IndexBlockBuilder::IndexRecord);
	char * buffer = new char[block_size];

	if(block_size)
	{
		OSDLOG(DEBUG, "creating index block buffer for block size "<<block_size);
		char * nextMarker = buffer;
		std::vector<IndexBlock::IndexRecord>::iterator it = index->get_iterator();;
		uint64_t size = index->get_size();
		IndexBlockBuilder::IndexRecord *index_record = new IndexBlockBuilder::IndexRecord;
		for(uint64_t i = 0; i < size; i++)
		{
			memset(reinterpret_cast<char*>(index_record), '\0', sizeof(IndexBlockBuilder::IndexRecord));
			IndexBlock::IndexRecord record = *it++;
			memcpy(
				index_record->hash,
				record.hash.c_str(),
				record.hash.length()
				);
			index_record->file_state = record.file_state;
			index_record->size = record.size;
			memcpy(nextMarker, reinterpret_cast<char*>(index_record), sizeof(IndexBlockBuilder::IndexRecord));
			nextMarker += sizeof(IndexBlockBuilder::IndexRecord);
		}
		
		delete index_record;
	}
	return boost::make_tuple(buffer, block_size);
}

}
