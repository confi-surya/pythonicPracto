/*
 * =====================================================================================
 *
 *       Filename:  data_block_writer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/06/2014 11:20:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <string.h>

#include "containerLibrary/data_block_writer.h"
#include "containerLibrary/data_block_builder.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace containerInfoFile
{

block_buffer_size_tuple DataBlockWriter::get_writable_block(RecordList const * records)
{
	uint64_t block_size = records->size() * sizeof(DataBlockBuilder::ObjectRecord_);
	char * buffer = new char[block_size];

	if(block_size)
	{
		OSDLOG(DEBUG, "coverting data block for block size "<<block_size);
		DataBlockBuilder::ObjectRecord_ *data_record = reinterpret_cast<DataBlockBuilder::ObjectRecord_ *>(buffer);
		memset(buffer, '\0', block_size);
		RecordList::const_iterator it = records->begin();
		for(; it != records->end(); it++)
		{
			recordStructure::ObjectRecord const & record = (*it).second;
			memcpy(data_record->name, record.name.c_str(), record.name.length());
			memcpy(data_record->content_type, record.get_content_type().c_str(), record.get_content_type().length());
			memcpy(data_record->etag, record.get_etag().c_str(), record.get_etag().length());
			data_record->row_id = record.get_row_id();
			memcpy(
				data_record->created_at,
				record.get_creation_time().c_str(),
				record.get_creation_time().length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : record.get_creation_time().length());
			data_record->size = record.get_size();
			data_record->deleted = record.get_deleted_flag();
			data_record++;
		}
	}

	return boost::make_tuple(buffer, block_size);
}

}

