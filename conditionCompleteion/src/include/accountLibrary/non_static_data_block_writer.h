#ifndef __NON_STATIC_DATA_BLOCK_WRITER_H__
#define __NON_STATIC_DATA_BLOCK_WRITER_H__

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "non_static_data_block.h"

namespace accountInfoFile
{

typedef boost::tuple<char *,uint64_t> block_buffer_size_tuple;

class NonStaticDataBlockWriter
{

public:
	block_buffer_size_tuple get_writable_block(boost::shared_ptr<NonStaticDataBlock> non_static_data_block);
	//block_buffer_size_tuple get_writable_map_block(boost::shared_ptr<NonStaticDataBlock> non_static_data_block);

};

}

#endif
