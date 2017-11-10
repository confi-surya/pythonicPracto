#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class OsmTimer
{
public:
	OsmTimer();
	~OsmTimer();
	int blockingWait(int waitTime);
	int nonBlockingWait(int waitTime);

private:
	static int doTaskWhileWaiting(const boost::system::error_code&);
	boost::asio::io_service io;

};

