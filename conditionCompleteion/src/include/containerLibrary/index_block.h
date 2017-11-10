#ifndef __INDEX_BLOCK_H__
#define __INDEX_BLOCK_H__

#include <boost/functional/hash.hpp>
#include <string>
#include <vector>

// this section need clean up, hash and its function poorly implemented

namespace containerInfoFile
{


class IndexBlock
{
public:
	struct IndexRecord
	{
		IndexRecord(std::string const& hash, uint64_t file_state, uint64_t size):
			hash(hash), file_state(file_state), size(size)
		{
			;
		}

		std::string hash;
		uint64_t file_state;
		uint64_t size;
	};

	typedef std::vector<IndexRecord> index_record_list;
	typedef std::vector<IndexRecord>::iterator index_record_iterator;

	bool exists(std::string const & hash);
	bool is_deleted(std::string const & hash);
	bool is_modified(std::string const & hash);
	void add(std::string const & hash, uint64_t file_state, uint64_t size);
	void append(std::string const & hash, uint64_t file_state, uint64_t size);
	void add(IndexRecord const & record);
	void set_file_state(std::string const & hash, uint64_t file_state);
	uint64_t get_file_state(std::string const & hash);
	uint64_t get_obj_size(std::string const & hash);
	IndexRecord get(std::string const & hash);
	void remove(std::string const & hash);
	uint64_t get_size() const;
	static std::string get_hash(std::string const & acc_name, const std::string & cont_name, const std::string & obj_name);

	// HACK find a better method for builder access
	const std::vector<IndexRecord>::iterator get_iterator()
	{
		return this->index_.begin();;
	}

private:
	std::vector<IndexRecord> index_;
	std::vector<IndexRecord>::iterator find(std::string hash);
	boost::hash<std::string> name_hash;
};

}

#endif
