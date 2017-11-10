#ifndef __LOCK_MANAGER_H__
#define __LOCK_MANAGER_H__

/*
 * =====================================================================================
 *
 *       Filename:  lock_manager.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/13/2014 12:51:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>
#include "object_storage_log_setup.h"

namespace lockManager
{

class LockManager;

class Mutex: private boost::noncopyable
{
public:
	void lock_shared();
	void lock();
	void unlock_shared();
	void unlock();
	~Mutex();
	friend class LockManager;

private:
	Mutex(LockManager &lock_manager, std::string key); // only lock manager should be able to create it 

	LockManager &lock_manager;
	std::string key;
	boost::shared_mutex mutex;
	uint64_t current_count;
};

class ReadLock
{
private:
	boost::shared_ptr<Mutex> rwlock;

public:
	ReadLock(boost::shared_ptr<Mutex> lock) : rwlock(lock) 
	{
		//std::cout<<"lock shared\n";
		rwlock->lock_shared(); 
		OSDLOG(DEBUG, "ReadLock is taken.");
	}

	~ReadLock() 
	{
		//std::cout<<"unlock shared\n";
		rwlock->unlock_shared();
		OSDLOG(DEBUG, "ReadLock is released."); 
	}  
};

class WriteLock
{
private:
	boost::shared_ptr<Mutex> rwlock;
public:
	WriteLock(boost::shared_ptr<Mutex> lock) : rwlock(lock) 
	{ 
		//std::cout<<"lock unique\n";
		rwlock->lock(); 
		OSDLOG(DEBUG, "WriteLock is taken.");
	}

	~WriteLock() 
	{
		//std::cout<<"unlock unique\n";
		rwlock->unlock(); 
		OSDLOG(DEBUG, "WriteLock is released.");
	}
};

class LockManager
{
public:
	LockManager(uint64_t lock_limit);
	boost::shared_ptr<Mutex> get_lock(std::string key);
	void release_lock(std::string key);

private:
	typedef std::map<std::string, boost::shared_ptr<Mutex> > LockMap;
	const uint64_t lock_limit;
	LockMap lock_map;
	boost::mutex mutex;
	boost::condition_variable condition;
};

}

#endif
