#ifndef __TRAILER_STAT_BLOCK_BUILDER_H__
#define __TRAILER_STAT_BLOCK_BUILDER_H__

#include <boost/shared_ptr.hpp>
 #include <boost/tuple/tuple.hpp>

#include "trailer_block.h"
#include "record_structure.h"
#include "config.h"

namespace accountInfoFile
{
/*
static const uint16_t ACCOUNT_NAME_LENGTH = 256;
static const uint16_t TIMESTAMP_LENGTH = 16;
static const uint16_t HASH_LENGTH = 32;
static const uint16_t ID_LENGTH = 36;
static const uint16_t STATUS_LENGTH = 16;
static 	const uint16_t META_DATA_LENGTH = 32; // metadata is not supported in account library
*/
class TrailerStatBlockBuilder
{

public:
	struct StatBlock_
	{
		char account[ACCOUNT_NAME_LENGTH + 1];
		char created_at[TIMESTAMP_LENGTH + 1];
		char put_timestamp[TIMESTAMP_LENGTH + 1];
		char delete_timestamp[TIMESTAMP_LENGTH + 1];
		uint64_t container_count;
		uint64_t object_count;
		uint64_t bytes_used;
		char hash[HASH_LENGTH + 1];
		char id[ID_LENGTH + 1];
		char status[STATUS_LENGTH + 1];
		char status_changed_at[TIMESTAMP_LENGTH + 1];
		char metadata[META_DATA_LENGTH + 1]; // 30KB metadata( 24KB acl, 4KB metadata, 2KB system metadata)
	};

	struct TrailerBlock_
	{
		uint64_t reserved[4];
		uint64_t sorted_record;
		uint64_t unsorted_record;
		uint64_t static_data_block_offset;
		uint64_t non_static_data_block_offset;
		uint64_t stat_block_offset;
		//unit64_t data_index_block_offset;
		//uint64_t last_compaction_offset;
	};

	boost::tuple<boost::shared_ptr<recordStructure::AccountStat>, boost::shared_ptr<TrailerBlock> >
										 create_trailer_and_stat(char *buffer);

		boost::shared_ptr<TrailerBlock> create_block_from_content(
						uint64_t sorted_record, uint64_t unsorted_record,
						uint64_t static_data_block_offset,
						uint64_t non_static_data_block_offset,
						uint64_t stat_block_offset);

	boost::shared_ptr<recordStructure::AccountStat> create_block_from_content(char* account, char* created_at,
							char* put_timestamp, char* delete_timestamp, uint64_t container_count,
							uint64_t object_count, uint64_t bytes_used, char* hash, char* id, char* status,
							char* status_changed_at, boost::python::dict metadata);


};

}

#endif
