#ifndef __ACCOUNT_INFO_FILE_H__
#define __ACCOUNT_INFO_FILE_H__

#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <list>
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/lock_manager.h"
#include "static_data_block.h"
#include "non_static_data_block.h"
#include "trailer_block.h"

#include "reader.h"
#include "writer.h"

namespace accountInfoFile
{

class Reader;
class Writer;

static const boost::regex META_PATTERN("^m-.*");
static const boost::regex ACL_PATTERN("^[rw]-.*");
static const boost::regex SYSTEM_META_PATTERN("^sm-.*");

static const std::string META_DELETE = "";

static const uint64_t MAX_META_LENGTH = 4 * 1024; // 4KB
static const uint64_t MAX_META_KEYS = 90;
static const uint64_t MAX_ACL_LENGTH = 24 * 1024; //24KB
static const uint64_t MAX_SYSTEM_META_LENGTH = 2 * 1024;

using StatusAndResult::Status;

static const uint64_t MAX_UNSORTED_RECORD = 5000; // set MAX_UNSORTED_RECORD = 10000;

class AccountInfoFile
{
public:
	AccountInfoFile(boost::shared_ptr<Reader> reader, boost::shared_ptr<Writer> writer, 
				boost::shared_ptr<lockManager::Mutex> mutex);
	AccountInfoFile(boost::shared_ptr<Reader> reader, boost::shared_ptr<lockManager::Mutex> mutex);
	boost::shared_ptr<recordStructure::AccountStat> get_account_stat();
	void set_account_stat(boost::shared_ptr<recordStructure::AccountStat> account_stat, bool second_put_case = false);
	void create_account(boost::shared_ptr<recordStructure::AccountStat> account_stat);
	void add_container(boost::shared_ptr<recordStructure::ContainerRecord> container_record);
	void delete_container(boost::shared_ptr<recordStructure::ContainerRecord> container_record);
	void update_container(std::list<recordStructure::ContainerRecord> container_records);
	std::list<recordStructure::ContainerRecord> list_container(uint64_t const count, std::string const &marker, 
		string const &end_marker, std::string const &prefix, std::string const &delimiter);

private:
	void add_container_internal(std::list<recordStructure::ContainerRecord> container_records);
	void create_stat_block(boost::shared_ptr<recordStructure::AccountStat> account_stat);
	void read_stat_trailer();
	bool update_record(recordStructure::ContainerRecord & source,
			NonStaticDataBlockBuilder::NonStaticBlock_ & target);
	void compaction();

	boost::shared_ptr<Reader> reader;
	boost::shared_ptr<Writer> writer;
	boost::shared_ptr<lockManager::Mutex> mutex;

	boost::shared_ptr<StaticDataBlock> static_data_block;
	boost::shared_ptr<NonStaticDataBlock> non_static_data_block;
	boost::shared_ptr<recordStructure::AccountStat> stat_block;
	boost::shared_ptr<TrailerBlock> trailer_block;

};

}

#endif
