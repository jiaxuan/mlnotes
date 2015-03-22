#ifndef __InterruptibleSleep__
#define __InterruptibleSleep__

#include <common/concurrent/Mutex.hpp>
#include <common/concurrent/Condition.hpp>

namespace ml { namespace common { namespace concurrent {

/**
 * Use this class when you want to have a thread go to pause, but very easily be able to wake it up from another thread.
 */
 
class InterruptibleSleep : private virtual boost::noncopyable
{
public:
	/**
	 * Causes the calling thread to sleep.
	 * Returns true if it was woken up, false if it timed out.
	 */
	bool sleep(int milliseconds);
	bool wait(int msec);
	/**
	 * Use these methods to wake up any threads sleeping via this mechanism.
	 */
	void wakeOne();
	void wakeAll();
	
private:
	ml::common::concurrent::Mutex m_mutex;
	ml::common::concurrent::Condition m_condition;
};


}}}	// namespace

#endif
