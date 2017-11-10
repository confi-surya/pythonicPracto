#ifndef __MESSAGE_PARSER_999__
#define __MESSAGE_PARSER_999__

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition_variable.hpp>

#include "osm/localLeader/ll_msg_handler.h"
#include "osm/globalLeader/gl_msg_handler.h"
#include "communication/msg_external_interface.h"

class MainThreadHandler;

namespace common_parser
{

enum MSG_TYPE
{
	HEART_BEAT = 1,
	NODE_ADDITION = 2,
	NODE_DELETION = 3,
	NODE_STOP = 4,
	GET_SERVICE_COMPONENT = 5,	//OSD service to GL (only ownership)
	GET_SERVICE_COMPONENT_ACK = 6,	//Response by GL to OSD service
	GET_GLOBAL_MAP = 7,		//OSD service to GL (complete map)
	GLOBAL_MAP = 8,			//response by GL to OSD service
	LL_STRT_MONITORING = 9,	
	NODE_FAILOVER = 10,
	RCV_PRO_STRT_MONITORING = 11,
	RCV_PRO_STRT_MONITORING_ACK = 12,
	COMP_TRNSFR_INFO = 13,
	COMP_TRNSFR_INFO_ACK = 14,
	COMP_TRNFR_FINAL_STAT = 15,
	GL_STOP = 16,
	OSD_STRT_MONITORING = 17,
	TAKEOVER_AS_GL = 18,
	NODE_RETIREMENT = 19
};

class Executor
{
	public:
		//Executor();
		boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> get_functor(int msg_type);
		void register_callback(
			int msg_type,
			boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> callback_func
			);
		//void register_callback(std::string msg, boost::function<void(boost::shared_ptr<message::Message>)> callback_func);
		void unregister_callback(int msg_type);
		//void unregister_callback(std::string msg);
	private:
		std::map<int, boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> > callback_map;
};

class MessageParser
{
	public:
		MessageParser(Executor &ex);
		//void decode_message(char *buf, int size, int new_fd);
		void decode_message(boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj);
		void register_ll_functionality(ll_parser::LlMsgHandler &ll_parser_obj, bool ll_full);
		void add_msg(int msg_type, boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> callback_func);
		//void add_msg(std::string msg, boost::function<void(boost::shared_ptr<message::Message>)> callback_func);
		void remove_msg(int msg_type);
		//void remove_msg(std::string msg);
		void register_ll_wait_functionality(ll_parser::LlMsgHandler &ll_parser_obj);
		void register_gl_msg_handler(boost::shared_ptr<gl_parser::GLMsgHandler> gl_parser_obj);
	private:
		bool if_gl;
		bool ll_full;
		Executor executor_obj;
};

class MainThreadMsgProvider
{
	public:
		void register_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj,
				boost::shared_ptr<MainThreadHandler> main_handler_obj);
		void unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj);
};

class LLMsgProvider
{
	public:
		void register_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj,
					 boost::shared_ptr<ll_parser::LlMsgHandler> ll_parser_ptr);
		void unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj);
};

class GLMsgProvider
{
	public:
		void register_msgs(boost::shared_ptr<common_parser::MessageParser> &parser_obj, 
					boost::shared_ptr<gl_parser::GLMsgHandler> &gl_parser_ptr);
		void unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj);	//mis-spelled

	private:
		boost::shared_ptr<common_parser::MessageParser> parser_obj;
};

class LLNodeAddMsgProvider
{
	public:
		void register_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj, 
					boost::shared_ptr<ll_parser::LlMsgHandler> ll_parser_ptr);
		void unregister_msgs(boost::shared_ptr<common_parser::MessageParser> parser_obj);
};

} // namespace common_parser

#endif
