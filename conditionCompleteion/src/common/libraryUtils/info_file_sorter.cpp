#include <iostream>
#include "libraryUtils/osd_logger_iniliazer.h"
#include "cool/debug.h"
#include "libraryUtils/info_file_sorter.h"

namespace infoFileSorter
{

InfoFileSorter::InfoFileSorter(std::vector<InfoFileIterator *> iterators,
		std::string marker, std::string end_marker, bool prefix)
	: iterators(iterators), end_marker(end_marker)
{
	std::vector<InfoFileIterator *>::const_iterator iter = this->iterators.begin();
	for (uint32_t i = 0; i < this->iterators.size(); i ++)
	{
		InfoFileRecord const * record = this->iterators[i]->next();
		if (!marker.empty())
		{
			//if(record) OSDLOG(INFO, "record name next 1 "<<record->get_name());
			while((record && prefix && ::strncmp(marker.c_str(), record->get_name(), marker.length()) != 0)
				|| (record && !prefix && (::strncmp(marker.c_str(), record->get_name(), marker.length()) > 0)))
			//		|| (record && !prefix && ::strncmp(marker.c_str(), record->get_name(), marker.length()) < 0))
			{
				record = this->iterators[i]->next();
				//if(record) OSDLOG(INFO, "record name next 1 "<<record->get_name()<<" come "<<prefix <<" oun "<<
					//::strncmp(marker.c_str(), record->get_name(), marker.length()));
			}  
			if(!prefix && record && ::strncmp(marker.c_str(), record->get_name(), marker.length()) == 0 
					&& ::strlen(record->get_name()) == marker.length())
			{
				record = this->iterators[i]->next();
				//if(record) OSDLOG(INFO, "record name next back 1 "<<record->get_name());
			}
		}
		this->records.push_back(record);
	}
	::memset(this->target_name, 0, sizeof(this->target_name));
	/*  if(!marker.empty())
	{
		for (uint32_t i = 0; i < this->iterators.size(); i ++)
		{
			if (this->records[i] && (::strnlen(this->target_name, 256) == 0 ||
					::strncmp(this->target_name, this->records[i]->get_name(), 256) >= 0))
			{
				::strncpy(this->target_name, this->records[i]->get_name(), 256);
				DASSERT(::strnlen(this->target_name, 256), "must have name");
			}
		}
	}*/
}

InfoFileRecord const * InfoFileSorter::next()
{
	//OSDLOG(INFO, "target name "<<this->target_name);
	if (::strnlen(this->target_name, 256))
	{
		for (uint32_t i = 0; i < this->iterators.size(); i ++)
		{
			if (this->records[i] && ::strncmp(this->target_name, this->records[i]->get_name(), 256) == 0)
			{
				//OSDLOG(INFO, "record name next 1 "<<records[i]->get_name());
				this->records[i] = this->iterators[i]->next();
				//if(records[i]) OSDLOG(INFO, "record name next 2 "<<records[i]->get_name());
			}
		}
	}

	InfoFileRecord const * record = NULL;
	::memset(this->target_name, 0, sizeof(this->target_name));
	for (uint32_t i = 0; i < this->iterators.size(); i ++)
	{
		if (this->records[i] && (::strnlen(this->target_name, 256) == 0 ||
				::strncmp(this->target_name, this->records[i]->get_name(), 256) >= 0))
		{
			::strncpy(this->target_name, this->records[i]->get_name(), 256);
			DASSERT(::strnlen(this->target_name, 256), "must have name");
			record = this->records[i];
			//if(records[i]) OSDLOG(INFO, "record name next in 2 loog 1 "<<records[i]->get_name());
		}
	}

	if (!this->end_marker.empty() && ::strncmp(this->end_marker.c_str(), this->target_name, 256) <= 0)
	{
		return NULL;
	}

	//if(record) OSDLOG(INFO, "returning record; "<<record->get_name());
	return record;
}

}
