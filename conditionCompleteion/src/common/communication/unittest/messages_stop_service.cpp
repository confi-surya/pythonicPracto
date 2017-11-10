#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include <map>
#include "communication/message.h"
#include "communication/communication.h"
#include "communication/message_interface_handlers.h"

namespace stop_service_message
{

TEST(http_stop_service_test, send_http_request)
{

	Communication::SynchronousCommunication comm(ip, port);
	std::map<std::string, std::string> dict;
	dict["X-GLOBAL-MAP-VERSION"] = "-1";
	comm_messages::BlockRequests req(dict);
	uint32_t timeout = 60;
	obj = MessageExternalInterface::send_stop_service_http_request(&comm, &req, ip, port, timeout);
}

}
