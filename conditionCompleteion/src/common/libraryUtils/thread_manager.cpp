/*
 * =====================================================================================
 *
 *       Filename:  thread_manager.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/12/2015 05:58:31 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "libraryUtils/thread_manager.h"
#include "libraryUtils/object_storage_exception.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace osd_threads
{

OsdThreadInterface::OsdThreadInterface(bool arg_stopable):is_running(false), running(true), stopable(arg_stopable)
{
	;
}

bool OsdThreadInterface::is_stopable()
{
        return this->stopable;
}

boost::thread::native_handle_type OsdThread::start()
{
	this->is_running = true;
	this->thread_ = new boost::thread(boost::bind(&OsdThread::run, this));
	return this->thread_->native_handle();
}

bool OsdThread::stop()
{
	if(this->stopable)
	{
		this->running = false;
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		if(this->is_running)
			return false;
		return true;
	}
	
	throw OSDException::UnstopableException();
}

OsdThread::~OsdThread()
{
	;
}

OsdThread_::OsdThread_(boost::function<void()> arg):OsdThreadInterface(false), runnable(arg)
{
}

OsdThread_::OsdThread_(boost::function<void(bool & is_running, bool & running)> arg):OsdThreadInterface(true)
{
	this->runnable = boost::bind(arg, boost::ref(this->is_running), boost::ref(this->running));
}

boost::thread::native_handle_type OsdThread_::start()
{
	this->is_running = true;
	this->thread_ = new boost::thread(this->runnable);
	return this->thread_->native_handle();
}

bool OsdThread_::stop()
{
        if(this->stopable)
        {
                this->running = false;
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                if(this->is_running)
                        return false;
                return true;
        }

        throw OSDException::UnstopableException();
}

OsdThread_::~OsdThread_()
{
	;
}


ThreadManager::ThreadManager(uint64_t no_of_threads): max_threads(no_of_threads), current_threads(0)
{

}

boost::thread::native_handle_type ThreadManager::create_thread(boost::function<void()> runnable)
{
	boost::mutex::scoped_lock lock(this->mutex);
	if(this->current_threads < max_threads)
	{
		this->current_threads++;
		boost::shared_ptr<OsdThreadInterface> thread_ptr(new OsdThread_(runnable));
		boost::thread::native_handle_type id = thread_ptr->start();
		this->map[id] = thread_ptr;
		return id;
	}
	throw OSDException::MaxThreadLimitReached();
}

boost::thread::native_handle_type ThreadManager::create_thread(boost::function<void(bool &is_running, bool &running)> runnable)
{
        boost::mutex::scoped_lock lock(this->mutex);
        if(this->current_threads < max_threads)
        {
                this->current_threads++;
                boost::shared_ptr<OsdThreadInterface> thread_ptr(new OsdThread_(runnable));
                boost::thread::native_handle_type id = thread_ptr->start();
                this->map[id] = thread_ptr;
                return id;
        }
        throw OSDException::MaxThreadLimitReached();
}

boost::thread::native_handle_type ThreadManager::create_thread(boost::shared_ptr<OsdThreadInterface> thread_ptr)
{
        boost::mutex::scoped_lock lock(this->mutex);
	if(this->current_threads < max_threads)
	{
		this->current_threads++;
		boost::thread::native_handle_type id = thread_ptr->start();
		this->map[id] = thread_ptr;
		return id;
	}
	throw OSDException::MaxThreadLimitReached();
}

bool ThreadManager::stop(boost::thread::native_handle_type id)
{
	boost::mutex::scoped_lock lock(this->mutex);
	boost::shared_ptr<OsdThreadInterface> thread_ptr(this->map[id]);
	//thread_ptr.reset(this->map[id])
	
	if(thread_ptr->is_stopable())
	{
		thread_ptr->stop();
		this->map.erase(id);
		this->current_threads--;
		return true;
	}
	return false;
}

bool ThreadManager::force_stop(boost::thread::native_handle_type id)
{
	boost::mutex::scoped_lock lock(this->mutex);
	boost::shared_ptr<OsdThreadInterface> thread_ptr = this->map[id];
	pthread_kill(id, 9);
	this->map.erase(id);
	this->current_threads--;
	return true;
}

}
