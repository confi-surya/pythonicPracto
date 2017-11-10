#include "accountLibrary/trailer_stat_block_builder.h"
#include <iostream>

namespace accountInfoFile
{

boost::tuple<boost::shared_ptr<recordStructure::AccountStat>, boost::shared_ptr<accountInfoFile::TrailerBlock> >
						 TrailerStatBlockBuilder::create_trailer_and_stat(char *buffer)
{
	char *stat_buffer = new char[sizeof(StatBlock_)];
	memset(stat_buffer, '\0', sizeof(StatBlock_));
	memcpy(stat_buffer, buffer, sizeof(StatBlock_));

	StatBlock_ *stat_block = reinterpret_cast<StatBlock_*>(stat_buffer);

	uint64_t size = reinterpret_cast<uint64_t>(stat_block->metadata);
	cool::FixedBufferInputStreamAdapter input(stat_block->metadata + sizeof(uint64_t), size);
	cool::ObjectIoStream<> stream(input);
	stream.enableTypeChecking(false);
	std::map<std::string, std::string> metadata;
	stream.readWrite(metadata);

	boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat( 
				//= this->create_block_from_content(
				stat_block->account, stat_block->created_at, stat_block->put_timestamp, stat_block->delete_timestamp,
				stat_block->container_count, stat_block->object_count, stat_block->bytes_used, stat_block->hash,
				stat_block->id, stat_block->status, stat_block->status_changed_at, metadata));

	char *trailer_buffer = new char[sizeof(TrailerBlock_)];
	memcpy(trailer_buffer, buffer + sizeof(StatBlock_), sizeof(TrailerBlock_));
	
	TrailerBlock_ *trailer_ptr = reinterpret_cast<TrailerBlock_*>(trailer_buffer);

	boost::shared_ptr<TrailerBlock> trailer(new TrailerBlock(trailer_ptr->sorted_record,
							trailer_ptr->unsorted_record,
							trailer_ptr->static_data_block_offset,
							trailer_ptr->non_static_data_block_offset,
							trailer_ptr->stat_block_offset));

	delete[] stat_buffer;
	delete[] trailer_buffer;
	return boost::make_tuple(account_stat, trailer);
}

boost::shared_ptr<recordStructure::AccountStat> TrailerStatBlockBuilder::create_block_from_content(
						char* account, char* created_at, char* put_timestamp, char* delete_timestamp,
						uint64_t container_count, uint64_t object_count, uint64_t bytes_used, char* hash, char* id,
						char* status, char* status_changed_at, boost::python::dict metadata)
{

	boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat(
								account, created_at, put_timestamp, delete_timestamp, container_count, 
								object_count, bytes_used, hash, id, status, status_changed_at, metadata));
		return account_stat;
}

boost::shared_ptr<TrailerBlock> TrailerStatBlockBuilder::create_block_from_content(
							uint64_t sorted_record, uint64_t unsorted_record,
							uint64_t static_data_block_offset,
							uint64_t non_static_data_block_offset,
							uint64_t stat_block_offset)
{

	boost::shared_ptr<TrailerBlock> trailer(new TrailerBlock(
							sorted_record, unsorted_record,
							static_data_block_offset,
							non_static_data_block_offset,
							stat_block_offset));
	return trailer;
}

}
