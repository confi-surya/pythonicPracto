/*
 * =====================================================================================
 *
 *       Filename:  index_block_builder.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/30/2014 12:20:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "containerLibrary/index_block_builder.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace containerInfoFile
{
//UNCOVERED_CODE_BEGIN:
boost::shared_ptr<IndexBlock> IndexBlockBuilder::create_index_block_from_memory(char *buffer, uint64_t number_of_records)
{
	boost::shared_ptr<IndexBlock> index(new IndexBlock);
	IndexBlockBuilder::IndexRecord *index_record = reinterpret_cast<IndexBlockBuilder::IndexRecord*>(buffer);

	OSDLOG(DEBUG, "building index block for record size "<<number_of_records);
	for(uint64_t i = 0; i < number_of_records; i++)
	{
		index->add(IndexBlock::IndexRecord(index_record->hash, index_record->file_state, index_record->size));
		index_record++;
	}

	return index;
}
//UNCOVERED_CODE_END

}
