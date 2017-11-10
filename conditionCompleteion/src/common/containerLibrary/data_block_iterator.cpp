/*
 * =====================================================================================
 *
 *       Filename:  data_block_builder.cpp
 *
 *    Description:  
 *
 *	Version:  1.0
 *	Created:  08/06/2014 10:30:17 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *	 Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "libraryUtils/object_storage_log_setup.h"
#include "containerLibrary/data_block_iterator.h"
#include "containerLibrary/data_block_builder.h"
#include "libraryUtils/object_storage.h"
#include <iostream>

namespace containerInfoFile
{

ContainerInfoFileRecord::ContainerInfoFileRecord()
{
}

ContainerInfoFileRecord::~ContainerInfoFileRecord()
{
}

void ContainerInfoFileRecord::set(DataRecord const * data_record,
		IndexRecord const * index_record)
{
	this->data_record = data_record;
	this->index_record = index_record;
}

bool ContainerInfoFileRecord::is_deleted() const
{
	return (this->data_record->deleted == OBJECT_DELETE) | (this->data_record->deleted == OBJECT_UNKNOWN_DELETE) ? true : false;
}

char const * ContainerInfoFileRecord::get_name() const
{
	return this->data_record->name;
}

recordStructure::ObjectRecord ContainerInfoFileRecord::get_object_record() const
{
	return recordStructure::ObjectRecord(uint64_t(0), this->data_record->name,
			this->data_record->created_at, this->data_record->size,
			this->data_record->content_type, this->data_record->etag);
}

DataRecord const * ContainerInfoFileRecord::get_data_record() const
{
	return this->data_record;
}

IndexRecord const * ContainerInfoFileRecord::get_index_record() const
{
	return this->index_record;
}

MemoryContainerInfoFileRecord::MemoryContainerInfoFileRecord()
{
}

MemoryContainerInfoFileRecord::~MemoryContainerInfoFileRecord()
{
}

void MemoryContainerInfoFileRecord::set(recordStructure::ObjectRecord const * record)
{
	this->record = record;
}

bool MemoryContainerInfoFileRecord::is_deleted() const
{
	return (this->record->get_deleted_flag() == OBJECT_DELETE) | (this->record->get_deleted_flag() == OBJECT_UNKNOWN_DELETE) ? true : false;
}

char const * MemoryContainerInfoFileRecord::get_name() const
{
	return this->record->get_name().c_str();
}

recordStructure::ObjectRecord MemoryContainerInfoFileRecord::get_object_record() const
{
	return *this->record;
}

SortedContainerInfoIterator::SortedContainerInfoIterator(boost::shared_ptr<Reader> reader,
		cool::Option<std::string> start_pattern)
	: reader(reader), compaction(false)
{
	OSDLOG(DEBUG, "SortedContainerInfoIterator::SortedContainerInfoIterator");
	this->max_read_index_records = MAX_READ_INDEX_RECORD; 
	this->max_read_data_records = MAX_READ_RECORD; 
	boost::shared_ptr<Trailer> trailer = this->reader->get_trailer();
	if (trailer->sorted_record == 0)
	{
		this->rest_count = 0;
	}
	else if (start_pattern.hasSome())
	{
		// TODO read data_index_block to set current_count
		this->current_count = 0;
		if (!start_pattern.getRefValue().empty())
		{
			std::vector<DataBlockBuilder::DataIndexRecord> diRecord;
			this->reader->read_data_index_block(diRecord);
			std::vector<DataBlockBuilder::DataIndexRecord>::const_iterator iter = diRecord.begin();
			for (; iter != diRecord.end(); iter ++)
			{
				if (::strncmp(start_pattern.getRefValue().c_str(), iter->name, 1024) < 0)
				{
					break;
				}
				this->current_count += objectStorage::DATA_INDEX_INTERVAL;
			}
		}
		this->rest_count = trailer->sorted_record - this->current_count;;
		this->read_data_records();
	}
	else
	{
		// compaction
		this->compaction = true;
		this->current_count = 0;
		this->rest_count = trailer->sorted_record;
		this->read_data_records();
		this->read_index_records();
	}
}

void SortedContainerInfoIterator::read_data_records(bool reverse)
{
	uint32_t read_count = std::min(this->max_read_data_records, this->rest_count);
	this->data_buffer.resize(read_count);
	if(!reverse)
	{
		this->reader->read_data_block(&this->data_buffer[0], current_count, read_count);
		this->data_iter = this->data_buffer.begin();
	}
	else
	{
                this->reader->read_data_block(&this->data_buffer[0], current_count - read_count, read_count);
                this->data_iter = this->data_buffer.end();
	}
}

void SortedContainerInfoIterator::read_index_records(bool reverse)
{
	uint32_t read_count = std::min(this->max_read_index_records, this->rest_count);
	this->index_buffer.resize(read_count);
	if(!reverse)
	{
		this->reader->read_index_block(this->index_buffer, current_count, read_count);
		this->index_iter = this->index_buffer.begin();
	}
	else
	{
		this->reader->read_index_block(this->index_buffer, current_count - read_count, read_count);
                this->index_iter = this->index_buffer.end();
	}
}

InfoFileRecord const * SortedContainerInfoIterator::next()
{
	DataRecord const * data_record = NULL;
	IndexRecord const * index_record = NULL;
	if (this->rest_count == 0)
	{
		return NULL;
	}

	if (this->data_iter == this->data_buffer.end())
	{
		this->read_data_records();
	}
	data_record = &*this->data_iter;
	this->data_iter++;

	if (this->compaction)
	{
		if (this->index_iter == this->index_buffer.end())
		{
			this->read_index_records();
		}
		index_record = &*this->index_iter;
		this->index_iter++;
	}
	this->current_count++;
	this->rest_count--;
	this->current_record.set(data_record, index_record);
	return &current_record;
}

InfoFileRecord const * SortedContainerInfoIterator::back()
{
	// figure out what to do when marker is at the begining
        DataRecord const * data_record = NULL;
        IndexRecord const * index_record = NULL;
        if (this->current_count == 0)
        {
                return NULL;
        }

        if (this->data_iter == this->data_buffer.begin())
        {
                this->read_data_records(true);
        }

        this->data_iter--;
        data_record = &*this->data_iter;

        if (this->compaction)
        {
                if (this->index_iter == this->index_buffer.begin())
                {
                        this->read_index_records(true);
                }
                this->index_iter--;
                index_record = &*this->index_iter;
        }
        this->current_count--;
        this->rest_count++;
        this->current_record.set(data_record, index_record);
        return &current_record;
}

UnsortedContainerInfoIterator::UnsortedContainerInfoIterator(boost::shared_ptr<Reader> reader)
	: reader(reader)
{
	OSDLOG(INFO, "UnsortedContainerInfoIterator::UnsortedContainerInfoIterator");
	this->reader->read_unsorted_index_block(this->index_buffer);
	this->reader->read_unsorted_data_block(this->data_buffer);
	DASSERT(this->index_buffer.size() == this->data_buffer.size(), "must be same");

	std::vector<IndexRecord>::const_iterator index_iter = this->index_buffer.begin();
	std::vector<DataRecord>::const_iterator data_iter = this->data_buffer.begin();
	for ( ; index_iter != this->index_buffer.end(); index_iter ++, data_iter ++)
	{
		this->sorted_map[std::string((*data_iter).name)] = std::make_pair(&*data_iter, &*index_iter);
	}
	this->iter = this->sorted_map.begin();
}

InfoFileRecord const * UnsortedContainerInfoIterator::next()
{
	if (this->iter == this->sorted_map.end())
	{
		return NULL;
	}
	this->current_record.set(this->iter->second.first, this->iter->second.second);
	this->iter ++;
//std::cout << "UnsortedContainerInfoIterator::next " << this->current_record.get_name() << std::endl;
	return &current_record;
}

InfoFileRecord const * UnsortedContainerInfoIterator::back()
{
        if (this->iter == this->sorted_map.begin())
        {
                return NULL;
        }
        this->iter--;
        this->current_record.set(this->iter->second.first, this->iter->second.second);
//std::cout << "UnsortedContainerInfoIterator::next " << this->current_record.get_name() << std::endl;
        return &current_record;
}

MemoryContainerInfoIterator::MemoryContainerInfoIterator(
		boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records)
	: records(records)
{
	if (this->records)
	{
		std::vector<recordStructure::ObjectRecord>::const_iterator iter = this->records->begin();
		for (; iter != this->records->end(); iter++)
		{
			this->sorted_map[iter->get_name()] = &*iter;
		}
	}

	this->iter = this->sorted_map.begin();
}

InfoFileRecord const * MemoryContainerInfoIterator::next()
{
	if (this->iter == this->sorted_map.end())
	{
		return NULL;
	}
	this->current_record.set(this->iter->second);
	this->iter ++;
//std::cout << "MemoryContainerInfoIterator::next " << this->current_record.get_name() << std::endl;
	return &current_record;
}

InfoFileRecord const * MemoryContainerInfoIterator::back()
{
        if (this->iter == this->sorted_map.begin())
        {
                return NULL;
        }
        this->iter--;
        this->current_record.set(this->iter->second);
//std::cout << "MemoryContainerInfoIterator::next " << this->current_record.get_name() << std::endl;
        return &current_record;
}

IndexBlockIterator::IndexBlockIterator(boost::shared_ptr<Reader> reader, bool reverse): reader(reader), reverse(reverse)
{
	this->max_read_index_records = MAX_READ_INDEX_RECORD;
	boost::shared_ptr<Trailer> trailer = this->reader->get_trailer();
	if(reverse)
	{
		this->rest_count = trailer->sorted_record + trailer->unsorted_record;
		this->current_count = trailer->sorted_record + trailer->unsorted_record;
	}
	else
	{
		this->rest_count = trailer->sorted_record + trailer->unsorted_record;
		this->current_count = 0;
	}
	this->read_index_records();
}

IndexBlockIterator::~IndexBlockIterator()
{
	;
}

IndexRecord const *IndexBlockIterator::next()
{
	IndexRecord const * index_record = NULL;
	if (this->rest_count == 0)
	{
		return NULL;
	}

	if(reverse)
	{
		this->index_iter--;
		index_record = &*this->index_iter;
		this->current_count--;
		this->rest_count--;
		if(this->index_iter == this->index_buffer.begin())
			this->read_index_records();
	}
	else
	{
		if(this->index_iter == this->index_buffer.end())
			this->read_index_records();
		index_record = &*this->index_iter;
		this->index_iter++;
		this->current_count++;
		this->rest_count--;
	}
	this->current_record = *index_record;
	return &current_record;
}

IndexRecord const *IndexBlockIterator::back()
{
        IndexRecord const * index_record = NULL;
        if (this->rest_count == 0)
        {
                return NULL;
        }

        if(!reverse)
        {
                this->index_iter--;
                index_record = &*this->index_iter;
                this->current_count--;
                this->rest_count--;
                if(this->index_iter == this->index_buffer.begin())
                        this->read_index_records();
        }
        else
        {
                if(this->index_iter == this->index_buffer.end())
                        this->read_index_records();
                index_record = &*this->index_iter;
                this->index_iter++;
                this->current_count++;
                this->rest_count--;
        }
        this->current_record = *index_record;
        return &current_record;
}

void IndexBlockIterator::read_index_records()
{
	uint64_t read_count = std::min(this->max_read_index_records, this->rest_count);
	this->index_buffer.resize(read_count);
	if(reverse)
	{
		this->reader->read_index_block(this->index_buffer, current_count - read_count, read_count);
		this->index_iter = this->index_buffer.end();
	}
	else
	{
		this->reader->read_index_block(this->index_buffer, current_count, read_count);
		this->index_iter = this->index_buffer.begin();
	}
}

}

