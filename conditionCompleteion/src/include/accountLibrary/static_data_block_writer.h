#ifndef __STATIC_DATA_BLOCK_WRITER_H__
#define __STATIC_DATA_BLOCK_WRITER_H__

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "static_data_block.h"

namespace accountInfoFile
{

typedef boost::tuple<char *,uint64_t> block_buffer_size_tuple;

class StaticDataBlockWriter
{

public:
	block_buffer_size_tuple get_writable_block(boost::shared_ptr<StaticDataBlock> static_data_block);
};

}

#endif
