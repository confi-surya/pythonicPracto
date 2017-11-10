#ifndef __DATA_BLOCK_BUILDER_H__
#define __DATA_BLOCK_BUILDER_H__

#include <stdint.h>
#include <string.h>

#include "config.h"
namespace containerInfoFile
{

class DataBlockBuilder
{

public:
	struct ObjectRecord_
	{
		uint64_t row_id;
		char name[1024 + 1];
		char created_at[TIMESTAMP_LENGTH + 1];
		uint64_t size;
		char content_type[256 + 1];
		char etag[32 + 1];
		//bool deleted;
		int deleted;	//TODO GM: Get it verified from Mannu san
	};

	struct DataIndexRecord
	{
		DataIndexRecord()
		{
		}
		DataIndexRecord(char const * buf)
		{
			::memcpy(name, buf, 1024);
		}
		char name[1024];
	};
};

}

#endif
