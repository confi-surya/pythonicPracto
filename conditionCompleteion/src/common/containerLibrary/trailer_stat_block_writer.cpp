#include <boost/python/extract.hpp>
#include "containerLibrary/trailer_stat_block_writer.h"
#include "containerLibrary/trailer_stat_block_builder.h"
#include "containerLibrary/config.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace containerInfoFile
{

block_buffer_size_tuple TrailerStatBlockWriter::get_writable_block(boost::shared_ptr<recordStructure::ContainerStat> container_stat,
							boost::shared_ptr<Trailer> trailer)
{
	OSDLOG(DEBUG, "coverting trailer and stat block");
	char *buffer  = new char[sizeof(TrailerAndStatBlockBuilder::Trailer_) + sizeof(TrailerAndStatBlockBuilder::StatBlock_)];
	memset(buffer, '\0', sizeof(TrailerAndStatBlockBuilder::Trailer_) + sizeof(TrailerAndStatBlockBuilder::StatBlock_));

	TrailerAndStatBlockBuilder::StatBlock_ *stat = reinterpret_cast<TrailerAndStatBlockBuilder::StatBlock_ *>(buffer);
	
	memcpy(
		stat->account,
		container_stat->account.c_str(),
		container_stat->account.length() > ACCOUNT_NAME_LENGTH ? ACCOUNT_NAME_LENGTH : container_stat->account.length());

	memcpy(
		stat->container,
		container_stat->container.c_str(),
		container_stat->container.length() > CONTAINER_NAME_LENGTH ? CONTAINER_NAME_LENGTH : container_stat->container.length());

	memcpy(
		stat->created_at,
		container_stat->created_at.c_str(),
		container_stat->created_at.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : container_stat->created_at.length());

	memcpy(
		stat->put_timestamp,
		container_stat->put_timestamp.c_str(),
		container_stat->put_timestamp.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : container_stat->put_timestamp.length());

	memcpy(
		stat->delete_timestamp,
		container_stat->delete_timestamp.c_str(),
		container_stat->delete_timestamp.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH:container_stat->delete_timestamp.length());

	stat->object_count = int64_t(container_stat->object_count);
	stat->bytes_used = container_stat->bytes_used;

	memcpy(
		stat->hash,
		container_stat->hash.c_str(),
		container_stat->hash.length() > HASH_LENGTH ? HASH_LENGTH : container_stat->hash.length());

	memcpy(
		stat->id,
		container_stat->id.c_str(),
		container_stat->id.length() > ID_LENGTH ? TIMESTAMP_LENGTH : container_stat->id.length());

	memcpy(
		stat->status,
		container_stat->status.c_str(),
		container_stat->status.length() > STATUS_LENGTH ? STATUS_LENGTH : container_stat->status.length());

	memcpy(
		stat->status_changed_at,
		container_stat->status_changed_at.c_str(),
		container_stat->status_changed_at.length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : container_stat->status_changed_at.length());

	cool::CountingOutputStream counter;
	cool::ObjectIoStream<> countingStream(counter);
	countingStream.enableTypeChecking(false);

	countingStream.readWrite(container_stat->metadata);
	int64_t size = counter.getCounter();

	char *metadata_buffer = new char[size];
	cool::FixedBufferOutputStreamAdapter output(metadata_buffer, size);
	cool::ObjectIoStream<> stream(output);
	stream.enableTypeChecking(false);
	stream.readWrite(container_stat->metadata);

	memcpy(stat->metadata, &size, sizeof(uint64_t));
	memcpy(stat->metadata + sizeof(uint64_t), metadata_buffer, size);
	delete[] metadata_buffer;

	TrailerAndStatBlockBuilder::Trailer_ *trailer_ptr = reinterpret_cast<TrailerAndStatBlockBuilder::Trailer_ *>(buffer + sizeof(TrailerAndStatBlockBuilder::StatBlock_));

	// Writing the version in the trailer section as CONTAINER_FILE_SUPPORTED_VERSION.
	trailer_ptr->version = CONTAINER_FILE_SUPPORTED_VERSION;
	trailer_ptr->sorted_record = trailer->sorted_record;
	trailer_ptr->unsorted_record = trailer->unsorted_record;
	trailer_ptr->index_offset = trailer->index_offset;
	trailer_ptr->data_index_offset = trailer->data_index_offset;
	trailer_ptr->data_offset = trailer->data_offset;
	trailer_ptr->stat_offset = trailer->stat_offset;
	trailer_ptr->journal_file_id = trailer->journal_offset.first;
	trailer_ptr->journal_offset = trailer->journal_offset.second;

	OSDLOG(INFO, "Trailer value are "<< trailer_ptr->sorted_record<<", "<<trailer_ptr->unsorted_record<<", "<<trailer_ptr->index_offset<<", "<<trailer_ptr->data_index_offset
			<<", "<<trailer_ptr->data_offset<<", "<<trailer_ptr->stat_offset<<", "<<trailer_ptr->journal_file_id<<", "<<trailer_ptr->journal_offset);

	return boost::make_tuple(buffer, sizeof(TrailerAndStatBlockBuilder::StatBlock_) + 
						sizeof(containerInfoFile::TrailerAndStatBlockBuilder::Trailer_));
}

}
