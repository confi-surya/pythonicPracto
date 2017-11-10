#ifndef __RECORD_STRUCTURE_H__
#define __RECORD_STRUCTURE_H__

#include <string>
#include <map>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>

#include "cool/cool.h"

namespace recordStructure
{

class AccountStat
{

public:
	std::string account;
	std::string created_at;
	std::string put_timestamp;
	std::string delete_timestamp;
	uint64_t container_count;
	uint64_t object_count;
	uint64_t bytes_used;
	std::string hash;
	std::string id;
	std::string status;
	std::string status_changed_at;
	std::map<std::string, std::string>  metadata;

	AccountStat();
	
	AccountStat(AccountStat const &copy);

	AccountStat(std::string account, std::string created_at, std::string put_timestamp, std::string delete_timestamp, 
			uint64_t container_count, uint64_t object_count, uint64_t bytes_used, std::string hash, std::string id,
			std::string status, std::string status_changed_at, boost::python::dict metadata);
	
	AccountStat(std::string account, std::string created_at, std::string put_timestamp, std::string delete_timestamp,
                        uint64_t container_count, uint64_t object_count, uint64_t bytes_used, std::string hash, std::string id,
                        std::string status, std::string status_changed_at, std::map<std::string, std::string>  metadata);

	std::string get_account();
	std::string get_created_at();
	std::string get_put_timestamp();
	std::string get_delete_timestamp();
	uint64_t get_container_count();
	uint64_t get_object_count();
	uint64_t get_bytes_used();
	std::string get_hash();
	std::string get_id();
	std::string get_status();
	std::string get_status_changed_at();
	boost::python::dict get_metadata();

	void set_container_count(uint64_t container_count);
	void set_object_count(uint64_t object_count);
	void set_bytes_used(uint64_t bytes_used);
	void set_metadata(boost::python::dict arg_metadata);

};



class ContainerRecord
{
public:
	uint64_t ROWID;
	std::string name;
	std::string hash;
	std::string put_timestamp;
	std::string delete_timestamp;
	uint64_t object_count;
	uint64_t bytes_used;
	bool deleted;

	ContainerRecord();
	ContainerRecord(uint64_t ROWID, std::string name, std::string hash, std::string put_timestamp, std::string delete_timestamp,  
			uint64_t object_count, uint64_t bytes_used, bool deleted);

	uint64_t get_ROWID();
	std::string get_name();
	std::string get_hash();
	std::string get_put_timestamp();
	std::string get_delete_timestamp();
	uint64_t get_object_count();
	uint64_t get_bytes_used();
	bool get_deleted();

	void set_name(std::string name);

};

}

#endif

