#define GTEST_HAS_TR1_TUPLE 0
#include <stdexcept>
#include "gtest/gtest.h"
#include "libraryUtils/osd_logger_iniliazer.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include "communication/communication.h"
#include "boost/shared_ptr.hpp"
#include "libraryUtils/object_storage_log_setup.h"

namespace communication_test
{

void fun()
{
	int i;
	std::cin >> i;
}

TEST(CommunicationConstructorTest, default_constructor_testing)
{
        boost::shared_ptr<OSD_Logger::OSDLoggerImpl>logger;
        logger.reset(new OSD_Logger::OSDLoggerImpl("osm"));
	logger->initialize_logger();
	boost::shared_ptr<Communication::SynchronousCommunication>  com(new Communication::SynchronousCommunication("192.168.123.13", 1234));
	OSDLOG(INFO, "reseting comm");
	//com.reset();
//	exit(1);
//	std::cout << "destroyed";
	//boost::thread t(boost::bind(fun));
	//t.join();
//	int i;
/*	std::cout << "is connected : " << com->is_connected();
	std::cout << "\n" << true;
	//std::cin >> i;*/
}

}
