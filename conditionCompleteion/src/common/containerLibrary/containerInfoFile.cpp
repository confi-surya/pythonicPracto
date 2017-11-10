#include <boost/bind.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>
#include <algorithm>
#include <ctime>

#include "containerLibrary/containerInfoFile.h"
#include "containerLibrary/data_block_iterator.h"
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/object_storage.h"
#include <iostream>

namespace containerInfoFile
{

ContainerInfoFile::ContainerInfoFile(
		boost::shared_ptr<Reader> reader, boost::shared_ptr<Writer> writer, boost::shared_ptr<lockManager::Mutex> mutex)
	: reader(reader), writer(writer), mutex(mutex)
{
}

void ContainerInfoFile::add_container(recordStructure::ContainerStat const container_stat, JournalOffset const offset)
{
	OSDLOG(DEBUG, "calling ContainerInfoFile::add_container");
	this->trailer.reset(new containerInfoFile::Trailer(0));
	this->trailer->journal_offset = offset;
	boost::shared_ptr<recordStructure::ContainerStat> stat(new recordStructure::ContainerStat(container_stat));
	OSDLOG(DEBUG, "Stats while adding container " << stat->account << "/" << stat->container << " are: object-count- " << stat->object_count << ", bytes-used- " << stat->bytes_used);
	this->writer->write_file(stat, this->trailer);
}

void ContainerInfoFile::read_stat_trailer()
{
	OSDLOG(DEBUG, "calling ContainerInfoFile::read_stat_trailer");
	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>,
		boost::shared_ptr<containerInfoFile::Trailer> > blocks_tuple = this->reader->read_stat_trailer_block();
	this->trailer = blocks_tuple.get<1>();
	this->container_stat = blocks_tuple.get<0>();
}

void ContainerInfoFile::flush_container(
	RecordList * record_list,
	bool unknown_status,
	boost::function<void(void)> const & release_flush_records,
	uint64_t node_id
)
{
	OSDLOG(INFO, "calling ContainerInfoFile::flush_container " << node_id << " :node id ");
	this->read_stat_trailer();

	// Invoke compaction if version in the Info file is less than the current supported version.
	if ((this->trailer->version < CONTAINER_FILE_SUPPORTED_VERSION) ||
	   (this->trailer->unsorted_record + record_list->size() >= 5000)) //TODO : to start compaction thresold must be half of file extention thresold
	{
		OSDLOG(INFO, "calling ContainerInfoFile::flush_container, running compaction");
		this->compaction(record_list->size());
		this->read_stat_trailer();
	}

	//Check if journal id of last commit offset is its belong to this node itself.
	OSDLOG(DEBUG, "Journal offset is: " << this->trailer->journal_offset.first);
	OSDLOG(DEBUG, "Node id is " << this->get_node_id_from_journal_id(this->trailer->journal_offset.first));
	if ( this->get_node_id_from_journal_id(this->trailer->journal_offset.first) == node_id)
	{
	 if ( !record_list->empty() )
	   while (!record_list->empty() && record_list->begin()->first <= this->trailer->journal_offset)
	   {
	      // Only in case of recovery so we do not have to care for lock
	      OSDLOG(INFO, "calling ContainerInfoFile::flush_container, recovery running");
	      record_list->pop_front();
	   }
	}
	else
	{
	    if ( ! unknown_status)
	    {
		// Changes for OBB-676
		OSDLOG(INFO, "calling ContainerInfoFile::flush_container, Journal Id and Node Id did not match");
		OSDLOG(INFO, "calling ContainerInfoFile::flush_container, unknown_status flag is : " << unknown_status);

		IndexBlockIterator index_iterator(this->reader, true);
		IndexRecord const * index_record;
		std::map<string, uint64_t> object_record_map;
		OSDLOG(DEBUG, "Record list size is : " << record_list->size());
		while((index_record = index_iterator.next()) != NULL)
		{
		    RecordList::iterator iter = record_list->begin();
		    for (; iter != record_list->end();)
		    {
			recordStructure::ObjectRecord const & record = (*iter).second;
			string hash = this->index_block->get_hash(this->container_stat->account,
				this->container_stat->container, record.get_name());
			OSDLOG(DEBUG, "Hash of object "<< record.get_name() <<" is :" << hash);
			if ( hash == index_record->hash )
			{
			    if (record.get_deleted_flag() == index_record->file_state)
			    {
				if (object_record_map.find(hash) == object_record_map.end())
				{
				    OSDLOG(INFO, "Object removed from record List");
				    RecordList::iterator it = iter;
				    iter++;
				    record_list->erase(it);
				}
				else
				{
				    iter++;
				}
			    }
			    else
			    {
				if (object_record_map.find(hash) == object_record_map.end())
				{
				    OSDLOG(INFO, "Object inserted in map with hash : "<< hash <<"Flag : "<< record.get_deleted_flag());
				    object_record_map.insert(std::pair<string, uint64_t>(hash, record.get_deleted_flag()));
				}
				iter++;
				break;
			    }
			}
			else
			{
			    iter++;
			}
		    }
		}
	    }
	}

	if (record_list->empty())
	{	
		OSDLOG(INFO, "calling ContainerInfoFile::flush_container, record list empty");
		if(unknown_status && this->trailer->unsorted_record)
		{
			OSDLOG(INFO, "Flushing already done. Unknown Status is true, compaction start");
			this->compaction(0);
		}
		release_flush_records();
		return;   // TODO Judge whether if need compaction
	}

	// below code breaks all the parameter of sanity, need serious review and rework
	this->index_block.reset(new IndexBlock);
	RecordList::const_iterator iter = record_list->begin();
	std::map<string, recordStructure::ObjectRecord const *> unknown_status_map;
//	std::list<recordStructure::ObjectRecord const *> unknown_status_list;
	for (; iter != record_list->end(); iter++)
	{
		recordStructure::ObjectRecord const & record = (*iter).second;
//		index_block->add(record.get_name(), record.get_deleted_flag());
		std::string hash = this->index_block->get_hash(this->container_stat->account,
				this->container_stat->container, record.get_name());
                OSDLOG(DEBUG, "record name: " << record.get_name() << " string hash: "<< hash);
		// Pass the size as well to add so that it can be stored in index section & can be accessed later.
		//this->index_block->add(hash, record.get_deleted_flag());
		if (unknown_status_map.find(hash) != unknown_status_map.end())
		{
			// Only latest entry must be used.
			unknown_status_map.erase(hash);
		}
 
		uint64_t file_type_flag = record.get_deleted_flag();
		switch(file_type_flag)
		{

			case OBJECT_ADD:
				this->container_stat->bytes_used += record.get_size();
				this->container_stat->object_count++;
				break;
			case OBJECT_DELETE:
				OSDLOG(DEBUG, "Object delete while flushing in containers");
				if (this->container_stat->object_count > 0)
					this->container_stat->object_count--;
				if (this->container_stat->bytes_used >= int64_t(record.get_size()))
					this->container_stat->bytes_used -= record.get_size();
				else
					this->container_stat->bytes_used = 0;
				break;
			case OBJECT_MODIFY:
				OSDLOG(DEBUG, "OBJECT modify case while flushing in containers");
				this->container_stat->bytes_used += (record.get_size() - record.get_old_size());//check uint condition
				if(this->container_stat->bytes_used < 0)
					this->container_stat->bytes_used = 0;
				break;
			case OBJECT_UNKNOWN_DELETE:
				OSDLOG(DEBUG, "OBJECT_UNKNOWN_DELETE: " << record.get_name());
				file_type_flag = OBJECT_DELETE;
				unknown_status = true;
				break;
			case OBJECT_UNKNOWN_ADD:
				OSDLOG(DEBUG, "OBJECT_UNKNOWN_ADD: " << record.get_name());
				file_type_flag = OBJECT_ADD;
				unknown_status = true;
				break;
			default:
				OSDLOG(WARNING, "Wrong flag in object record");
				break;
				// LOG IT
		}
		this->index_block->append(hash, file_type_flag, record.get_size());
	}
	// and Knuth will be turning in his grave 
	if(unknown_status_map.size())
	{
		OSDLOG(INFO, "Unknown status list is not empty");
		IndexBlockIterator index_iterator(this->reader, true);
		IndexRecord const * record;
		while((record = index_iterator.next()) != NULL)
		{
			std::map<string, recordStructure::ObjectRecord const*>::iterator it = unknown_status_map.begin(); 
			while(it != unknown_status_map.end())
			{
				if(it->first == record->hash)
				{
					OSDLOG(DEBUG, "entry found in info file while " << it->second->get_deleted_flag() << " case");
					switch(it->second->get_deleted_flag())
					{
						case OBJECT_UNKNOWN_DELETE:  
							switch(record->file_state)
							{
								case OBJECT_ADD:
								case OBJECT_MODIFY:
									OSDLOG(DEBUG, "Object exists in Info file, removing it in case of Unknown object delete");
									if (this->container_stat->object_count > 0)
										this->container_stat->object_count--;
									if (this->container_stat->bytes_used >= int64_t(it->second->get_size()))
										this->container_stat->bytes_used -= it->second->get_size();
									else
										this->container_stat->bytes_used = 0;
//									this->container_stat->bytes_used -= it->second->get_size();
//									this->container_stat->object_count--;
									this->index_block->set_file_state(it->first, OBJECT_DELETE);
									break;
								case OBJECT_DELETE:
									this->index_block->set_file_state(it->first, OBJECT_DELETE);
									OSDLOG(DEBUG, "Object already deleted. Doing nothing in case of Unknown object delete");
									break;
								default:
									break;
							}
							unknown_status_map.erase(it++);
							break;
						case OBJECT_UNKNOWN_ADD:
							switch(record->file_state)
							{
								case OBJECT_DELETE:
									OSDLOG(DEBUG, "Object marked as deleted in Info file, adding in case of Add Object unknown");
									this->container_stat->bytes_used += it->second->get_size();
									this->container_stat->object_count++;
//									this->container_stat->bytes_used += (*it)->get_size();
//									this->container_stat->object_count++;
									this->index_block->set_file_state(it->first, OBJECT_ADD);
									break;
								case OBJECT_ADD:
								case OBJECT_MODIFY:
									// no need to manipulate bytes_used as compaction will
									// compaction will come in role after flushing
									//this->container_stat->bytes_used += record.get_size();
									OSDLOG(DEBUG, "Object already added in Info file while Add Object unknown case");
									this->index_block->set_file_state(it->first, OBJECT_MODIFY);
									break;
								default:
									break;
							}
							unknown_status_map.erase(it++);
							break;
					}
				}
				else
				{
					++it;
				}
			}
		}
	}

	if(unknown_status_map.size())
	{
		OSDLOG(INFO, "Unknown status list is not empty, some records still left whose status were not determined");
		for(std::map<string, recordStructure::ObjectRecord const*>::iterator it = unknown_status_map.begin();
				it != unknown_status_map.end(); it++)
		{
			OSDLOG(DEBUG, "Record for which no status were determined "<<it->second->get_name())
			switch(it->second->get_deleted_flag())
			{
				case OBJECT_UNKNOWN_ADD:
					this->container_stat->bytes_used += it->second->get_size();
					this->container_stat->object_count++;
					this->index_block->set_file_state(it->first, OBJECT_ADD);
					break;
				case OBJECT_UNKNOWN_DELETE:
					this->index_block->set_file_state(it->first, OBJECT_DELETE);
					break;
				default:
					break;
			}
		}
	
	}
	
	// TODO: rasingh 15/10/2014 std::time(0) timestamp is not normalized
	//this->container_stat->put_timestamp = std::time(0);
	this->trailer->unsorted_record += record_list->size();
	this->trailer->journal_offset = (--record_list->end())->first;
	this->writer->write_file(record_list, this->index_block,
							this->container_stat, this->trailer);
	if(unknown_status)
	{
		OSDLOG(INFO, "Flushing done. Unknown Status is true, compaction start");
		this->compaction(0);
	}
	release_flush_records();
	OSDLOG(DEBUG, "Write Lock released from Info file while updating container");
}

void ContainerInfoFile::compaction(uint64_t num_record)
{
	OSDLOG(INFO, "Beginning compaction at Container level for version: " << trailer->version);
	this->read_stat_trailer();
	this->container_stat->object_count =0;
	this->container_stat->bytes_used =0;
	uint64_t total_size = this->trailer->sorted_record + this->trailer->unsorted_record + num_record;
	JournalOffset j_offset = this->trailer->journal_offset;
	this->trailer.reset(new containerInfoFile::Trailer(total_size, this->trailer->version));
	this->trailer->journal_offset = j_offset;
	this->writer->begin_compaction(this->trailer, total_size);
	std::vector<InfoFileIterator *> iters;
	SortedContainerInfoIterator sorted(this->reader);
	iters.push_back(&sorted);
	UnsortedContainerInfoIterator unsorted(this->reader);
	iters.push_back(&unsorted);
	InfoFileSorter sorter(iters);

	uint64_t count = 0;
	std::vector<DataBlockBuilder::DataIndexRecord> diRecords;
	InfoFileRecord const * record;
	while((record = sorter.next()) != NULL)
	{
		if(!record->is_deleted())
		{
			ContainerInfoFileRecord const * typed_record = dynamic_cast<ContainerInfoFileRecord const *>(record);
			DataBlockBuilder::ObjectRecord_ const * data = typed_record->get_data_record();
			IndexBlockBuilder::IndexRecord const * index = typed_record->get_index_record();
			
			// For Version 0 files, Update the size field of index section from the size info in data section
			if (this->trailer->version == 0) { 
				OSDLOG(INFO, "0V file, Updating size in index section: " << data->name << " " << data->size);
				IndexBlockBuilder::IndexRecord* index_nc = const_cast<IndexBlockBuilder::IndexRecord*>(index) ;
				index_nc->size = data->size;

				// FIX OBB-840: IndexBlock and deleted/file_state must be corrected.
				string hash = IndexBlock::get_hash(this->container_stat->account,
				                this->container_stat->container, data->name);
				strncpy(index_nc->hash, hash.c_str(), sizeof(index_nc->hash));
				DataBlockBuilder::ObjectRecord_ *data_nc = const_cast<DataBlockBuilder::ObjectRecord_ *>(data);
				if (data_nc->deleted == OBJECT_UNKNOWN_ADD)
				{
				        data_nc->deleted = OBJECT_ADD;
				}
				index_nc->file_state = data_nc->deleted;
			}

			this->trailer->sorted_record ++;
			this->container_stat->object_count ++;
			this->container_stat->bytes_used += data->size;
			this->writer->write_record(data, index);

			count ++;
			if ((count % objectStorage::DATA_INDEX_INTERVAL) == 0)
			{
				diRecords.push_back(DataBlockBuilder::DataIndexRecord(typed_record->get_name()));
			}
		}
	}
	this->writer->flush_record();
	if (!diRecords.empty())
	{
		this->writer->write_data_index_block(this->trailer, diRecords);
	}
	// Update the version of the file to current_supported_version
	this->trailer->version = CONTAINER_FILE_SUPPORTED_VERSION;
	this->writer->write_file(this->container_stat, this->trailer);
	this->writer->finish_compaction();
	this->reader->reopen();
}

void ContainerInfoFile::get_object_list(
		std::list<recordStructure::ObjectRecord> *records,
		boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > memory_records,
		uint64_t const count, string const & marker, string const & end_marker,
		std::string const & prefix, std::string const & delimiter)
{
	// TODO: Do we require to convert the version 0 files to current version for read operations?
	/*
	this->read_stat_trailer();
	if (this->trailer->version < CONTAINER_FILE_SUPPORTED_VERSION) {
		this->compaction(0);
	}*/

	OSDLOG(INFO, "calling ContainerInfoFile::get_object_list, count, marker, end marker, prefix and delimiter "<< count <<","<<marker<<","<<end_marker<<","<<prefix<<","<<delimiter);
	std::vector<InfoFileIterator *> iters;
	SortedContainerInfoIterator sorted(this->reader, marker > prefix ? marker : prefix);
	iters.push_back(&sorted);
	UnsortedContainerInfoIterator unsorted(this->reader);
	iters.push_back(&unsorted);
	MemoryContainerInfoIterator memory(memory_records);
	iters.push_back(&memory);

	InfoFileSorter sorter(iters, marker > prefix ? marker : prefix, end_marker, marker >= prefix ? false : true); 

	// TODO asrivastava , to get it reviewed from Mannu at highest priority.

	InfoFileRecord const * record;
	char prev_path[256 + prefix.length()];
	prev_path[0] = '\0';
	while((record = sorter.next()) != NULL)
	{
		
		if(record->is_deleted())
			continue;
		//OSDLOG(INFO, "record name in case of delimetr is "<< record->get_name())
		if (count != 0 && records->size() >= count)
		{
			break;
		}
		if (::strncmp(prefix.c_str(), record->get_name(), prefix.length()))
		{
			break;
		}
		if (!delimiter.empty())
		{
			// TODO delimiter; what kind of information is required???
			// The below, just skip if record has same path name before delimiter.
			int32_t len = ::strlen(prev_path);
			char const * name = record->get_name();
			OSDLOG(DEBUG, "Record name in case of delimeter is " << record->get_name() << " end")
			if (len && ::strncmp(prev_path, name, len) == 0)
			{
				OSDLOG(INFO, "Continuing record name in case of delimetr is "<< record->get_name() <<" end")
				continue;
			}
			::strncpy(prev_path, prefix.c_str(), prefix.length());
			for (int32_t i = prefix.length(); i < 256; i++)
			{
				if (name[i] == '\0')
				{
					// No delimiter found
					prev_path[0] = '\0';
					break;
				}
				if (::strncmp(&name[i], delimiter.c_str(), delimiter.length()) == 0)
				{
					::strcpy(&prev_path[i], delimiter.c_str());
					break;
				}
				prev_path[i] = name[i];
			}
		} 
		OSDLOG(DEBUG, "record name in case of delimetr is "<< record->get_name() <<" and it is deleted "<< record->is_deleted())

		ContainerInfoFileRecord const * typed_record = dynamic_cast<ContainerInfoFileRecord const *>(record);
		if (typed_record)
		{
			records->push_back(typed_record->get_object_record());
		}
		else
		{
			MemoryContainerInfoFileRecord const * typed_record = dynamic_cast<MemoryContainerInfoFileRecord const *>(record);
			records->push_back(typed_record->get_object_record());
		}
	}
	OSDLOG(DEBUG, "Read lock released from Info file while listing container");
}

void ContainerInfoFile::get_container_stat(
	std::string const & path,
	recordStructure::ContainerStat* container_stat,
	boost::function<recordStructure::ContainerStat(void)> const & get_stat
)
{
	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, 
		boost::shared_ptr<containerInfoFile::Trailer> > blocks_tuple = this->reader->read_stat_trailer_block();
	//TODO asrivastava, Modification for  Asynchronous call, to be reviewed from Mannu.
	*container_stat = *(blocks_tuple.get<0>().get());
	recordStructure::ContainerStat memory_stat = get_stat();
	container_stat->bytes_used += memory_stat.bytes_used;
	container_stat->object_count += memory_stat.object_count;
	OSDLOG(INFO, "Object count in memory- " << memory_stat.object_count << ", in info file- " << container_stat->object_count << " for container " << path);
	if (container_stat->object_count < 0)
		container_stat->object_count = 0;
	if (container_stat->bytes_used < 0)
		container_stat->bytes_used = 0;
	OSDLOG(DEBUG,  "Read Lock released while get_container_stat");
	return; 
}

void ContainerInfoFile::get_container_stat_after_compaction(
	std::string const & path,
	recordStructure::ContainerStat* container_stat,
	RecordList * record_list,
	boost::function<void(void)> const & release_flush_records,
	uint8_t node_id
)
{
	if(record_list != NULL)
	{
		OSDLOG(INFO, "calling ContainerInfoFile::get_container_stat_after_compaction, first flushing then start compaction start on file");
		// flushing
		this->flush_container(record_list, true, release_flush_records, node_id);
	}

	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>,
		boost::shared_ptr<containerInfoFile::Trailer> > blocks_tuple = this->reader->read_stat_trailer_block();
	*container_stat = *(blocks_tuple.get<0>().get());
	OSDLOG(DEBUG,  "Object count read from Info file is " << container_stat->object_count << " for Container" << path);
	OSDLOG(DEBUG,  "Bytes used read from Info file is " << container_stat->bytes_used << " for Container" << path);
}

void ContainerInfoFile::set_container_stat(recordStructure::ContainerStat stat, bool put_timestamp_change)
{

	OSDLOG(DEBUG, "In ContainerInfoFile::set_container_stat");
	uint64_t total_meta_length = 0 ;
	uint64_t total_meta_keys = 0 ;
	uint64_t total_acl_length = 0 ;
	uint64_t total_system_meta_length = 0;
	
	this->read_stat_trailer();	
	for(std::map<std::string, std::string>::const_iterator it = stat.metadata.begin(); it != stat.metadata.end(); ++it)
	{
		if (it->second == containerInfoFile::META_DELETE)
		{
			this->container_stat->metadata.erase(it->first);
		}
		else
		{
			this->container_stat->metadata[it->first] = it->second;
		}
	}

	for(std::map<std::string, std::string>::const_iterator it = this->container_stat->metadata.begin();
									it != this->container_stat->metadata.end(); ++it)
	{
		if (boost::regex_match(it->first, containerInfoFile::META_PATTERN)){
			total_meta_length += it->first.length();
			total_meta_length += it->second.length();
			total_meta_keys += 1;
		}
		else if(boost::regex_match(it->first, containerInfoFile::ACL_PATTERN)){
			total_acl_length += it->first.length() + it->second.length();
		}
		else if(boost::regex_match(it->first, containerInfoFile::SYSTEM_META_PATTERN)){
			total_system_meta_length += it->first.length() + it->second.length();
		}

	}

	OSDLOG(DEBUG, "Total meta length : " << total_meta_length << " Total meta keys : "<<total_meta_keys
					<<"Total ACL length : "<<total_acl_length<<"Total system meta length :"<<total_system_meta_length);
	if(total_meta_length > containerInfoFile::MAX_META_LENGTH)
		throw OSDException::MetaSizeException();
	if(total_meta_keys > containerInfoFile::MAX_META_KEYS)
		throw OSDException::MetaCountException();
	if(total_acl_length > containerInfoFile::MAX_ACL_LENGTH)
		throw OSDException::ACLMetaSizeException();
	if(total_system_meta_length > containerInfoFile::MAX_SYSTEM_META_LENGTH)
		throw OSDException::SystemMetaSizeException();

	/*OSD_EXCEPTION_THROW_IF(total_meta_length > containerInfoFile::MAX_META_LENGTH,
				OSDException::MetaSizeException, "Total meta length exceeds "<<containerInfoFile::MAX_META_LENGTH)
	OSD_EXCEPTION_THROW_IF(total_meta_keys > containerInfoFile::MAX_META_KEYS,
				OSDException::MetaCountException, "Total meta keys exceeds "<<containerInfoFile::MAX_META_KEYS)
	OSD_EXCEPTION_THROW_IF(total_acl_length > containerInfoFile::MAX_ACL_LENGTH,
				OSDException::ACLMetaSizeException, "ACL meta length exceeds "<<containerInfoFile::MAX_ACL_LENGTH)
	OSD_EXCEPTION_THROW_IF(total_system_meta_length > containerInfoFile::MAX_SYSTEM_META_LENGTH,
				OSDException::SystemMetaSizeException, "System meta length exceeds "<<containerInfoFile::MAX_SYSTEM_META_LENGTH)
*/
	if (put_timestamp_change)
	{
		this->container_stat->put_timestamp = stat.put_timestamp;
		OSDLOG(DEBUG, "Second PUT case, put_time_stamp is updated in stat."); 
	}
	this->writer->write_file(this->container_stat, this->trailer);
	OSDLOG(DEBUG,  "Write lock released from Info file while set_container_stat");
}

uint64_t ContainerInfoFile::get_node_id_from_journal_id(uint64_t unique_id)
{
	uint64_t node_id = unique_id >> 56;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        return node_id;
}

}
