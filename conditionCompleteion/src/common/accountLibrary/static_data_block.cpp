#include "accountLibrary/static_data_block.h"

namespace accountInfoFile
{

void StaticDataBlock::add(uint64_t ROWID, std::string name)
{
	this->static_data_.push_back(StaticDataBlockRecord(ROWID, name));
}

uint64_t StaticDataBlock::get_size() const
{
	return this->static_data_.size();
}

}
