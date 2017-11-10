#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
//#include <boost/range/algorithm/find_if.hpp>
#include <algorithm>
#include "accountLibrary/non_static_data_block.h"

namespace accountInfoFile
{

void NonStaticDataBlock::add(
			std::string hash,
			std::string put_timestamp,
			std::string delete_timestamp,
			uint64_t object_count,
			uint64_t bytes_used,
			bool deleted)
{
		this->non_static_data_.push_back(NonStaticDataBlockRecord(hash, put_timestamp, delete_timestamp,
										object_count, bytes_used, deleted));
}
//UNCOVERED_CODE_BEGIN:
std::vector<NonStaticDataBlock::NonStaticDataBlockRecord>::iterator NonStaticDataBlock::find(std::string hash)
{
	std::vector<NonStaticDataBlock::NonStaticDataBlockRecord>::reverse_iterator r_it = std::find_if(this->non_static_data_.rbegin(), this->non_static_data_.rend(),
				boost::lambda::bind(&NonStaticDataBlock::NonStaticDataBlockRecord::hash, boost::lambda::_1) == hash);

	if(r_it != this->non_static_data_.rend())
	{
		return (r_it + 1).base();
	}
	else
	{
		return this->non_static_data_.end(); // return end() to caller if element is not present
	}

}
//UNCOVERED_CODE_END
uint64_t NonStaticDataBlock::get_size() const
{
	return this->non_static_data_.size();
}

}
