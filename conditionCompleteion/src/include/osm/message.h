#ifndef __MESSAGE_999__
#define __MESSAGE_999__

#include <string>
#include "libraryUtils/record_structure.h"

namespace message
{

class Message
{
	public:
		int32_t get_fd();
		std::string get_service_id();
		std::string get_node_id();
		int get_msg_type();
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > get_transferred_comp_list();
		std::string service_id;
		std::string node_id;
		std::vector<std::pair<int32_t, bool> > get_final_comp_status();
		int type;
		int32_t fd;
};

class GSRMMessage : public Message
{
	public:
		GSRMMessage(std::string service_id, int32_t fd, int type);
};

class GNAMMessage : public Message
{
	public:
		GNAMMessage(std::string service_id, int32_t fd, int type);
	
};

class GNDMMessage : public Message
{
	public:
		GNDMMessage(std::string service_id, int32_t fd, int type);
};

class LHBTMessage : public Message
{
	public:
		LHBTMessage(std::string service_id, int32_t fd, int type);
};

class STRMMessage: public Message
{
	public:
		STRMMessage(std::string service_id, int32_t fd, int type);	
};

class NDADMessage: public Message
{
	public:
		NDADMessage(std::string service_id, int32_t fd, int type);

};

} // namespace message

#endif


