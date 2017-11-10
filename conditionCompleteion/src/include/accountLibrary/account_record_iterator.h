#ifndef __ACCOUNT_DATA_BLOCK_ITERATOR_H__
#define __ACCOUNT_DATA_BLOCK_ITERATOR_H__

#include <vector>
#include "cool/option.h"
#include "libraryUtils/info_file_sorter.h"
#include "accountLibrary/reader.h"

#define MAX_READ_STATIC_RECORD 1000
#define MAX_READ_NON_STATIC_RECORD 10000

namespace accountInfoFile
{

using namespace infoFileSorter;

typedef StaticDataBlockBuilder::StaticBlock_ StaticRecord;
typedef NonStaticDataBlockBuilder::NonStaticBlock_ NonStaticRecord;

class AccountInfoFileRecord : public InfoFileRecord
{
public:
	AccountInfoFileRecord();
	~AccountInfoFileRecord();
	void set(StaticRecord const * static_record, NonStaticRecord const * non_static_record);
	bool is_deleted() const;
	char const * get_name() const;
	recordStructure::ContainerRecord get_object_record() const;
	StaticRecord const * get_static_record() const;
	NonStaticRecord const * get_non_static_record() const;
private:
	StaticRecord const * static_record;
	NonStaticRecord const * non_static_record;
};

class SortedAccountInfoIterator : public InfoFileIterator
{
public:
        SortedAccountInfoIterator(boost::shared_ptr<Reader> reader,
                        cool::Option<std::string> start_pattern = cool::Option<std::string>());
        InfoFileRecord const * next();
        InfoFileRecord const * back();
private:
	void read_static_records(bool reverse = false);
	void read_non_static_records(bool reverse = false);

        boost::shared_ptr<Reader> reader;
        uint64_t current_count;
        uint64_t rest_count;
        uint64_t max_read_static_records;
        uint64_t max_read_non_static_records;
        boost::shared_ptr<accountInfoFile::StaticDataBlockBuilder> static_block_reader;
        std::vector<StaticRecord> static_buffer;
        std::vector<NonStaticRecord> non_static_buffer;
        std::vector<StaticRecord>::const_iterator static_iter;
        std::vector<NonStaticRecord>::const_iterator non_static_iter;
        AccountInfoFileRecord current_record;
};

class UnsortedAccountInfoIterator : public InfoFileIterator
{
public:
	UnsortedAccountInfoIterator(boost::shared_ptr<Reader> reader);
	InfoFileRecord const * next();
	InfoFileRecord const * back();
private:
	boost::shared_ptr<Reader> reader;
	std::vector<StaticRecord> static_buffer;
	std::vector<NonStaticRecord> non_static_buffer;

	typedef std::map<std::string, std::pair<StaticRecord const *, NonStaticRecord const *> > SortedMap;
	SortedMap sorted_map;
	SortedMap::const_iterator iter;
	AccountInfoFileRecord current_record;
};

}

#endif
