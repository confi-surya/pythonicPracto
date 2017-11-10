#include "accountLibrary/trailer_stat_block_writer.h"
#include "accountLibrary/trailer_stat_block_builder.h"
#include "libraryUtils/object_storage_log_setup.h"
#include <iostream>

namespace accountInfoFile
{

block_buffer_size_tuple TrailerStatBlockWriter::get_writable_block(boost::shared_ptr<recordStructure::AccountStat> account_stat,
											boost::shared_ptr<TrailerBlock> trailer)
{
	char *buffer  = new char[sizeof(TrailerStatBlockBuilder::StatBlock_) + sizeof(TrailerStatBlockBuilder::TrailerBlock_)];
	memset(buffer, '\0', 
			sizeof(TrailerStatBlockBuilder::StatBlock_) + sizeof(TrailerStatBlockBuilder::TrailerBlock_));

	TrailerStatBlockBuilder::StatBlock_ *stat = reinterpret_cast<TrailerStatBlockBuilder::StatBlock_*>(buffer);
	memcpy(
		stat->account,
		account_stat->account.c_str(),
		account_stat->account.length() > ACCOUNT_NAME_LENGTH ? ACCOUNT_NAME_LENGTH : account_stat->account.length());

	memcpy(
		stat->created_at,
		account_stat->created_at.c_str(),
		account_stat->created_at.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : account_stat->created_at.length());

	memcpy(
		stat->put_timestamp,
		account_stat->put_timestamp.c_str(),
		account_stat->put_timestamp.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : account_stat->put_timestamp.length());

	memcpy(
		stat->delete_timestamp,
		account_stat->delete_timestamp.c_str(),
		account_stat->delete_timestamp.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : account_stat->delete_timestamp.length());

	stat->container_count = account_stat->container_count;
	stat->object_count = account_stat->object_count;
	stat->bytes_used = account_stat->bytes_used;

	memcpy(
		stat->hash,
		account_stat->hash.c_str(),
		account_stat->hash.length() > HASH_LENGTH ? HASH_LENGTH : account_stat->hash.length());

	memcpy(
		stat->id,
		account_stat->id.c_str(),
		account_stat->id.length() > ID_LENGTH ? ID_LENGTH : account_stat->id.length());

	memcpy(
		stat->status,
		account_stat->status.c_str(),
		account_stat->status.length() > STATUS_LENGTH ? STATUS_LENGTH : account_stat->status.length());

	memcpy(
		stat->status_changed_at,
		account_stat->status_changed_at.c_str(),
		account_stat->status_changed_at.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : account_stat->status_changed_at.length());

		cool::CountingOutputStream counter;
		cool::ObjectIoStream<> countingStream(counter);
		countingStream.enableTypeChecking(false);

		countingStream.readWrite(account_stat->metadata);
		int64_t size = counter.getCounter();

		char *metadata_buffer = new char[size];
		cool::FixedBufferOutputStreamAdapter output(metadata_buffer, size);
		cool::ObjectIoStream<> stream(output);
		stream.enableTypeChecking(false);
		stream.readWrite(account_stat->metadata);

		memcpy(stat->metadata, &size, sizeof(uint64_t));
		memcpy(stat->metadata + sizeof(uint64_t), metadata_buffer, size);
		delete[] metadata_buffer;

	TrailerStatBlockBuilder::TrailerBlock_ *trailer_ptr = 
		reinterpret_cast<TrailerStatBlockBuilder::TrailerBlock_*>(buffer + sizeof(TrailerStatBlockBuilder::StatBlock_));
	trailer_ptr->sorted_record = trailer->sorted_record;
	trailer_ptr->unsorted_record = trailer->unsorted_record;
	trailer_ptr->static_data_block_offset = trailer->static_data_block_offset;
	trailer_ptr->non_static_data_block_offset = trailer->non_static_data_block_offset;
	trailer_ptr->stat_block_offset = trailer->stat_block_offset;

	OSDLOG(DEBUG, "Trailer values are :- sorted_record : "<<trailer_ptr->sorted_record<<", unsorted_record : "
			<<trailer_ptr->unsorted_record<<", static_data_block_offset : "<<trailer_ptr->static_data_block_offset
			<<", non_static_data_block_offset : "<<trailer_ptr->non_static_data_block_offset
			<<", stat_block_offset : "<<trailer_ptr->stat_block_offset);

	return boost::make_tuple(reinterpret_cast<char*>(buffer), 
		sizeof(TrailerStatBlockBuilder::StatBlock_) + sizeof(TrailerStatBlockBuilder::TrailerBlock_));
}

}
