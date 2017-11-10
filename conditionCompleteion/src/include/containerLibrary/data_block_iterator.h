#ifndef __DATA_BLOCK_ITERATOR_H__
#define __DATA_BLOCK_ITERATOR_H__

#include <vector>
#include "cool/option.h"
#include "libraryUtils/info_file_sorter.h"
#include "data_block.h"
#include "containerLibrary/reader.h"

#define MAX_READ_INDEX_RECORD 100000
#define MAX_READ_RECORD 1000

namespace containerInfoFile
{

using namespace infoFileSorter;

typedef DataBlockBuilder::ObjectRecord_ DataRecord;
typedef IndexBlockBuilder::IndexRecord IndexRecord;

class ContainerInfoFileRecord : public InfoFileRecord
{
public:
	ContainerInfoFileRecord();
	~ContainerInfoFileRecord();
	void set(DataRecord const * data_record, IndexRecord const * index_record);
	bool is_deleted() const;
	char const * get_name() const;
	recordStructure::ObjectRecord get_object_record() const;
	DataRecord const * get_data_record() const;
	IndexRecord const * get_index_record() const;
private:
	DataRecord const * data_record;
	IndexRecord const * index_record;
};

class MemoryContainerInfoFileRecord : public InfoFileRecord
{
public:
	MemoryContainerInfoFileRecord();
	~MemoryContainerInfoFileRecord();
	void set(recordStructure::ObjectRecord const * record);
	bool is_deleted() const;
	char const * get_name() const;
	recordStructure::ObjectRecord get_object_record() const;
private:
	recordStructure::ObjectRecord const * record;
};

class ContainerInfoFileIndexRecord : public InfoFileRecord
{
public:
        ContainerInfoFileIndexRecord();
        ~ContainerInfoFileIndexRecord();
        void set(IndexRecord const * index_record);
        bool is_deleted() const;
        char const * get_name() const;
        IndexRecord const * get_index_record() const;
private:
        IndexRecord const * index_record;
};

class SortedContainerInfoIterator : public InfoFileIterator
{
public:
	SortedContainerInfoIterator(boost::shared_ptr<Reader> reader,
			cool::Option<std::string> start_pattern = cool::Option<std::string>());
	InfoFileRecord const * next();
	InfoFileRecord const * back();
private:
	void read_index_records(bool reverse = false);
	void read_data_records(bool reverse = false);

	boost::shared_ptr<Reader> reader;
	uint32_t current_count;
	uint32_t rest_count;
	bool compaction;
	uint32_t max_read_index_records;
	uint32_t max_read_data_records;
	boost::shared_ptr<containerInfoFile::DataBlockBuilder> data_block_reader;
	std::vector<DataRecord> data_buffer;
	std::vector<IndexRecord> index_buffer;
	std::vector<DataRecord>::const_iterator data_iter;
	std::vector<IndexRecord>::const_iterator index_iter;
	ContainerInfoFileRecord current_record;
};

class UnsortedContainerInfoIterator : public InfoFileIterator
{
public:
	UnsortedContainerInfoIterator(boost::shared_ptr<Reader> reader);
	InfoFileRecord const * next();
	InfoFileRecord const * back();
private:
	boost::shared_ptr<Reader> reader;
	std::vector<DataRecord> data_buffer;
	std::vector<IndexRecord> index_buffer;

	typedef std::map<std::string, std::pair<DataRecord const *, IndexRecord const *> > SortedMap;
	SortedMap sorted_map;
	SortedMap::const_iterator iter;
	ContainerInfoFileRecord current_record;
};

class MemoryContainerInfoIterator : public InfoFileIterator
{
public:
	MemoryContainerInfoIterator(boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records);
	InfoFileRecord const * next();
	InfoFileRecord const * back();
private:
	boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records;
	typedef std::map<std::string, recordStructure::ObjectRecord const *> SortedMap;
	SortedMap sorted_map;
	SortedMap::const_iterator iter;
	MemoryContainerInfoFileRecord current_record;
};

struct IndexBlockIterator
{
public:
	IndexBlockIterator(boost::shared_ptr<Reader> reader, bool reverse = false);
	~IndexBlockIterator();
	IndexRecord const *next();
	IndexRecord const *back();

private:
	void read_index_records();

	boost::shared_ptr<Reader> reader;
	uint64_t rest_count;
	uint64_t current_count;
        uint64_t max_read_index_records;
	std::vector<IndexRecord> index_buffer;
        std::vector<IndexRecord>::const_iterator index_iter;
	IndexRecord current_record;
	bool reverse;

};

}
#endif
