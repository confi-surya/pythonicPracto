/*
 * =====================================================================================
 *
 *       Filename:  lock_manager.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/13/2014 01:02:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <algorithm> 

#include "libraryUtils/lock_manager.h"

namespace lockManager
{

Mutex::Mutex(LockManager &lock_manager, std::string key):
		lock_manager(lock_manager), key(key), current_count(1)
{
	;
}

void Mutex::lock_shared()
{
	this->mutex.lock_shared();
}

void Mutex::lock()
{
	this->mutex.lock();
}

void Mutex::unlock_shared()
{
	this->mutex.unlock_shared();
	this->lock_manager.release_lock(this->key);
}

void Mutex::unlock()
{
	this->mutex.unlock();
	this->lock_manager.release_lock(this->key);
}

Mutex::~Mutex()
{
	;
}

LockManager::LockManager(uint64_t lock_limit):lock_limit(lock_limit)
{
	;
}

boost::shared_ptr<Mutex> LockManager::get_lock(std::string key)
{
	boost::mutex::scoped_lock lock(this->mutex);	
	while(this->lock_map.size() >= this->lock_limit)
	{
		this->condition.wait(lock);
	}
	LockMap::iterator it = this->lock_map.find(key);
	if( it != this->lock_map.end())
	{
		it->second->current_count ++;
		return it->second;
	}
	else
	{
		boost::shared_ptr<Mutex> mutex(new Mutex(*this, key));	
		this->lock_map[key] = mutex;
		return mutex;
	}
}

void LockManager::release_lock(std::string key)
{
	boost::mutex::scoped_lock lock(this->mutex);
	LockMap::iterator it = this->lock_map.find(key);
	if( it != this->lock_map.end())
	{
		it->second->current_count --;
		if (it->second->current_count == 0)
		{
			this->lock_map.erase(it);
			this->condition.notify_one();
		}
	}
}

}
