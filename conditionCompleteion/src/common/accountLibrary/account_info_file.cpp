#include "accountLibrary/account_info_file.h"
#include "accountLibrary/account_record_iterator.h"
#include <boost/lexical_cast.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>
#include <cool/debug.h>

namespace accountInfoFile
{

AccountInfoFile::AccountInfoFile(
				boost::shared_ptr<Reader> reader, boost::shared_ptr<Writer> writer,
				boost::shared_ptr<lockManager::Mutex> mutex):
				reader(reader), writer(writer), mutex(mutex)
{
	;
}
//UNCOVERED_CODE_BEGIN:
AccountInfoFile::AccountInfoFile(boost::shared_ptr<Reader> reader, boost::shared_ptr<lockManager::Mutex> mutex): 
		reader(reader), mutex(mutex)
{
	;
}
//UNCOVERED_CODE_END
void AccountInfoFile::create_account(boost::shared_ptr<recordStructure::AccountStat> account_stat)
{
	OSDLOG(DEBUG, "calling AccountInfoFile::create_account with AccountStat( account : "
			<<account_stat->account	<<", created_at : "<<account_stat->created_at<<", put_timestamp : "
			<<account_stat->put_timestamp<<", delete_timestamp : "<<account_stat->delete_timestamp
			<<", container_count : "<<account_stat->container_count<<", object_count : "
			<<account_stat->object_count<<", bytes_used : "<<account_stat->bytes_used<<", hash : "
			<<account_stat->hash<<", id : "<<account_stat->id<<", status : "<< account_stat->status
			<<", status_changed_at : "<<account_stat->status_changed_at <<" )");
			//<<", metadata : "<<account_stat->metadata<<" )");
	this->trailer_block.reset(new TrailerBlock(0));
	this->writer->write_file(account_stat, this->trailer_block);
}


boost::shared_ptr<recordStructure::AccountStat> AccountInfoFile::get_account_stat()
{
	this->read_stat_trailer();
	return this->stat_block;
}

void AccountInfoFile::set_account_stat(boost::shared_ptr<recordStructure::AccountStat> account_stat, bool second_put_case)
{

		OSDLOG(DEBUG, "In AccountInfoFile::set_container_stat");
		uint64_t total_meta_length = 0 ;
		uint64_t total_meta_keys = 0 ;
		uint64_t total_acl_length = 0 ;
		uint64_t total_system_meta_length = 0;

		this->read_stat_trailer();
		for(std::map<std::string, std::string>::const_iterator it = account_stat->metadata.begin(); it != account_stat->metadata.end(); ++it)
		{
				if (it->second == accountInfoFile::META_DELETE)
				{
						this->stat_block->metadata.erase(it->first);
				}
				else
				{
						this->stat_block->metadata[it->first] = it->second;
				}
		}

		for(std::map<std::string, std::string>::const_iterator it = this->stat_block->metadata.begin();									it != this->stat_block->metadata.end(); ++it)
		{
				if (boost::regex_match(it->first, accountInfoFile::META_PATTERN)){
						total_meta_length += it->first.length();
						total_meta_length += it->second.length();
						total_meta_keys += 1;
				}
				else if(boost::regex_match(it->first, accountInfoFile::ACL_PATTERN)){
						total_acl_length += it->first.length() + it->second.length();
				}
				else if(boost::regex_match(it->first, accountInfoFile::SYSTEM_META_PATTERN)){
						total_system_meta_length += it->first.length() + it->second.length();
				}

		}
	OSDLOG(DEBUG, "Total meta length : " << total_meta_length << " Total meta keys : "<<total_meta_keys
										<<"Total ACL length : "<<total_acl_length<<"Total system meta length :"<<total_system_meta_length);
		if(total_meta_length > accountInfoFile::MAX_META_LENGTH)
				throw OSDException::MetaSizeException();
		if(total_meta_keys > accountInfoFile::MAX_META_KEYS)
				throw OSDException::MetaCountException();
		if(total_acl_length > accountInfoFile::MAX_ACL_LENGTH)
				throw OSDException::ACLMetaSizeException();
		if(total_system_meta_length > accountInfoFile::MAX_SYSTEM_META_LENGTH)
				throw OSDException::SystemMetaSizeException();

		if (second_put_case)
		{
				this->stat_block->put_timestamp = account_stat->put_timestamp;
				OSDLOG(DEBUG, "Second account PUT case, put_time_stamp is updated in stat.");
		}
		this->writer->write_file(this->stat_block, this->trailer_block);
}



void AccountInfoFile::add_container(boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	std::list<recordStructure::ContainerRecord> list;
	list.push_back(*container_record);
	this->add_container_internal(list);
}

void AccountInfoFile::add_container_internal(std::list<recordStructure::ContainerRecord> container_records)
{
	OSDLOG(DEBUG, "calling add_container_internal");
	this->read_stat_trailer();
	
	if (this->trailer_block->unsorted_record >= MAX_UNSORTED_RECORD)
	{
		OSDLOG(INFO, "Max unsorted record limit reached : "<<MAX_UNSORTED_RECORD<<", Compaction starting...");
		this->compaction();
		this->read_stat_trailer();
	}

	this->static_data_block.reset(new StaticDataBlock);
	this->non_static_data_block.reset(new NonStaticDataBlock);
	
	for(std::list<recordStructure::ContainerRecord>::iterator container_record = container_records.begin(); container_record != container_records.end(); container_record++)
	{
		this->static_data_block->add(container_record->ROWID, container_record->name);
		this->non_static_data_block->add(
				container_record->hash,
				container_record->put_timestamp,
				container_record->delete_timestamp,
				container_record->object_count,
				container_record->bytes_used,
				container_record->deleted);
		this->stat_block->container_count++;
		this->stat_block->bytes_used += container_record->bytes_used;
		this->stat_block->object_count += container_record->object_count;
	}

	this->trailer_block->unsorted_record += static_data_block->get_size();
	this->writer->write_static_data_block(this->static_data_block, this->non_static_data_block,
						this->stat_block, this->trailer_block);
	OSDLOG(DEBUG, "exiting add_container_internal");
}


void AccountInfoFile::delete_container(boost::shared_ptr<recordStructure::ContainerRecord> container_record)
{
	std::list<recordStructure::ContainerRecord> list;
	container_record->bytes_used = 0;
	container_record->object_count = 0;
	container_record->deleted = true;
	list.push_back(*container_record);
	this->update_container(list);
}

bool AccountInfoFile::update_record(recordStructure::ContainerRecord & source, 
		NonStaticDataBlockBuilder::NonStaticBlock_ & target)
{
	OSDLOG(DEBUG, "update_container, container hash(name) "
		<<source.get_hash()<<"("<<source.get_name()<<")"
		<<"entry is present in info-file.")
	if(!source.get_deleted()) // delete entry from updater
	{
		if(target.deleted) // source is not deleted then update case
		{
			return false;
		}
	}

	// update non-static record
	//memset(&target, 0, sizeof(target));//memset is not required .Use old values if you dnt want to update the record
	memcpy(target.hash, source.get_hash().c_str(),
			source.hash.length() > HASH_LENGTH ? HASH_LENGTH : source.hash.length());
//	memcpy(target.put_timestamp, source.get_put_timestamp().c_str(),
//			source.get_put_timestamp().length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : source.put_timestamp.length());//dnt update put time stamp
	memcpy(target.delete_timestamp, source.get_delete_timestamp().c_str(),
			source.get_delete_timestamp().length() > TIMESTAMP_LENGTH ? TIMESTAMP_LENGTH : source.get_delete_timestamp().length());
	target.object_count = source.get_object_count();
	target.bytes_used = source.get_bytes_used();
	target.deleted = source.get_deleted();
	return true;
}

void AccountInfoFile::update_container(std::list<recordStructure::ContainerRecord> container_records)
{
	this->read_stat_trailer();
	//uint64_t put_time_stamp;
	uint64_t max_read_count = MAX_READ_NON_STATIC_RECORD;
	OSDLOG(DEBUG, "update_container, unsorted_record: "
		<< this->trailer_block->unsorted_record);

	std::vector<NonStaticDataBlockBuilder::NonStaticBlock_> blocks(max_read_count);
	// check unsorted_record
	blocks.resize(this->trailer_block->unsorted_record);
	//this->non_static_data_block = this->reader->read_non_static_data_block(this->trailer_block->sorted_record,
	//		this->trailer_block->unsorted_record);
	this->reader->read_non_static_data_block(&blocks[0],
			this->trailer_block->sorted_record, this->trailer_block->unsorted_record);

	std::list<recordStructure::ContainerRecord>::iterator container_record_iter = container_records.begin();
	bool modified = false;
	OSDLOG(DEBUG, "update_container, looking entries in unsorted non-static data part");
	while(container_record_iter != container_records.end() && container_records.size())
	{
		std::vector<NonStaticDataBlockBuilder::NonStaticBlock_>::reverse_iterator iter =
				std::find_if(blocks.rbegin(), blocks.rend(),
					boost::lambda::bind(::strcmp,
						boost::lambda::bind(&NonStaticDataBlockBuilder::NonStaticBlock_::hash, boost::lambda::_1),
						container_record_iter->hash.c_str()) == 0);
		if(iter != blocks.rend())
		{
			OSDLOG(DEBUG, "update_container, container hash(name) "
				<<container_record_iter->get_hash()<<"("<<container_record_iter->get_name()<<")"
				<<"entry is present in info-file.")
			if(this->update_record(*container_record_iter, *iter)) //is_updated then do
			{
				modified = true;
				container_records.erase(container_record_iter++);
				OSDLOG(DEBUG, "update_container, entry is updated");
			}
			else  //addition-case
			{
				OSDLOG(INFO, "update_container, addition case for "
					<< container_record_iter->get_name());
				container_record_iter++;
			}
		}
		else
		{
			container_record_iter++;
		}
	}
	if(modified)
	{
		OSDLOG(DEBUG, "update_container, non static data block is writing..")
		writer->write_non_static_data_block(&blocks[0], this->trailer_block,
				this->trailer_block->sorted_record, this->trailer_block->unsorted_record);
	}

	this->stat_block->container_count = 0;
	this->stat_block->object_count = 0;
	this->stat_block->bytes_used = 0;
	std::set<std::string> hash_set;
	std::vector<NonStaticDataBlockBuilder::NonStaticBlock_>::reverse_iterator riter = blocks.rbegin();
	for(; riter != blocks.rend(); riter ++)
	{
		if(hash_set.find(riter->hash) != hash_set.end())
		{
			continue;
		}
		hash_set.insert(riter->hash);
		if(!riter->deleted)
		{
			this->stat_block->container_count ++;
			this->stat_block->object_count += riter->object_count;
			this->stat_block->bytes_used += riter->bytes_used;
		}
	}

	uint64_t rest_count = this->trailer_block->sorted_record;
	uint64_t current_start_position = 0;
	OSDLOG(DEBUG, "update_container, looking entries in sorted non-static data part");
	while(rest_count != 0)
	{
		uint64_t count = std::min(max_read_count, rest_count);
		blocks.resize(count);
		this->reader->read_non_static_data_block(&blocks[0], current_start_position, count);

		std::list<recordStructure::ContainerRecord>::iterator container_record_iter = container_records.begin();
		modified = false;
		while(container_record_iter != container_records.end())
		{
			std::vector<NonStaticDataBlockBuilder::NonStaticBlock_>::iterator iter =
					std::find_if(blocks.begin(), blocks.end(),
						boost::lambda::bind(::strcmp,
						boost::lambda::bind(&NonStaticDataBlockBuilder::NonStaticBlock_::hash, boost::lambda::_1),
						container_record_iter->hash.c_str()) == 0);
			if(iter != blocks.end())
			{
				OSDLOG(DEBUG, "update_container, container hash(name) "
						<<container_record_iter->get_hash()<<"("<<container_record_iter->get_name()<<")"
						<<"entry is present in info-file.")
				if (this->update_record(*container_record_iter, *iter))
				{
					modified = true;
					container_records.erase(container_record_iter++);
					OSDLOG(DEBUG, "update_container, entry is updated");
				}
				else
				{
					OSDLOG(INFO, "update_container, addition case for "
										<< container_record_iter->get_name());
					container_record_iter++;
				}
			}
			else
			{
				container_record_iter++;
			}
		}
		if (modified)
		{
			writer->write_non_static_data_block(&blocks[0], this->trailer_block, current_start_position, count);
		}
		current_start_position += count;
		rest_count -= count;
		std::vector<NonStaticDataBlockBuilder::NonStaticBlock_>::iterator iter = blocks.begin();
		for(; iter != blocks.end(); iter ++)
		{
			if(hash_set.find(iter->hash) != hash_set.end())
			{
				continue;
			}
			if(!iter->deleted)
			{
				this->stat_block->container_count ++;
				this->stat_block->object_count += iter->object_count;
				this->stat_block->bytes_used += iter->bytes_used;
			}
		}
	}

	OSDLOG(DEBUG, "update_container, new entries count "<<container_records.size());

	//re-set of container records having addition or already deleted entries(donothing)
	container_record_iter = container_records.begin();
	while(container_record_iter!=container_records.end())
	{
		if (container_record_iter->get_deleted()) //deleted entry from updater : entry is already deleted by compaction
		{
			container_records.erase(container_record_iter++);
		}
		else
		{
			container_record_iter++;
		}
	}

	OSDLOG(INFO, "update_container, stat block is writing.."<<" container_count:"
			<< this->stat_block->container_count<<"byte used:"<<this->stat_block->bytes_used
			<<"object count:"<< this->stat_block->object_count);
	writer->write_stat_trailer_block(this->stat_block, this->trailer_block);

	if(!container_records.empty())
	{
		
		OSDLOG(INFO, "update_container, add container entries, count "<<container_records.size());
		this->add_container_internal(container_records);
	}
}

std::list<recordStructure::ContainerRecord> AccountInfoFile::list_container(uint64_t const count, std::string const &marker, 
							std::string const &end_marker, std::string const &prefix, std::string const &delimiter)
{
	OSDLOG(INFO, "Account Info-File: Listing is starting with, start marke: "<<marker<<" end marker: "<<end_marker<<" prefix: "<<prefix<<" delimiter: "<<delimiter);
	std::vector<InfoFileIterator *> iters;
	SortedAccountInfoIterator sorted(this->reader, marker > prefix ? marker : prefix);
	iters.push_back(&sorted);
	UnsortedAccountInfoIterator unsorted(this->reader);
	iters.push_back(&unsorted);
	InfoFileSorter sorter(iters, marker > prefix ? marker : prefix, end_marker, marker >= prefix ? false : true );
	std::list<recordStructure::ContainerRecord> records;
	InfoFileRecord const * record;

	char prev_path[256 + prefix.length()];
	prev_path[0] = '\0';

	while((record = sorter.next()) != NULL)
	{
		if (count != 0 && records.size() >= count)
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
			if (len && ::strncmp(prev_path, name, len) == 0)
			{
					//std::cout << "same path: " << name << std::endl;
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
		if(!record->is_deleted())
		{
			AccountInfoFileRecord const * typed_record = dynamic_cast<AccountInfoFileRecord const *>(record);
			if (typed_record)
			{
				records.push_back(typed_record->get_object_record());
			}
		}
	}
	return records;
}

void AccountInfoFile::compaction()
{
	OSDLOG(INFO, "Account Info-File: Compaction is starting");
	this->read_stat_trailer();
	this->stat_block->container_count = 0;
	this->stat_block->object_count = 0;
	this->stat_block->bytes_used = 0;
	uint64_t total_size = this->trailer_block->sorted_record + this->trailer_block->unsorted_record;
	this->trailer_block.reset(new accountInfoFile::TrailerBlock(total_size));
	this->writer->begin_compaction(this->trailer_block, total_size);

	std::vector<InfoFileIterator *> iters;
	SortedAccountInfoIterator sorted(this->reader);
	iters.push_back(&sorted);
	UnsortedAccountInfoIterator unsorted(this->reader);
	iters.push_back(&unsorted);
	InfoFileSorter sorter(iters);

	uint64_t count = 0;
	//std::vector<DataBlockBuilder::DataIndexRecord> diRecords;
	InfoFileRecord const * record;
	while((record = sorter.next()) != NULL)
	{
		if(!record->is_deleted())
		{
			AccountInfoFileRecord const * typed_record = dynamic_cast<AccountInfoFileRecord const *>(record);
			StaticDataBlockBuilder::StaticBlock_ const * static_record = typed_record->get_static_record();
			NonStaticDataBlockBuilder::NonStaticBlock_ const * non_static_record = typed_record->get_non_static_record();
			this->trailer_block->sorted_record ++;
			this->stat_block->container_count ++;
			this->stat_block->object_count += non_static_record->object_count;
			this->stat_block->bytes_used += non_static_record->bytes_used;
			this->writer->write_record(static_record, non_static_record);

			count ++;
			/*if ((count % objectStorage::DATA_INDEX_INTERVAL) == 0)
			{
				diRecords.push_back(DataBlockBuilder::DataIndexRecord(typed_record->get_name()));
			}*/
		}
	}
	this->writer->flush_record();
	/*if (!diRecords.empty())
	{
		this->writer->write_data_index_block(this->trailer, diRecords);
	}*/
	this->writer->write_file(this->stat_block, this->trailer_block);
	this->writer->finish_compaction();
	OSDLOG(INFO, "Account Info-File: Compaction is completed");
	this->reader->reopen();
}

void AccountInfoFile::read_stat_trailer()
{
	boost::tuple<boost::shared_ptr<recordStructure::AccountStat>,
	boost::shared_ptr<TrailerBlock> > blocks_tuple = this->reader->read_stat_trailer_block();
	this->trailer_block = blocks_tuple.get<1>();
	this->stat_block = blocks_tuple.get<0>();
}

}
