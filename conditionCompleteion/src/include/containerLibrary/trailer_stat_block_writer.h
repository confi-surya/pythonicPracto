#ifndef __TRAILER_STAT_BLOCK_WRITER_H__
#define __TRAILER_STAT_BLOCK_WRITER_H__

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "trailer_block.h"
#include "libraryUtils/record_structure.h"
#include "trailer_stat_block_builder.h"

namespace containerInfoFile
{

typedef boost::tuple<char *,uint64_t> block_buffer_size_tuple;

class TrailerStatBlockWriter
{

public:
        block_buffer_size_tuple get_writable_block(boost::shared_ptr<recordStructure::ContainerStat> stat, 
						boost::shared_ptr<Trailer> trailer);

};

}

#endif
