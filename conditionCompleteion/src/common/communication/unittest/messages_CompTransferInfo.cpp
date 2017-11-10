#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include <vector>
#include <utility>
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/md5.h"

namespace CompTransferInfo_message_test
{

TEST(CompTransferInfo, explicit_constructor_serialization_check)
{
	std::string service_id = "HN0101_proxy-server";
	std::string service_ip = "192.168.101.";
	int32_t port = 124;

	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_service_list;

	for (int32_t i = 0; i < 1; i++){
		std::string counter = boost::lexical_cast<std::string>(i);
		service_id += counter;
		recordStructure::ComponentInfoRecord comp(service_id, service_ip, port);
		comp_service_list.push_back(std::make_pair(i, comp));
	}

	comm_messages::CompTransferInfo msg("container-server", comp_service_list);

	std::string serialized;
	ASSERT_TRUE(msg.serialize(serialized));

	char * deserialize_arrary = new char[serialized.size() + 1];
	memset(deserialize_arrary, '\0', serialized.size() + 1);
	memcpy(deserialize_arrary, serialized.c_str(), serialized.size());

	//deserialize_arrary[21] = 'A';
	//
//	std::cout << "md5 sum of serialized : " << md5(serialized.c_str(), serialized.size()) << std::endl;
//	std::cout << "md5 sum of deserialized : " << md5(deserialize_arrary, serialized.size()) << std::endl;
//	std::cout << "size : " << serialized.size();

	for (int i = 0; i < serialized.size() + 1; i++){
	//	std::cout << "< " << int(serialized[i]) << ", " << serialized[i] << " >";
	}
//	std::cout << "\nfinished\n";

	comm_messages::CompTransferInfo msg2;

	ASSERT_TRUE(msg2.deserialize(serialized.c_str(), serialized.size()));
}

}
