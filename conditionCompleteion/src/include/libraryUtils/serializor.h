#ifndef __SERIALIZER_MANAGER_H_9978__
#define __SERIALIZER_MANAGER_H_9978__

#include <cool/serialization.h>

#include "libraryUtils/record_structure.h"

namespace serializor_manager
{

static const std::string START_MARKER = "####";
static const std::string END_MARKER = "&&&&";
static const uint64_t START_SIZE = sizeof(int64_t);
static const uint64_t HEADER_SIZE = START_MARKER.length() + sizeof(int64_t);
static const uint64_t FOOTER_SIZE = END_MARKER.length();

class Deserializor
{
	public:
		Deserializor();
		~Deserializor();
		recordStructure::Record * deserialize(char * buff, int64_t size);
		recordStructure::Record * osm_info_file_deserialize(char * buff, int64_t size);
		int64_t get_record_size(char* buf);
                int64_t get_osm_info_file_record_size(char* buf);
};

class Serializor
{
	public:
		Serializor();
		~Serializor();
		void serialize(recordStructure::Record * record, char * buff, int64_t size);
                void osm_info_file_serialize(recordStructure::Record * record, char * buff, int64_t size);
		int64_t get_serialized_size(recordStructure::Record * record);
                int64_t get_osm_info_file_serialized_size(recordStructure::Record * record);
};

} // namespace ends

#endif
