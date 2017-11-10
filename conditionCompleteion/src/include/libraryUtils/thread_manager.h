/*
 * =====================================================================================
 *
 *       Filename:  thread_manager.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/12/2015 05:30:06 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __THREAD_MANAGER_H__
#define __THREAD_MANAGER_H__


#include <map>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/noncopyable.hpp>


namespace osd_threads
{

class ThreadManager;

class OsdThreadInterface: public boost::noncopyable
{
public:
        friend class ThreadManager;
	OsdThreadInterface(bool arg_stopable);
        virtual ~OsdThreadInterface() = 0;
//protected:
        virtual boost::thread::native_handle_type start() = 0;
        virtual bool stop() = 0;
        bool is_stopable();
        boost::thread *thread_;
        bool is_running;
        bool running;
        const bool stopable;
};


class OsdThread: public OsdThreadInterface
{
public:
	virtual ~OsdThread();
//protected:
	virtual boost::thread::native_handle_type start();
	virtual bool stop();
	virtual void run() = 0;
};

class OsdThread_ : public OsdThreadInterface
{
public:
	OsdThread_(boost::function<void()>);
	OsdThread_(boost::function<void(bool &, bool &)>);
	virtual ~OsdThread_();
//protected:
	boost::function<void(void)> runnable;
	virtual boost::thread::native_handle_type start();
	virtual bool stop();
};

class ThreadManager
{
public:
	ThreadManager(uint64_t no_of_threads);
	boost::thread::native_handle_type create_thread(boost::function<void()>);
	boost::thread::native_handle_type create_thread(boost::function<void(bool &, bool &)>);
	boost::thread::native_handle_type create_thread(boost::shared_ptr<OsdThreadInterface>);
	bool stop(boost::thread::native_handle_type id);
	bool force_stop(boost::thread::native_handle_type id);
	bool is_stopable(boost::thread::native_handle_type id);
private:
	//void remove_thread(boost::thread::native_handle_type id);
	typedef std::map<boost::thread::native_handle_type, boost::shared_ptr<OsdThreadInterface> > thread_map;
	thread_map map;
	boost::mutex mutex;
	const uint64_t max_threads;
	uint64_t current_threads;
};

} // end of namespace osd_threads

#endif
