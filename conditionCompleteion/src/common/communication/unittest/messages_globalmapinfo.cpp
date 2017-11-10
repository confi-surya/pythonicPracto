#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include <tr1/tuple>
#include <vector>
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include "libraryUtils/record_structure.h"

namespace globalmapinfo_message_test
{

std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> get_globalmap_tuple(std::string service_id, std::string service_ip, int32_t port)
{
	std::vector<recordStructure::ComponentInfoRecord> comp_list;
	for (uint32_t i = 0; i < 1; i++){
		std::string counter = boost::lexical_cast<std::string>(i);
//		service_id.append(counter);
		service_id += counter;
		recordStructure::ComponentInfoRecord comp(service_id, service_ip, port);
		comp_list.push_back(comp);
	}
//	std::cout << "size : " << comp_list.size() << std::endl;
	float help_me = 2.1;
	return std::tr1::make_tuple(comp_list, help_me);
}

bool verify_vector_content(std::vector<recordStructure::ComponentInfoRecord> vector_1, std::vector<recordStructure::ComponentInfoRecord> vector_2){

	if (vector_1.size() != vector_2.size()){
		return false;
	}

	std::vector<recordStructure::ComponentInfoRecord>::iterator it;

	for (it = vector_1.begin(); it != vector_1.end(); it++){
		bool is_found = false;
		for (int i = 0; i < vector_2.size(); i++){
			if (*it == vector_2[i]){
//				std::cout << "base : " << *it << " , copy : " << vector_2[i] << std::endl;
				is_found = true;
				break;
			}
		}
		if (!is_found){
			return false;
		}
	}
	return true;

}

TEST(GlobalMapInfoTest, explicit_constructor_serialization_check)
{

	std::string service_id = "HN0101_proxy-server";
	std::string service_ip = "192.168.101.";
	int32_t port = 110;

	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair;
	float version = 21.1;

	container_pair = get_globalmap_tuple(service_id, service_ip, port);
	account_pair = get_globalmap_tuple(service_id, service_ip, port);
	updater_pair = get_globalmap_tuple(service_id, service_ip, port);
	object_pair = get_globalmap_tuple(service_id, service_ip, port);

	comm_messages::GlobalMapInfo msg(container_pair, account_pair, updater_pair, object_pair, version);

	std::string serialized;
	ASSERT_TRUE(msg.serialize(serialized));
	uint64_t size = serialized.size();

	comm_messages::GlobalMapInfo msg2;
	char * msg_string = new char[size + 1];
	memset(msg_string, '\0', size + 1);
	memcpy(msg_string, serialized.c_str(), size);


	ASSERT_TRUE(msg2.deserialize(msg_string, size));
	ASSERT_FLOAT_EQ(version, msg2.get_version());
	ASSERT_TRUE(msg2.get_status());

	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair_2(
		std::tr1::make_tuple(
			msg2.get_service_object_of_container(),
			msg2.get_version_in_container()
		)
	);
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair_2(
		std::tr1::make_tuple(
			msg2.get_service_object_of_account(),
			msg2.get_version_in_account()
		)
	);
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair_2(
		std::tr1::make_tuple(
			msg2.get_service_object_of_updater(),
			msg2.get_version_in_updater()
		)
	);
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair_2(
		std::tr1::make_tuple(
			msg2.get_service_object_of_objectService(),
			msg2.get_version_in_object()
		)
	);

	ASSERT_TRUE(verify_vector_content(std::tr1::get<0>(container_pair_2), std::tr1::get<0>(container_pair)));
	ASSERT_TRUE(verify_vector_content(std::tr1::get<0>(account_pair_2), std::tr1::get<0>(account_pair)));
	ASSERT_TRUE(verify_vector_content(std::tr1::get<0>(updater_pair_2), std::tr1::get<0>(updater_pair)));
	ASSERT_TRUE(verify_vector_content(std::tr1::get<0>(object_pair_2), std::tr1::get<0>(object_pair)));

	/*std::cout << "base float : " << std::tr1::get<1>(container_pair_2) << ", copy float : " << std::tr1::get<1>(container_pair) << std::endl;
	std::cout << "base float : " << std::tr1::get<1>(account_pair_2) << ", copy float : " << std::tr1::get<1>(account_pair) << std::endl;
	std::cout << "base float : " << std::tr1::get<1>(updater_pair_2) << ", copy float : " << std::tr1::get<1>(updater_pair) << std::endl;
	std::cout << "base float : " << std::tr1::get<1>(object_pair_2) << ", copy float : " << std::tr1::get<1>(object_pair) << std::endl;*/

	delete [] msg_string;
}

}
