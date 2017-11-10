#ifndef __TRAILER_BLOCK_WRITER_H__
#define __TRAILER_BLOCK_WRITER_H__

#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

#include "trailer_block.h"
#include "record_structure.h"

namespace accountInfoFile
{

typedef boost::tuple<char *,uint64_t> block_buffer_size_tuple;

class TrailerStatBlockWriter
{

public:
	block_buffer_size_tuple get_writable_block(boost::shared_ptr<recordStructure::AccountStat> account_stat, 
									boost::shared_ptr<TrailerBlock> trailer_block);

};

}

#endif
