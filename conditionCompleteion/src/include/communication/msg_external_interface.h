#ifndef __MESSAGE_COMM_WRAPPER_H_897__
#define __MESSAGE_COMM_WRAPPER_H_897__

#include "communication/message_interface_handlers.h"
//#include "osm/listener.h"

class MessageCommunicationWrapper
{
	public:
		MessageCommunicationWrapper(
			boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr,
			boost::shared_ptr<Communication::SynchronousCommunication> comm
		);
		boost::shared_ptr<MessageResultWrap> get_msg_result_wrap();
		boost::shared_ptr<Communication::SynchronousCommunication> get_comm_object();
	private:
		boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr;
		boost::shared_ptr<Communication::SynchronousCommunication> comm;
};

#endif
