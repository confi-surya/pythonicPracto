#ifndef __COMMUNICATION_HELPER_H__
#define __COMMUNICATION_HELPER_H__

//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

/*
#define COMMLOGGER2(logLevel, msg)\
{\
        OSDLOG(logLevel, " [communication-library] : " << msg);\
}\
*/

//namespace CommunicationHelper{

class CommunicationHelper
{
	public:
		CommunicationHelper();
		~CommunicationHelper();
		static boost::shared_ptr<boost::asio::io_service> get_sync_io_service();
		static boost::shared_ptr<boost::asio::io_service> get_async_io_service();
		static void invoke();

	private:
		static boost::shared_ptr<boost::asio::io_service> sync_io_service;
		static boost::shared_ptr<boost::asio::io_service> async_io_service;
		static boost::shared_ptr<boost::asio::io_service::work> work;
		static boost::shared_ptr<boost::thread> async_thread;
		static boost::mutex mtx;
		static uint64_t flag;
};

//}

#endif
