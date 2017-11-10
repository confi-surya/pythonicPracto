#include "accountLibrary/record_structure.h"

namespace recordStructure
{
AccountStat::AccountStat()
{
	;
}

AccountStat::AccountStat(
		std::string account, std::string created_at, std::string put_timestamp,
		std::string delete_timestamp, uint64_t container_count, uint64_t object_count,
		uint64_t bytes_used, std::string hash, std::string id, std::string status,
		std::string status_changed_at, boost::python::dict metadata)
{
	this->account = account;
	this->created_at = created_at;
	this->put_timestamp = put_timestamp;
	this->delete_timestamp = delete_timestamp;
	this->container_count = container_count;
	this->object_count = object_count;
	this->bytes_used = bytes_used;
	this->hash = hash;
	this->id = id;
	this->status = status;
	this->status_changed_at = status_changed_at;
	this->set_metadata(metadata);
}

AccountStat::AccountStat(
				std::string account, std::string created_at, std::string put_timestamp,
				std::string delete_timestamp, uint64_t container_count, uint64_t object_count,
				uint64_t bytes_used, std::string hash, std::string id, std::string status,
				std::string status_changed_at, std::map<std::string, std::string> metadata)
{
		this->account = account;
		this->created_at = created_at;
		this->put_timestamp = put_timestamp;
		this->delete_timestamp = delete_timestamp;
		this->container_count = container_count;
		this->object_count = object_count;
		this->bytes_used = bytes_used;
		this->hash = hash;
		this->id = id;
		this->status = status;
		this->status_changed_at = status_changed_at;
		this->metadata = metadata;
}


AccountStat::AccountStat(AccountStat const &copy) 
{
		this->account = copy.account;
		this->created_at = copy.created_at;
		this->put_timestamp = copy.put_timestamp;
		this->delete_timestamp = copy.delete_timestamp;
		this->container_count = copy.container_count;
		this->object_count = copy.object_count;
		this->bytes_used = copy.bytes_used;
		this->hash = copy.hash;
		this->id = copy.id;
		this->status = copy.status;
		this->status_changed_at = copy.status_changed_at;
		this->metadata = copy.metadata;
}


std::string AccountStat::get_account()
{
	return this->account;
}

std::string AccountStat::get_created_at()
{
	return this->created_at;
}

std::string AccountStat::get_put_timestamp()
{
	return this->put_timestamp;
}

std::string AccountStat::get_delete_timestamp()
{
	return this->delete_timestamp;
}

uint64_t AccountStat::get_container_count()
{
	return this->container_count;
}

uint64_t AccountStat::get_object_count()
{
	return this->object_count;
}

uint64_t AccountStat::get_bytes_used()
{
	return this->bytes_used;
}

std::string AccountStat::get_hash()
{
	return this->hash;
}

std::string AccountStat::get_id()
{
	return this->id;
}

std::string AccountStat::get_status()
{
	return this->status;
}

std::string AccountStat::get_status_changed_at()
{
	return this->status_changed_at;
}

boost::python::dict AccountStat::get_metadata()
{
	boost::python::dict ret_metadata;
	for(std::map<std::string, std::string>::const_iterator it = this->metadata.begin(); it != this->metadata.end(); ++it)
	{
		ret_metadata[it->first] = it->second;
	}
	return ret_metadata;
}


void AccountStat::set_metadata(boost::python::dict arg_metadata)
{
	boost::python::list keys = arg_metadata.keys();
	for (int i = 0; i < len(keys); ++i)
	{
		boost::python::extract<std::string> extracted_key(keys[i]);
		if(!extracted_key.check())
			continue;
		std::string key = extracted_key;
		boost::python::extract<std::string> extracted_val(arg_metadata[key]);
		if(!extracted_val.check())
			continue;
		std::string value = extracted_val;
		this->metadata[key] = value;
	}
}


void AccountStat::set_container_count(uint64_t container_count)
{
	this->container_count = container_count; 
}

void AccountStat::set_object_count(uint64_t object_count)
{
	this->object_count = object_count;
}

void AccountStat::set_bytes_used(uint64_t bytes_used)
{

	this->bytes_used = bytes_used;
}

ContainerRecord::ContainerRecord()
{
	;
}

ContainerRecord::ContainerRecord(uint64_t ROWID, std::string name, std::string hash, std::string put_timestamp, std::string delete_timestamp,
				uint64_t object_count, uint64_t bytes_used, bool deleted)
{
	this->ROWID = ROWID;
	this->name = name;
	this->hash = hash;
	this->put_timestamp = put_timestamp;
	this->delete_timestamp = delete_timestamp;
	this->object_count = object_count;
	this->bytes_used = bytes_used;
	this->deleted = deleted;
}

uint64_t ContainerRecord::get_ROWID()
{
	return this->ROWID;
}

std::string ContainerRecord::get_name()
{
	return this->name;
}

std::string ContainerRecord::get_hash()
{
	return this->hash;
}

std::string ContainerRecord::get_put_timestamp()
{
	return this->put_timestamp;
}

std::string ContainerRecord::get_delete_timestamp()
{
	return this->delete_timestamp;
}

uint64_t ContainerRecord::get_object_count()
{
	return this->object_count;
}

uint64_t ContainerRecord::get_bytes_used()
{
	return this->bytes_used;
}

bool ContainerRecord::get_deleted()
{
	return this->deleted;
}

void ContainerRecord::set_name(std::string name)
{
	this->name = name;
}

}
