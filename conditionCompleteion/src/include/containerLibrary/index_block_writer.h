#ifndef __INDEX_BLOCK_WRITER_H__
#define __INDEX_BLOCK_WRITER_H__

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "index_block.h"

namespace containerInfoFile
{

typedef boost::tuple<char *,uint64_t> block_buffer_size_tuple;

class IndexBlockWriter
{

public:
        block_buffer_size_tuple get_writable_block(boost::shared_ptr<IndexBlock>);

};

}

#endif
