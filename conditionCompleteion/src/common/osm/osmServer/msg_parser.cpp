#include <boost/bind.hpp>
#include <iostream>

#include "communication/message_type_enum.pb.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/osmServer/osm_daemon.h"

using namespace ll_parser;
using namespace gl_parser;
//using namespace message;

namespace common_parser
{

MessageParser::MessageParser(Executor &ex):executor_obj(ex)
{

}

void MessageParser::decode_message(boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj)
{
	volatile const int msg_type = msg_comm_obj->get_msg_result_wrap()->get_type();

	//boost::function<void(boost::shared_ptr<Message>)> msg_handler = this->executor_obj.get_functor(msg_id);
	boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> msg_handler = this->executor_obj.get_functor(msg_type);

	if (msg_handler)
	{
		OSDLOG(INFO, "Calling handler for msg: " << msg_type);

		msg_handler(msg_comm_obj);
	}
	else
	{
		OSDLOG(INFO, "Msg type: " << msg_type << " discarded");

	}
}

void MessageParser::add_msg(int msg_type, boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> callback_func)
{
	this->executor_obj.register_callback(msg_type, callback_func);
}

void MessageParser::remove_msg(int msg_type)
{
	this->executor_obj.unregister_callback(msg_type);
}

void MainThreadMsgProvider::register_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj,
							boost::shared_ptr<MainThreadHandler> main_handler_obj)
{
	//parser_obj->add_msg(messageTypeEnum::typeEnums::STOP_SERVICES, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	//parser_obj->add_msg("GLOP", boost::bind(&MainThreadHandler::push, main_handler_obj, _1));	//GL_STOP
	parser_obj->add_msg(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP, boost::bind(&MainThreadHandler::push, main_handler_obj, _1));
	//parser_obj->add_msg("BEGL", boost::bind(&MainThreadHandler::push, main_handler_obj, _1));
}

void MainThreadMsgProvider::unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj)
{
	/*parser_obj->remove_msg("NODE_STOP");
	parser_obj->remove_msg("GL_STOP");
	*/
	parser_obj->remove_msg(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP);
	
}

void LLMsgProvider::register_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj,
							 boost::shared_ptr<LlMsgHandler> ll_parser_ptr)
{
	parser_obj->add_msg(messageTypeEnum::typeEnums::OSD_START_MONITORING, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::STOP_SERVICES, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_STOP_CLI, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));

	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_ADDITION_GL, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));
}

void LLMsgProvider::unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj)
{
	parser_obj->remove_msg(messageTypeEnum::typeEnums::OSD_START_MONITORING);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::STOP_SERVICES);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_STOP_CLI);
}

void LLNodeAddMsgProvider::register_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj, 
						boost::shared_ptr<ll_parser::LlMsgHandler> ll_parser_ptr)
{
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_ADDITION_GL, boost::bind(&ll_parser::LlMsgHandler::push, ll_parser_ptr, _1));
}

void LLNodeAddMsgProvider::unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj)
{
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_ADDITION_GL);
}

void GLMsgProvider::register_msgs(boost::shared_ptr<common_parser::MessageParser> &parser_obj,
					 boost::shared_ptr<gl_parser::GLMsgHandler> &gl_parser_ptr)
{
	this->parser_obj = parser_obj;
	parser_obj->add_msg(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::GET_GLOBAL_MAP, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::LL_START_MONITORING, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_ADDITION_CLI, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_RETIRE, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_STATUS, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_FAILOVER, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::GET_OBJECT_VERSION, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::NODE_STOP_LL, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::GET_CLUSTER_STATUS, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
	parser_obj->add_msg(messageTypeEnum::typeEnums::INITIATE_CLUSTER_RECOVERY, boost::bind(&GLMsgHandler::push, gl_parser_ptr, _1));
}

void GLMsgProvider::unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj)
{
	parser_obj->remove_msg(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::GET_GLOBAL_MAP);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::LL_START_MONITORING);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_ADDITION_CLI);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_RETIRE);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_STATUS);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_FAILOVER);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::GET_OBJECT_VERSION);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::GET_OBJECT_VERSION);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::NODE_STOP_LL);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::GET_CLUSTER_STATUS);
	parser_obj->remove_msg(messageTypeEnum::typeEnums::INITIATE_CLUSTER_RECOVERY);
}

boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> Executor::get_functor(int msg)
{
	boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> func_obj;
	std::map<int, boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> >::iterator it = this->callback_map.find(msg);
	if(it != this->callback_map.end())
		return it->second;
	return func_obj;
}

void Executor::register_callback(int msg_type, boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> callback_func)
{
	this->callback_map.insert(std::pair<int, boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> >(msg_type, callback_func));
}

void Executor::unregister_callback(int msg)
{
	this->callback_map.erase(msg);
}

} // namespace common_parser
