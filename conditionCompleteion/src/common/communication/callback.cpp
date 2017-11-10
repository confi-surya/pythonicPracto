#ifndef __CALLBACKS__
#define __CALLBACKS__

#include "communication/callback.h"
#include "libraryUtils/object_storage_log_setup.h"

SuccessCallBack::SuccessCallBack()
:
	message_wrap(new MessageResultWrap("Default construction", 255))
{
}

SuccessCallBack::~SuccessCallBack()
{
}

void SuccessCallBack::success_handler(boost::shared_ptr<MessageResultWrap> handler)
{
	this->message_wrap.reset();
	this->message_wrap = handler;
	this->is_success = true;
}

void SuccessCallBack::failure_handler()
{
	OSDLOG(ERROR, "Failure handler executed");
	this->is_success = false;
}

bool SuccessCallBack::get_result()
{
	return this->is_success;
}

boost::shared_ptr<MessageResultWrap> SuccessCallBack::get_object()
{
	return this->message_wrap;
}
#endif
