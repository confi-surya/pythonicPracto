#include "communication/message_result_wrapper.h"

class SuccessCallBack
{
	public:
		SuccessCallBack();
		~SuccessCallBack();

		void success_handler(boost::shared_ptr<MessageResultWrap>);
		void failure_handler();
		bool get_result();
		boost::shared_ptr<MessageResultWrap> get_object();

	private:
		bool is_success;
		boost::shared_ptr<MessageResultWrap> message_wrap;
};

class TimeoutCallBack
{
	public:
		TimeoutCallBack();
		~TimeoutCallBack();

		void handler();

	private:
		bool is_success;
};
