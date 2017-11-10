#ifndef __TRAILER_STAT_BLOCK_BUILDER_H__
#define __TRAILER_STAT_BLOCK_BUILDER_H__

#include <boost/shared_ptr.hpp>

#include "config.h"
#include "trailer_block.h"
#include "libraryUtils/record_structure.h"
#include "trailer_stat_block_builder.h"
#include "index_block_builder.h"

namespace containerInfoFile
{

class TrailerAndStatBlockBuilder
{
public:
        struct StatBlock_
        {
                char account[ACCOUNT_NAME_LENGTH + 1];
                char container[CONTAINER_NAME_LENGTH + 1];
                char created_at[TIMESTAMP_LENGTH + 1];
                char put_timestamp[TIMESTAMP_LENGTH + 1];
                char delete_timestamp[TIMESTAMP_LENGTH + 1];
                uint32_t object_count;
                uint64_t bytes_used;
                char hash[HASH_LENGTH + 1];
                char id[ID_LENGTH + 1];
                char status[STATUS_LENGTH + 1];
                char status_changed_at[TIMESTAMP_LENGTH + 1];
		char metadata[META_DATA_LENGTH]; // 30KB metadata( 24KB acl, 4KB metadata, 2KB system metadata)
        };

	struct Trailer_
	{
		uint64_t reserved[2];
		uint64_t version;
		uint64_t sorted_record;
		uint64_t unsorted_record;
		uint64_t index_offset;
		uint64_t data_index_offset;
		uint64_t data_offset;
		uint64_t stat_offset;
		uint64_t journal_file_id;
		int64_t  journal_offset;
	}; 

        boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, boost::shared_ptr<containerInfoFile::Trailer> >
							 create_trailer_and_stat(char *buffer);

        boost::shared_ptr<Trailer> create_trailer_from_content(uint64_t sorted_record, uint64_t unsorted_record, 
					uint64_t index_offset, uint64_t data_index_offset, uint64_t data_offset, uint64_t stat_offset);

	boost::shared_ptr<recordStructure::ContainerStat> create_stat_from_content(std::string account, std::string container,\
                        std::string created_at, std::string put_timestamp, std::string delete_timestamp, int64_t object_count,\
                        uint64_t bytes_used, std::string hash, std::string id, std::string status, std::string status_changed_at,\
                        boost::python::dict metadata);

};

}

#endif
