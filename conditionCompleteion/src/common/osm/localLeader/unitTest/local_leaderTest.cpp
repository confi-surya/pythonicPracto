#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "osm/localLeader/monitoring_manager.h"
#include "osm/localLeader/local_leader.h"
#include "osm/osmServer/utils.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/localLeader/ll_msg_handler.h"



namespace LocalLeaderTest
{

	TEST(LocalLeaderTest, LocalLeaderTest)
	{
		GLUtils gl_utils_obj;
		cool::SharedPtr<config_file::OSMConfigFileParser> config_parser;
		config_parser = gl_utils_obj.get_config();
		int32_t tcp_port = config_parser->osm_portValue();
		common_parser::Executor ex_obj;
		boost::shared_ptr<common_parser::MessageParser> parser_obj(new common_parser::MessageParser(ex_obj));
//		enum network_messages::NodeStatusEnum node_stat = network_messages::NEW;
		enum network_messages::NodeStatusEnum node_stat = network_messages::REGISTERED;
		boost::shared_ptr<common_parser::LLMsgProvider> ll_msg_provider_obj(new common_parser::LLMsgProvider);
		boost::shared_ptr<common_parser::LLNodeAddMsgProvider> na_msg_provider(new common_parser::LLNodeAddMsgProvider);
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr(
				new local_leader::LocalLeader(
					node_stat,
					ll_msg_provider_obj,
					na_msg_provider,
					parser_obj,
					config_parser,
					tcp_port
					)
				);

		boost::shared_ptr<boost::thread> ll_thread;
		ll_thread.reset(new boost::thread(&local_leader::LocalLeader::start, ll_ptr));
		sleep(15);
		ll_thread->interrupt();
		ll_thread->join();

		node_stat = network_messages::NEW;
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr2(
				new local_leader::LocalLeader(
					node_stat,
					ll_msg_provider_obj,
					na_msg_provider,
					parser_obj,
					config_parser,
					tcp_port
					)
				);
		
		boost::shared_ptr<boost::thread> ll_thread2;
		ll_thread2.reset(new boost::thread(&local_leader::LocalLeader::start, ll_ptr2));
		sleep(20);
		ll_thread2->interrupt();
		ll_thread2->join();
		
		node_stat = network_messages::RECOVERED;
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr3(
				new local_leader::LocalLeader(
					node_stat,
					ll_msg_provider_obj,
					na_msg_provider,
					parser_obj,
					config_parser,
					tcp_port
					)
				);
	
		boost::shared_ptr<boost::thread> ll_thread3;
		ll_thread3.reset(new boost::thread(&local_leader::LocalLeader::start, ll_ptr3));
		sleep(15);
		ll_thread3->interrupt();
		ll_thread3->join();

		node_stat = network_messages::FAILED;
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr4(
				new local_leader::LocalLeader(
					node_stat,
					ll_msg_provider_obj,
					na_msg_provider,
					parser_obj,
					config_parser,
					tcp_port
					)
				);
	
		boost::shared_ptr<boost::thread> ll_thread4;
		ll_thread4.reset(new boost::thread(&local_leader::LocalLeader::start, ll_ptr4));
		sleep(15);
		ll_thread4->interrupt();
		ll_thread4->join();

		node_stat = network_messages::RETIRING_FAILED;
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr5(
				new local_leader::LocalLeader(
					node_stat,
					ll_msg_provider_obj,
					na_msg_provider,
					parser_obj,
					config_parser,
					tcp_port
					)
				);
	
		boost::shared_ptr<boost::thread> ll_thread5;
		ll_thread5.reset(new boost::thread(&local_leader::LocalLeader::start, ll_ptr5));
		sleep(15);
		ll_thread5->interrupt();
		ll_thread5->join();
	

/*	
		node_stat = network_messages::INVALID_NODE;
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr6(
				new local_leader::LocalLeader(
					node_stat,
					ll_msg_provider_obj,
					na_msg_provider,
					parser_obj,
					config_parser,
					tcp_port
					)
				);

		boost::shared_ptr<boost::thread> ll_thread6;
		ll_thread6.reset(new boost::thread(&local_leader::LocalLeader::start, ll_ptr6));
		sleep(10);
		ll_thread6->interrupt();
		ll_thread6->join();
*/				
	}
}
