
#ifndef __COMMON_CONCURRENT_MUTEX_HPP__
#define __COMMON_CONCURRENT_MUTEX_HPP__

#include <common/common.hpp>
#include <common/concurrent/ScopedLock.hpp>
#include <pthread.h>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 

namespace ml {
namespace common {
namespace concurrent {

/** ----------------------------------------------------------------------------------
 * Mutex class. 
 * 
 * This will throw an exception on error, deadlock, or double-unlock detection.
 */
class Mutex : private virtual boost::noncopyable
{
public:

	enum Type 
	{
		MT_Unchecked 	= PTHREAD_MUTEX_NORMAL,
		MT_Recursive 	= PTHREAD_MUTEX_RECURSIVE,
		MT_Checked 		= PTHREAD_MUTEX_ERRORCHECK
	};

	typedef ScopedLock<Mutex> scoped_lock;

public:

	/**
	 * Default to MT_Checked so we get an exception instead of deadlocking
	 */
	Mutex(Mutex::Type type = MT_Checked, bool process_shared = false);
	~Mutex();
	
private:
	
	friend class ScopedLock<Mutex>;

	/**
	 * Locking routine: kept private so you're forced to use the scoped_lock
	 */
	void lock();
		
	/**
	 * Unlocking routine: kept private so you're forced to use the scoped_lock
	 */
	void unlock();

private:
	
	friend class Condition;
	
	pthread_mutex_t m_mutex;
	Type m_type;
};
		
}; // concurrent
}; // common
}; // ml

#endif //__COMMON_CONCURRENT_MUTEX_HPP__
