#include "communication/msg_external_interface.h"

MessageCommunicationWrapper::MessageCommunicationWrapper(
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr,
	boost::shared_ptr<Communication::SynchronousCommunication> comm
):
	msg_result_wrap_ptr(msg_result_wrap_ptr),
	comm(comm)
{
}

boost::shared_ptr<MessageResultWrap> MessageCommunicationWrapper::get_msg_result_wrap()
{
	return this->msg_result_wrap_ptr;
}

boost::shared_ptr<Communication::SynchronousCommunication> MessageCommunicationWrapper::get_comm_object()
{
	return this->comm;
}
