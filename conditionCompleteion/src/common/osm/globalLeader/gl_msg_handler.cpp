#include <iostream>

#include "libraryUtils/object_storage_log_setup.h"
#include "osm/globalLeader/gl_msg_handler.h"
#include "communication/message_type_enum.pb.h"
#include "osm/osmServer/msg_handler.h"

namespace gl_parser
{

GLMsgHandler::GLMsgHandler(
	boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> global_leader_handler,
	boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> state_msg_handler,
	boost::function<void(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring>,boost::shared_ptr<Communication::SynchronousCommunication>) > monitor_service,
	boost::function<void(boost::shared_ptr<comm_messages::NodeStatus>, boost::shared_ptr<Communication::SynchronousCommunication>) > retrieve_node_status
):
	global_leader_handler(global_leader_handler),
	state_msg_handler(state_msg_handler),
	monitor_service(monitor_service),
	retrieve_node_status(retrieve_node_status)
{
	OSDLOG(DEBUG, "GL Message Handler built.");
	this->stop = false;
}

void GLMsgHandler::push(boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj)
{
	boost::mutex::scoped_lock lock(this->mtx);
	this->gl_msg_queue.push(msg_comm_obj);
	this->condition.notify_one();
}

boost::shared_ptr<MessageCommunicationWrapper> GLMsgHandler::pop()
{
	boost::mutex::scoped_lock lock(this->mtx);
	while(this->gl_msg_queue.empty() and !this->stop)
	{
		this->condition.wait(lock);
	}
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj;
	if(!this->gl_msg_queue.empty())
	{
		msg_comm_obj = this->gl_msg_queue.front();
		this->gl_msg_queue.pop();
	}
	return msg_comm_obj;
}

void GLMsgHandler::abort()
{
	this->stop = true;
	this->condition.notify_one();
}

void GLMsgHandler::handle_msg()
{
	OSDLOG(INFO, "GL Msg handler thread start");
	while(1)
	{
		boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj = this->pop();
		if(msg_comm_obj)
		{
			const uint32_t msg_type = msg_comm_obj->get_msg_result_wrap()->get_type();
			switch(msg_type)
			{
				case messageTypeEnum::typeEnums::NODE_ADDITION_CLI:
					OSDLOG(INFO, "Adding node in the system");
					this->global_leader_handler(msg_comm_obj);	//TODO: Ask LL to start OSD services 
					break;
/*				case common_parser::NODE_DELETION:
					this->global_leader_handler(msg_comm_obj);
					break;
*/				case messageTypeEnum::typeEnums::GET_OBJECT_VERSION:
					OSDLOG(DEBUG, "Get object version req rcvd");
					if(this->global_leader_handler != NULL)
						this->global_leader_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::NODE_RETIRE:
					this->state_msg_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::GET_GLOBAL_MAP:		//Get Global Map
					this->state_msg_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT:		//Get My Components
					this->state_msg_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::COMP_TRANSFER_INFO:
					this->state_msg_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::LL_START_MONITORING:		//Start LL Monitoring
					{
					boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_obj(
									new comm_messages::LocalLeaderStartMonitoring);
					strm_obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(),
									msg_comm_obj->get_msg_result_wrap()->get_size());
					boost::shared_ptr<Communication::SynchronousCommunication> comm_obj(
									msg_comm_obj->get_comm_object());
					this->monitor_service(strm_obj, comm_obj);
					}
					break;
				case messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY:
					this->global_leader_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::NODE_FAILOVER:		//Failure Notification by LL
					this->state_msg_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING:
					this->state_msg_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::INITIATE_CLUSTER_RECOVERY:
					this->global_leader_handler(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::NODE_STATUS:
					{
					boost::shared_ptr<comm_messages::NodeStatus> node_status_req(new comm_messages::NodeStatus);
					node_status_req->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					boost::shared_ptr<Communication::SynchronousCommunication> comm_obj(msg_comm_obj->get_comm_object());
					this->retrieve_node_status(node_status_req, comm_obj);
					}
					break;
				case messageTypeEnum::typeEnums::NODE_STOP_LL:
					{
					this->state_msg_handler(msg_comm_obj);
					break;
					}
				case messageTypeEnum::typeEnums::GET_CLUSTER_STATUS:
					{
					this->global_leader_handler(msg_comm_obj);	
					break;
					}
				default:
					OSDLOG(INFO, "Msg type not supported by GL Msg Handler");
			}
		}
		else
		{
			OSDLOG(INFO, "GL Msg dispatcher Thread exiting");
			break;
		}
	}
}

} // namespace gl_parser

