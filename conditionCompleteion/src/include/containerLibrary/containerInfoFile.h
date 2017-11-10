#ifndef __CONTAINER_INFO_FILE_H__
#define __CONTAINER_INFO_FILE_H__


#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

#include "reader.h"
#include "writer.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/lock_manager.h"

namespace containerInfoFile
{

static const boost::regex META_PATTERN("^m-.*");
static const boost::regex ACL_PATTERN("^[rw]-.*");
static const boost::regex SYSTEM_META_PATTERN("^sm-.*");

static const std::string META_DELETE = "";

static const uint64_t MAX_META_LENGTH = 4 * 1024; // 4KB
static const uint64_t MAX_META_KEYS = 90;
static const uint64_t MAX_ACL_LENGTH = 24 * 1024; //24KB
static const uint64_t MAX_SYSTEM_META_LENGTH = 2 * 1024;

class ContainerInfoFile
{
public:
	ContainerInfoFile(boost::shared_ptr<Reader> reader, boost::shared_ptr<Writer> writer, 
								boost::shared_ptr<lockManager::Mutex> mutex);

	void add_container(recordStructure::ContainerStat const container_stat, JournalOffset const offset);
	void flush_container(RecordList * record_list,bool unknown_status, boost::function<void(void)> const & release_flush_records, uint64_t node_id);
	void compaction(uint64_t num_record);
        //void deleteContainer();

	void get_object_list(
			std::list<recordStructure::ObjectRecord> *list,
			boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records,
			uint64_t const count, std::string const & marker, std::string const & end_marker,
			std::string const & prefix, std::string const & delimiter);

        //void get_container_stat(boost::shared_ptr<recordStructure::ContainerStat> container_stat);
        void get_container_stat(
		std::string const & path,
		recordStructure::ContainerStat* container_stat,
		boost::function<recordStructure::ContainerStat(void)> const & get_stat
		);

	void get_container_stat_after_compaction(
					std::string const & path,
					recordStructure::ContainerStat* containter_stat,
					RecordList * record_list,
					boost::function<void(void)> const & release_flush_records,
					uint8_t node_id
					);

        void set_container_stat(recordStructure::ContainerStat container_stat, bool put_timestamp_change = false);

	uint64_t get_node_id_from_journal_id(uint64_t unique_id);

private:
        boost::shared_ptr<Reader> reader;
        boost::shared_ptr<Writer> writer;
	boost::shared_ptr<lockManager::Mutex> mutex;

        boost::shared_ptr<IndexBlock> index_block;
        boost::shared_ptr<recordStructure::ContainerStat> container_stat;
        boost::shared_ptr<Trailer> trailer;


	void read_stat_trailer();
};

}

#endif

