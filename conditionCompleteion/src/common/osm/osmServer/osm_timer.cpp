#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "osm/osmServer/osm_timer.h"
#include "libraryUtils/object_storage_log_setup.h"


OsmTimer::OsmTimer()
{
}

OsmTimer::~OsmTimer()
{
}

int OsmTimer::doTaskWhileWaiting(const boost::system::error_code&)
{
	/*Just Do, Whatever you want to do man*/
	return 1;
}

int OsmTimer::blockingWait(int waitTime)
{
/*	boost::asio::deadline_timer timer(this->io, boost::posix_time::seconds(waitTime));
	timer.wait();
*/
	boost::this_thread::sleep( boost::posix_time::seconds(waitTime));
	OSDLOG(INFO, "Blocking wait over.");
	return 1;
}

int OsmTimer::nonBlockingWait(int waitTime)
{
	boost::asio::deadline_timer timer(this->io, boost::posix_time::seconds(waitTime));
	timer.async_wait(&doTaskWhileWaiting);
	return 1;
}

