
#ifndef __COMMON_CONCURRENT_READWRITEMUTEX_HPP__
#define __COMMON_CONCURRENT_READWRITEMUTEX_HPP__

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
 * Reader-writer mutex.
 */
class ReadWriteMutex : private virtual boost::noncopyable
{
public:

	typedef ScopedReadLock<ReadWriteMutex> 	scoped_read_lock;
	typedef ScopedWriteLock<ReadWriteMutex> scoped_write_lock;
	
public:

	ReadWriteMutex(bool process_shared = false);
	~ReadWriteMutex();

private:
	
	friend struct ScopedReadLock<ReadWriteMutex>;
	friend struct ScopedWriteLock<ReadWriteMutex>;

	/**
	 * Locking routines: kept private so you're forced to use the scoped_read_lock & scoped_write_lock
	 */
	void lock_read();
	void lock_write();
	
	/**
	 * Unlocking routines: kept private so you're forced to use the scoped_read_lock & scoped_write_lock
	 */
	void unlock_read();
	void unlock_write();
	
private:
	
	pthread_rwlock_t m_rwlock;
};
		
}; // concurrent
}; // common
}; // ml

#endif //__COMMON_CONCURRENT_READWRITEMUTEX_HPP__
