#ifndef __NON_STATIC_DATA_BLOCK_H__
#define __NON_STATIC_DATA_BLOCK_H__

#include <string>
#include <vector>
//#include <map>

namespace accountInfoFile
{

class NonStaticDataBlock
{
public:
	struct NonStaticDataBlockRecord
	{
		NonStaticDataBlockRecord(
			std::string hash,
			std::string put_timestamp,
			std::string delete_timestamp,
			uint64_t object_count,
			uint64_t bytes_used,
			bool deleted ):
			hash(hash),
			put_timestamp(put_timestamp),
			delete_timestamp(delete_timestamp),
			object_count(object_count),
			bytes_used(bytes_used),
			deleted(deleted)
		{
			;
		}
		std::string hash;
		std::string put_timestamp;
		std::string delete_timestamp;
		uint64_t object_count;
		uint64_t bytes_used;
		bool deleted;  // TODO: enum use instead of bool
		};



	std::vector<NonStaticDataBlockRecord>::iterator find(std::string hash);
	void add(std::string hash, std::string put_timestamp, std::string delete_timestamp,
			uint64_t object_count, uint64_t bytes_used, bool deleted = false);

	uint64_t get_size() const;
	
	const std::vector<NonStaticDataBlockRecord>::iterator get_iterator()
	{
		return this->non_static_data_.begin();
	}

	const std::vector<NonStaticDataBlockRecord>::iterator get_end_iterator()
	{
		return this->non_static_data_.end();
	}

private:
	std::vector<NonStaticDataBlockRecord> non_static_data_;
};


typedef std::vector<accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator non_static_data_block_itr;

}

#endif
