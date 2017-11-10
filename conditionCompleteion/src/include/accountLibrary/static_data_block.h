#ifndef __STATIC_DATA_BLOCK_H__
#define __STATIC_DATA_BLOCK_H__

#include <string>
#include <vector>
#include <string.h>

namespace accountInfoFile
{

class StaticDataBlock
{
public:
	struct StaticDataBlockRecord
	{
		StaticDataBlockRecord(uint64_t ROWID, std::string name): ROWID(ROWID), name(name)
		{
			;
		}

		uint64_t ROWID;
		std::string name;
	};


	void add(uint64_t ROWID, std::string name);

	uint64_t get_size() const;

	const std::vector<StaticDataBlockRecord>::iterator get_iterator()
	{
		return this->static_data_.begin();
	}

private:
	std::vector<StaticDataBlockRecord> static_data_;
};

}

#endif
