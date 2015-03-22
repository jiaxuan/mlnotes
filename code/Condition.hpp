
#ifndef __COMMON_CONCURRENT_CONDITION_HPP__
#define __COMMON_CONCURRENT_CONDITION_HPP__

#include <common/common.hpp>
#include <common/concurrent/Mutex.hpp>
#include <common/util/Time.hpp>
#include <pthread.h>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 

namespace ml {
namespace common {
namespace concurrent {
	
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
	void wait( ml::common::concurrent::Mutex & mtx );
	
	/**
	 * Wait on a mutex for a maximum of <code>millis</code> milliseconds
	 * 
	 * @param mtx the mutex to guard on
	 * @param millis the max number of milliseconds to wait for
	 * @return false if timed out
	 */
	bool wait( ml::common::concurrent::Mutex & mtx, uint32 millis );

	/**
	 * Wait on a mutex for a maximum time of <code>delta</code>
	 * 
	 * @param mtx the mutex to guard on
	 * @param delta the max time to wait for
	 * @return false if timed out
	 */
	bool wait( ml::common::concurrent::Mutex & mtx, const ml::common::util::Time & delta );

private:
	pthread_cond_t m_cond;
};

}; // concurrent
}; // common
}; // ml

#endif //__COMMON_CONCURRENT_CONDITION_HPP__
