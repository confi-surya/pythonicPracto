#include "communication/communication_helper.h"
//#include "libraryUtils/object_storage_log_setup.h"
#include <iostream>

//namespace CommunicationHelper{

uint64_t CommunicationHelper::flag = 0;
boost::mutex CommunicationHelper::mtx;
boost::shared_ptr<boost::asio::io_service> CommunicationHelper::async_io_service;
boost::shared_ptr<boost::asio::io_service::work> CommunicationHelper::work;
//boost::shared_ptr<boost::asio::io_service> CommunicationHelper::sync_io_service;
boost::shared_ptr<boost::thread> CommunicationHelper::async_thread;


CommunicationHelper::CommunicationHelper()
{
	boost::mutex::scoped_lock scoped_lock(mtx);

	if (flag == 0){
		async_io_service.reset(new boost::asio::io_service);
//		sync_io_service.reset(new boost::asio::io_service);
		work.reset(new boost::asio::io_service::work(*async_io_service.get()));
//		async_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, async_io_service.get())));
		async_thread.reset(new boost::thread(CommunicationHelper::invoke));
	}
	++flag;
}

CommunicationHelper::~CommunicationHelper()
{
	boost::mutex::scoped_lock scoped_lock(mtx);

	--flag;
	if (flag <= 0){
		work.reset();
		async_io_service->reset();
		async_io_service->stop();
	//	async_thread->join();
	}
}

void CommunicationHelper::invoke()
{
	async_io_service->run();
}

boost::shared_ptr<boost::asio::io_service> CommunicationHelper::get_async_io_service()
{
	return async_io_service;
}

/*boost::shared_ptr<boost::asio::io_service> CommunicationHelper::get_sync_io_service()
{
	return sync_io_service;
}*/

//}
