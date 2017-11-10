#include "libraryUtils/serializor.h"
#include "boost/lexical_cast.hpp"

#include "libraryUtils/object_storage_log_setup.h"

namespace serializor_manager
{

Deserializor::Deserializor()
{
}

Deserializor::~Deserializor()
{
}

int64_t Deserializor::get_record_size(char * buf)
{
	if (strncmp(buf, START_MARKER.c_str(), START_MARKER.size()) != 0)
	{
		OSDLOG(WARNING, "get_record_size START_MARKER mismatch");
		return int64_t(-1);
	}
	int64_t *size_ptr = reinterpret_cast<int64_t *>(&buf[START_MARKER.size()]);
	return *size_ptr;

}

int64_t Deserializor::get_osm_info_file_record_size(char* buf)
{
        try
	{
		int64_t *size_ptr = reinterpret_cast<int64_t *>(&buf[0]);
        	return *size_ptr;
	}
	catch(...)
	{
		return int64_t(-1);
	}
}

recordStructure::Record * Deserializor::deserialize(char * buff, int64_t size)
{
	if (memcmp((buff + size) - FOOTER_SIZE, END_MARKER.c_str(), FOOTER_SIZE))
	{
		OSDLOG(WARNING, "get_record_size END_MARKER mismatch");
		return NULL;
	}
	return recordStructure::deserialize_record(&buff[HEADER_SIZE], size - HEADER_SIZE - FOOTER_SIZE);
}

recordStructure::Record * Deserializor::osm_info_file_deserialize(char * buff, int64_t size)
{
        return recordStructure::deserialize_record(&buff[START_SIZE], size - START_SIZE);
}

Serializor::Serializor()
{
}

Serializor::~Serializor()
{
}

int64_t Serializor::get_serialized_size(recordStructure::Record * record)
{
	return record->get_serialized_size() + HEADER_SIZE + FOOTER_SIZE;
}

int64_t Serializor::get_osm_info_file_serialized_size(recordStructure::Record * record)
{
        return record->get_serialized_size() + START_SIZE;
}

void Serializor::serialize(recordStructure::Record * record, char * buff, int64_t size)
{
	::memcpy(&buff[0], START_MARKER.c_str(), START_MARKER.size());
	int64_t * ptr = reinterpret_cast<int64_t *>(&buff[START_MARKER.size()]);
	*ptr = size;
	int64_t len = size - HEADER_SIZE - FOOTER_SIZE;
	record->serialize_record(&buff[HEADER_SIZE], len);
	::memcpy(&buff[HEADER_SIZE + len], END_MARKER.c_str(), END_MARKER.size());
}

void Serializor::osm_info_file_serialize(recordStructure::Record * record, char * buff, int64_t size)
{
        int64_t * ptr = reinterpret_cast<int64_t *>(&buff[0]);
        *ptr = size;
        int64_t len = size - START_SIZE;
        record->serialize_record(&buff[START_SIZE], len);
}

} // namespace serializor_manager

