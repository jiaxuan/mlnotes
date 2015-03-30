
#ifndef __COMMON_CONCURRENT_CONDITION_HPP__
#define __COMMON_CONCURRENT_CONDITION_HPP__

#include "common.hpp"
#include "Mutex.hpp"
// #include <common/util/Time.hpp>
#include <pthread.h>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 

/** ----------------------------------------------------------------------------------
 * Condition class
 */
class Condition : private virtual boost::noncopyable
{
public:
	Condition(bool process_shared = false);
	~Condition();

	/**
	 * Wake one queued thread
	 */
	void notify();
	
	/**
	 * Wake all queued threads
	 */
	void notifyAll();
	
	/**
	 * Wait on a mutex
	 * 
	 * @param mtx the mutex to guard on
	 */
	void wait( Mutex & mtx );
	
	/**
	 * Wait on a mutex for a maximum of <code>millis</code> milliseconds
	 * 
	 * @param mtx the mutex to guard on
	 * @param millis the max number of milliseconds to wait for
	 * @return false if timed out
	 */
	bool wait( Mutex & mtx, uint32 millis );

	/**
	 * Wait on a mutex for a maximum time of <code>delta</code>
	 * 
	 * @param mtx the mutex to guard on
	 * @param delta the max time to wait for
	 * @return false if timed out
	 */
	// bool wait( Mutex & mtx, const ml::common::util::Time & delta );

private:
	pthread_cond_t m_cond;
};

#endif //__COMMON_CONCURRENT_CONDITION_HPP__
