#include <iostream>
#include <vector>
#include <utility>
#include <map>
#include <boost/shared_ptr.hpp>
#include "communication/message_format_handler.h"
#include "communication/message_result_wrapper.h"
#include "communication/communication.h"
#include "libraryUtils/record_structure.h"

void success(boost::shared_ptr<MessageResultWrap> result){
        std::cout << "success" << std::endl;
}

void failure(){
        std::cout << "failed" << std::endl;
}


int main(){
	std::map<std::string, std::string> dict;
	dict["X-GLOBAL-MAP-VERSION"] = "1.0";
	comm_messages::BlockRequests * msg = new comm_messages::BlockRequests(dict);

	ProtocolStructure::MessageFormatHandler msg_handler;
	Communication::SynchronousCommunication * com = new Communication::SynchronousCommunication("192.168.123.13", 3121);

	const std::string ip = "192.168.123.13";
	uint32_t timeout = 0;
	const uint32_t port = 3121;

	msg_handler.send_http_request(com, msg, ip, port, "STOP_SERVICE", success, failure, timeout);

	boost::shared_ptr<MessageResultWrap> m = msg_handler.get_msg();
	std::cout << "\n type : " << m->get_type() << std::endl;
	delete com;
	delete msg;
}
