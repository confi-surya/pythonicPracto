#ifndef __DATA_BLOCK_WRITER_H__
#define __DATA_BLOCK_WRITER_H__

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "data_block.h"
#include "memory_writer.h"

namespace containerInfoFile
{

typedef boost::tuple<char *,uint64_t> block_buffer_size_tuple;
using container_library::RecordList;

class DataBlockWriter
{

public:
	block_buffer_size_tuple get_writable_block(RecordList const *);

};

}

#endif
