#include "libraryUtils/object_storage_log_setup.h"
#include "containerLibrary/trailer_stat_block_builder.h"
#include "containerLibrary/config.h"

namespace containerInfoFile
{

boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, boost::shared_ptr<containerInfoFile::Trailer> >
						TrailerAndStatBlockBuilder::create_trailer_and_stat(char *buffer)
{
	OSDLOG(DEBUG, "Creating stat from trailer section");
	char *buffer_stat = new char[sizeof(TrailerAndStatBlockBuilder::StatBlock_)];
	memcpy(buffer_stat, buffer, sizeof(TrailerAndStatBlockBuilder::StatBlock_));

	TrailerAndStatBlockBuilder::StatBlock_ *stat = reinterpret_cast<TrailerAndStatBlockBuilder::StatBlock_*>(buffer);
	uint64_t size = reinterpret_cast<uint64_t>(stat->metadata);
	cool::FixedBufferInputStreamAdapter input(stat->metadata + sizeof(uint64_t), size);
	cool::ObjectIoStream<> stream(input);
	stream.enableTypeChecking(false);
	std::map<std::string, std::string> metadata;
	stream.readWrite(metadata);

	boost::shared_ptr<recordStructure::ContainerStat> stat_ptr(new recordStructure::ContainerStat(
	/*  this->createStatFromContent(*/stat->account, stat->container,
			stat->created_at, stat->put_timestamp, stat->delete_timestamp, stat->object_count,
			stat->bytes_used, stat->hash, stat->id, stat->status, stat->status_changed_at, metadata));
	char *buffer_trailer = new char[sizeof(Trailer_)];
	memcpy(buffer_trailer, buffer + sizeof(StatBlock_), sizeof(Trailer_));
	//Dump1(buffer_trailer, 72);
	Trailer_ *trailerPtr = reinterpret_cast<Trailer_*>(buffer_trailer);

	boost::shared_ptr<Trailer> trailer(new Trailer(
			trailerPtr->sorted_record, trailerPtr->unsorted_record,
			trailerPtr->index_offset, trailerPtr->data_index_offset,
			trailerPtr->data_offset, trailerPtr->stat_offset,
			trailerPtr->journal_file_id, trailerPtr->journal_offset, trailerPtr->version));

	delete[] buffer_stat;
	delete[] buffer_trailer;
	return boost::make_tuple(stat_ptr, trailer);
}
//UNCOVERED_CODE_BEGIN:

boost::shared_ptr<Trailer> TrailerAndStatBlockBuilder::create_trailer_from_content(uint64_t sorted_record, uint64_t unsorted_record,
		uint64_t index_offset, uint64_t data_index_offset, uint64_t data_offset, uint64_t stat_offset)
{
	// TODO remove; used only from unittest
	boost::shared_ptr<Trailer> trailer(new Trailer(sorted_record, unsorted_record,
			index_offset, data_index_offset, data_offset, stat_offset, 0, 0, CONTAINER_FILE_SUPPORTED_VERSION));
	return trailer;
}
//UNCOVERED_CODE_END

boost::shared_ptr<recordStructure::ContainerStat> TrailerAndStatBlockBuilder::create_stat_from_content(
		std::string account,
		std::string container,
		std::string created_at,
		std::string put_timestamp,
		std::string delete_timestamp,
		int64_t object_count,
		uint64_t bytes_used,
		std::string hash,
		std::string id,
		std::string status,
		std::string status_changed_at,
		boost::python::dict metadata
)
{

	boost::shared_ptr<recordStructure::ContainerStat> container_stat(
						new recordStructure::ContainerStat(account, container,
							created_at, put_timestamp, delete_timestamp, object_count,
							bytes_used, hash, id, status, status_changed_at, metadata)
					);
	return container_stat;
}


}
