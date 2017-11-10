#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include<string.h>
#include<iostream>
#include<stdlib.h>
#include <boost/shared_ptr.hpp>
#include "osm/osmServer/utils.h"
#include <cool/cool.h>
#include <cool/misc.h>
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/service_config_reader.h"
//#include "osm/localLeader/monitoring_manager.h"
//#include "osm/localLeader/election_manager.h"
#include "osm/localLeader/ll_msg_handler.h"
//#include "communication/message_result_wrapper.h"
//#include "communication/msg_external_interface.h"
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
//#include "communication/message_type_enum.pb.h"

using namespace ll_parser;
namespace LlMsgHandlerTest
{

	TEST(LlMsgHandlerTest, LlMsgHandlerTest)
	{
		GLUtils gl_utils_obj;
		cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
		config_parser = gl_utils_obj.get_config();		
		boost::shared_ptr<hfs_checker::HfsChecker>hfs_checker;
		hfs_checker.reset(new hfs_checker::HfsChecker(config_parser));
		boost::shared_ptr<MonitoringManager> mgr;
		mgr.reset(new MonitoringManager(hfs_checker, config_parser));
		boost::shared_ptr<ElectionManager>election_mgr;
		election_mgr.reset(new ElectionManager( gl_utils_obj.get_my_node_id(), config_parser, hfs_checker));		

		boost::shared_ptr<LlMsgHandler> llMsgHandlerTest;
                llMsgHandlerTest.reset(new LlMsgHandler(mgr,election_mgr, config_parser));

//		OSDLOG(DEBUG, "Verfiy node add called");
//		llMsgHandlerTest.verify_node_addition_msg_recv();
		
		boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;
		comm_obj.reset(new Communication::SynchronousCommunication("192.168.101.1", 61005));
		
		char *service_id = new char[100];
		strcpy(service_id, "hydra042_61014_account-server");
		bool val ;
		string new_payload;

		boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr;
		boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr_null;
		boost::shared_ptr<MessageCommunicationWrapper> msg;
		boost::shared_ptr<MessageCommunicationWrapper> msg_chk;

		msg_result_wrap_ptr.reset(new MessageResultWrap(52)); 
		comm_messages::NodeAdditionGl node_add_obj(service_id);
		val = node_add_obj.serialize(new_payload);
		OSDLOG(DEBUG, "serialize Node Addition Gl :" << val );
		msg_result_wrap_ptr->set_payload((char *)new_payload.c_str(), new_payload.size());
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		
		llMsgHandlerTest->push(msg); 
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();
//		llMsgHandlerTest.verify_node_addition_msg_recv();

		msg_result_wrap_ptr.reset(new MessageResultWrap(3));
		comm_messages::OsdStartMonitoring strm_obj(service_id, 61005, "192.168.101.1");
		new_payload = "";
		val = strm_obj.serialize(new_payload);
		OSDLOG(DEBUG, "serialize  Strm_obj  :" << val );
		msg_result_wrap_ptr->set_payload((char *)new_payload.c_str(), new_payload.size());
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		llMsgHandlerTest->push(msg);
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();		
	
	        msg_result_wrap_ptr.reset(new MessageResultWrap(56));
		comm_messages::NodeSystemStopCli stop_cli_obj(service_id);
		new_payload = "";
		val = stop_cli_obj.serialize(new_payload);
		OSDLOG(DEBUG, "serialize  System_stop_cli  :" << val );
		msg_result_wrap_ptr->set_payload((char *)new_payload.c_str(), new_payload.size());
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		llMsgHandlerTest->push(msg);
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();	

		msg_result_wrap_ptr.reset(new MessageResultWrap(57));
		comm_messages::LocalNodeStatus loc_node_obj(service_id);
		new_payload = "";
		val = loc_node_obj.serialize(new_payload);
		OSDLOG(DEBUG, "serialize  Local_node_obj_cli  :" << val);
		msg_result_wrap_ptr->set_payload((char *)new_payload.c_str(), new_payload.size());
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		llMsgHandlerTest->push(msg);
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();

		msg_result_wrap_ptr.reset(new MessageResultWrap(63));
		comm_messages::StopServices stop_serv_obj(service_id);
		new_payload = "";
		val = stop_serv_obj.serialize(new_payload);
		OSDLOG(DEBUG, "serialize  stop_serv_obj  :" << val);
		msg_result_wrap_ptr->set_payload((char *)new_payload.c_str(), new_payload.size());
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		llMsgHandlerTest->push(msg);
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();
				
		msg_result_wrap_ptr.reset(new MessageResultWrap(61));
		comm_messages::NodeStopCli node_stop_cli_obj(service_id);
		new_payload =  "";
		val  = node_stop_cli_obj.serialize(new_payload);
		OSDLOG(DEBUG, "serialize  node_stop_cli_obj  :" << val);
		msg_result_wrap_ptr->set_payload((char *)new_payload.c_str(), new_payload.size());
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		llMsgHandlerTest->push(msg);
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();

		msg_result_wrap_ptr.reset(new MessageResultWrap(4));
		msg.reset(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm_obj));
		llMsgHandlerTest->push(msg);
		llMsgHandlerTest->push(msg_chk);
		llMsgHandlerTest->start();
	}
}
