/*
 * =====================================================================================
 *
 *       Filename:  index_block.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/30/2014 09:52:23 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>

#include "containerLibrary/index_block.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/object_storage_log_setup.h"
#include <openssl/md5.h>

namespace containerInfoFile
{
//UNCOVERED_CODE_BEGIN:
bool IndexBlock::exists(std::string const & hash) 
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	return it == this->index_.end()? false : true;
}

bool IndexBlock::is_deleted(std::string const & hash)
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	if( it != this->index_.end())
	{
		return (*it).file_state == OBJECT_DELETE ? true : false;
	}
	return false; // probably throw 
}

bool IndexBlock::is_modified(std::string const & hash)
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	if( it != this->index_.end())
	{
		return (*it).file_state == OBJECT_MODIFY ? true : false;
	}
	return false; // probably throw 
}

// Below Function must be called only after make sure that the "hash" is present in the index block.
uint64_t IndexBlock::get_file_state(std::string const & hash)
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	return (*it).file_state;
}

uint64_t IndexBlock::get_obj_size(std::string const & hash)
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	return (*it).size;
}

void IndexBlock::append(std::string const & hash, uint64_t file_state, uint64_t size)
{
       this->index_.push_back(IndexRecord(hash, file_state, size));
}

void IndexBlock::add(std::string const & hash, uint64_t file_state, uint64_t size)
{
	index_record_iterator it = this->find(hash);
	if( it != this->index_.end())
		(*it) = IndexRecord(hash, file_state, size);
	else
		this->index_.push_back(IndexRecord(hash, file_state, size));
}

void IndexBlock::add(IndexRecord const & record)
{
	index_record_iterator it = this->find(record.hash);
	if( it != this->index_.end())
		(*it) = record;
	else
		this->index_.push_back(record);
}

void IndexBlock::set_file_state(std::string const & hash, uint64_t file_state)
{
	index_record_iterator it = this->find(hash);
	if( it != this->index_.end())
		(*it).file_state = file_state;
}

void IndexBlock::remove(std::string const & hash)
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	if( it != this->index_.end())
		this->index_.erase(it);
}

IndexBlock::IndexRecord IndexBlock::get(std::string const & hash) 
{
	std::vector<IndexRecord>::iterator it = this->find(hash);
	if( it != this->index_.end() )
		return (*it);
	else
		return IndexRecord(0, 1, 0);
}
//UNCOVERED_CODE_END

std::string IndexBlock::get_hash(std::string const & acc_name, std::string const & cont_name, std::string const & obj_name)
{
	std::string PREFIX = "changeme"; //similar to osd.conf 'osd_hash_path_prefix' = 'changeme'  value
	std::string SUFFIX = "changeme"; //similar to osd.conf 'osd_hash_path_suffix' = 'changeme' value
	std::string name = "/" + acc_name + "/" + cont_name + "/" + obj_name;
	std::string md5_input = PREFIX + name + SUFFIX; 
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5((const unsigned char*)md5_input.c_str(), md5_input.length(), (unsigned char*)&digest);
	char mdString[33];
	for (int i = 0; i < 16; i++)
		sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
	mdString[32] = '\0';
	OSDLOG(DEBUG, "Object name : "<<name<<", MD5 hash : "<<mdString);
	return std::string(mdString);
}
	
std::vector<IndexBlock::IndexRecord>::iterator IndexBlock::find(std::string hash) 
{
	return std::find_if(this->index_.begin(), this->index_.end(), 
				boost::lambda::bind(&IndexBlock::IndexRecord::hash, boost::lambda::_1) == hash);
}

uint64_t IndexBlock::get_size() const
{
	return this->index_.size();
}

}
