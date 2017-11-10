/*
 * =====================================================================================
 *
 *	   Filename:  data_block_builder.cpp
 *
 *	Description:  
 *
 *		Version:  1.0
 *		Created:  08/06/2014 10:30:17 AM
 *	   Revision:  none
 *	   Compiler:  gcc
 *
 *		 Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "accountLibrary/static_data_block_builder.h"
#include "accountLibrary/account_record_iterator.h"
#include "accountLibrary/non_static_data_block_builder.h"
#include "libraryUtils/object_storage.h"
#include "libraryUtils/osd_logger_iniliazer.h"
#include <iostream>
#include <cool/debug.h>

namespace accountInfoFile
{

AccountInfoFileRecord::AccountInfoFileRecord()
{
}

AccountInfoFileRecord::~AccountInfoFileRecord()
{
}

void AccountInfoFileRecord::set(StaticRecord const * static_record,
		NonStaticRecord const * non_static_record)
{
	this->static_record = static_record;
	this->non_static_record = non_static_record;
}

bool AccountInfoFileRecord::is_deleted() const
{
	return this->non_static_record->deleted;
}

char const * AccountInfoFileRecord::get_name() const
{
	return this->static_record->name;
}

recordStructure::ContainerRecord AccountInfoFileRecord::get_object_record() const // TO DO merge record
{
	return recordStructure::ContainerRecord(
					this->static_record->ROWID,
					this->static_record->name,
					this->non_static_record->hash,
					this->non_static_record->put_timestamp,
					this->non_static_record->delete_timestamp,
					this->non_static_record->object_count,
					this->non_static_record->bytes_used,
					this->non_static_record->deleted);
}

StaticRecord const * AccountInfoFileRecord::get_static_record() const
{
	return this->static_record;
}

NonStaticRecord const * AccountInfoFileRecord::get_non_static_record() const
{
	return this->non_static_record;
}

SortedAccountInfoIterator::SortedAccountInfoIterator(boost::shared_ptr<Reader> reader, 
		cool::Option<std::string> start_pattern)
	: reader(reader)
{
	this->max_read_static_records = MAX_READ_STATIC_RECORD;
	this->max_read_non_static_records = MAX_READ_NON_STATIC_RECORD;
	boost::shared_ptr<TrailerBlock> trailer = this->reader->get_trailer();
	//OSDLOG(INFO, "offsets "<< trailer->sorted_record <<" "<< trailer->unsorted_record);
	if (trailer->sorted_record == 0)
	{
		this->rest_count = 0;
	}
	//UNCOVERED_CODE_BEGIN:
	else if (start_pattern.hasSome())
	{
		// TODO read data_index_block to set current_count
		this->current_count = 0;
		this->rest_count = trailer->sorted_record - this->current_count;;
		this->read_static_records();
		this->read_non_static_records();
		if (!start_pattern.getRefValue().empty())
		{
			
			InfoFileRecord const *record = this->next();
			//OSDLOG(INFO, "SortedContainerInfoIterator:: record name " << record->get_name()<<" "<< start_pattern.getRefValue()<< " end");
			while(record && ::strncmp(start_pattern.getRefValue().c_str(), record->get_name(), start_pattern.getRefValue().length()) > 0)
			{
				record = this->next();
				//if(record) OSDLOG(INFO, "ture till now "<< record->get_name()<<" " <<::strncmp(start_pattern.getRefValue().c_str(), record->get_name(), start_pattern.getRefValue().length()));
			}
			// roll back iterator one position
			this->static_iter--;
			this->non_static_iter--;
			this->current_count--;
			this->rest_count = trailer->sorted_record - this->current_count;
		}
	}
	//UNCOVERED_CODE_END
	else
	{
		// compaction
		this->current_count = 0;
		this->rest_count = trailer->sorted_record;
		this->read_static_records();
		this->read_non_static_records();
	}
}

void SortedAccountInfoIterator::read_static_records(bool reverse)
{
	uint64_t read_count = std::min(this->max_read_static_records, this->rest_count);
	this->static_buffer.resize(read_count);
	if(!reverse)
	{
		this->reader->read_static_data_block(&this->static_buffer[0], current_count, read_count);
		this->static_iter = this->static_buffer.begin();
	}
	else
	{
		this->reader->read_static_data_block(&this->static_buffer[0], current_count - read_count, read_count);
		this->static_iter = this->static_buffer.end();
	}
}

void SortedAccountInfoIterator::read_non_static_records(bool reverse)
{
	//OSDLOG(INFO, "SortedContainerInfoIterator::read_non_static_records rest conut" << this->max_read_non_static_records <<" "<< this->rest_count);
	uint64_t read_count = std::min(this->max_read_non_static_records, this->rest_count);
	this->non_static_buffer.resize(read_count);
	if(!reverse)
	{
		//OSDLOG(INFO, "SortedContainerInfoIterator::read_non_static_records " << current_count <<" "<< read_count);
		this->reader->read_non_static_data_block(&this->non_static_buffer[0], current_count, read_count);
		this->non_static_iter = this->non_static_buffer.begin();
	}
	else
	{
		//OSDLOG(INFO, "SortedContainerInfoIterator::read_non_static_records reverse " << current_count <<" "<< read_count);
		this->reader->read_non_static_data_block(&this->non_static_buffer[0], current_count - read_count, read_count);
		this->non_static_iter = this->non_static_buffer.end();
	}
}

InfoFileRecord const * SortedAccountInfoIterator::next()
{
	StaticRecord const * static_record = NULL;
	NonStaticRecord const * non_static_record = NULL;
	if (this->rest_count == 0)
	{
		return NULL;
	}

	if (this->static_iter == this->static_buffer.end())
		this->read_static_records();
	if (this->non_static_iter == this->non_static_buffer.end())
		this->read_non_static_records();

	static_record = &*this->static_iter;
	this->static_iter++;
	non_static_record = &*this->non_static_iter;
	this->non_static_iter++;

	this->current_count ++;
	this->rest_count--;
	this->current_record.set(static_record, non_static_record);
	return &current_record;
}

InfoFileRecord const * SortedAccountInfoIterator::back()
{
        StaticRecord const * static_record = NULL;
        NonStaticRecord const * non_static_record = NULL;
        if (this->current_count == 0)
        {
                return NULL;
        }

        if (this->static_iter == this->static_buffer.begin())
                this->read_static_records(true);
        if (this->non_static_iter == this->non_static_buffer.begin())
                this->read_non_static_records(true);

        this->static_iter--;
        static_record = &*this->static_iter;
        this->non_static_iter--;
        non_static_record = &*this->non_static_iter;

        this->current_count--;
        this->rest_count++;
        this->current_record.set(static_record, non_static_record);
        return &current_record;
}

UnsortedAccountInfoIterator::UnsortedAccountInfoIterator(boost::shared_ptr<Reader> reader)
	: reader(reader)
{
	this->reader->read_unsorted_static_data_block(this->static_buffer);
	this->reader->read_unsorted_non_static_data_block(this->non_static_buffer);
	DASSERT(this->static_buffer.size() == this->non_static_buffer.size(), "must be same");

	std::vector<StaticRecord>::const_iterator static_iter = this->static_buffer.begin();
	std::vector<NonStaticRecord>::const_iterator non_static_iter = this->non_static_buffer.begin();
	for ( ; static_iter != this->static_buffer.end(); static_iter++, non_static_iter++)
	{
		this->sorted_map[std::string((*static_iter).name)] = std::make_pair(&*static_iter, &*non_static_iter);
	}
	this->iter = this->sorted_map.begin();
}

InfoFileRecord const * UnsortedAccountInfoIterator::next()
{
	if (this->iter == this->sorted_map.end())
	{
		return NULL;
	}
	this->current_record.set(this->iter->second.first, this->iter->second.second);
	this->iter++;
	return &current_record;
}

InfoFileRecord const * UnsortedAccountInfoIterator::back()
{
        if (this->iter == this->sorted_map.begin())
        {
                return NULL;
        }
        this->iter--;
        this->current_record.set(this->iter->second.first, this->iter->second.second);
        return &current_record;
}

}
