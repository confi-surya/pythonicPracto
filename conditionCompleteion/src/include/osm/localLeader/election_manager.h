#ifndef __election_manager__
#define __election_manager__
# include <map>
#include <iostream>
#include "osm/osmServer/hfs_checker.h"

#include "osm/osmServer/file_handler.h"
#include "libraryUtils/config_file_parser.h"
/*
#define Te_file_wait_TIMEOUT 10
#define T_el_TIMEOUT 60
#define T_notify_TIMEOUT 60
#define T_p_el_TIMEOUT 140//T_p_el_TIMEOUT = T_el_TIMEOUT + T_notify_TIMEOUT + delta
*/
#define DELTA 20
#define ELECTION_RECORD_SIZE 24
#define DEFAULT_CONNECTIVITY_COUNT 0

using namespace hfs_checker;

class ElectionManager
{
public:
	ElectionManager(std::string, cool::SharedPtr<config_file::OSMConfigFileParser> config_parser, boost::shared_ptr<HfsChecker>);
	int start_election(std::string gl_service_id);
private:
	boost::shared_ptr<ElectionFileHandler> election_handler;
	boost::shared_ptr<LockFile> lock_file;
	boost::shared_ptr<HfsChecker> hfs_checker;
	int election_sub_version;
	std::string gl_version;
	std::string gfs_path;
	std::string id;
	std::string exportPath;
	std::string node_info_file_path;
	uint16_t te_file_wait_timeout;
	uint16_t t_el_timeout;
	uint16_t new_GL_response_timeout;
	uint16_t t_notify_timeout;
	uint16_t t_p_el_timeout;	
	void proceed_as_coordinator(std::string gl_service_id);
	void proceed_as_participent(std::string gl_service_id);
	int gl_ownership_notification(std::string, std::string, std::string, int32_t);
	int close_election();
	bool is_gl_updated();
	int notify_leader();
};


#endif
