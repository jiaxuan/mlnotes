
#ifndef __COMMON_CONCURRENT_SCOPEDLOCK_HPP__
#define __COMMON_CONCURRENT_SCOPEDLOCK_HPP__

#include "common.hpp"
#include <boost/noncopyable.hpp>

/** ----------------------------------------------------------------------------------
 * Syntactic sugar macro providing a Java style "synchronized" block.
 * You can use the "break" keyword to break out of the synchronized block early.
 * 
 * Sample usage:
 * <code>
 * 		ml::common::concurrent::Mutex my_mutex;
 * 		bool shared_value;
 * 
 * 		synchronized( my_mutex ) {
 * 			if( shared_value == false )
 * 				break;	// break out early
 * 			
 * 			// do stuff here that requires the mutex lock
 * 		}
 * </code>
 * 
 * @warning Care must be taken when using <code>break</code> and <code>continue</code> statements with a 
 * 			combination of synchronized blocks and either <code>for</code>, <code>while</code> or 
 * 			<code>switch</code> blocks
 * <code>
 * 		switch {
 * 		case 1:
 * 			synchronized(my_mutex) {
 * 				...
 *	 			break; // This breaks out of the synchronized block, not the case statement
 * 			}
 * 
 * 		default:
 * 		}
 * 
 * 		for(int i=0; i<10; i++) {
 * 			synchronized(my_mutex) {
 * 				...
 *	 			break; // This breaks out of the synchronized block, not the for loop
 * 			}
 * 		}
 * </code>
 */

#define synchronized(mtx) 													\
	for( ScopedLock<Mutex> 	                                                \
		 	UNIQUE_NAME(lok)(mtx); 											\
		 	UNIQUE_NAME(lok); 												\
		 	UNIQUE_NAME(lok).unlock() ) 

/** ----------------------------------------------------------------------------------
 * As above, but for read write mutexes
 * 
 * Sample usage:
 * <code>
 * 		ml::common::concurrent::ReadWriteMutex my_rwmutex;
 * 
 * 		read_synchronized(my_rwmutex) {
 * 			// do read stuff
 * 		}
 * 
 * 		write_synchronized(my_rwmutex) {
 * 			// do write stuff
 * 		}
 * </code>		
 */

#define read_synchronized(mtx) 															\
	for( ScopedReadLock<ReadWriteMutex>                                                 \
		 	UNIQUE_NAME(lok)(mtx); 														\
		 	UNIQUE_NAME(lok); 															\
		 	UNIQUE_NAME(lok).unlock() ) 

#define write_synchronized(mtx) 														\
	for( ScopedWriteLock<ReadWriteMutex>                                                \
		 	UNIQUE_NAME(lok)(mtx); 														\
		 	UNIQUE_NAME(lok); 															\
		 	UNIQUE_NAME(lok).unlock() ) 

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 

/** ----------------------------------------------------------------------------------
 * Simple auto-lock / auto-unlock class that performs a lock on the Lockable argument
 * on construction, and unlocks on destruction
 * 
 * Sample usage:
 * <code>
 * 		Mutex mtx;
 * 		Mutex::scoped_lock autolock(mtx);
 * 		// perform actions here
 * </code>
 */
template <typename Lockable>
class ScopedLock : protected virtual boost::noncopyable
{
	typedef void(Lockable::*Locker)();
	typedef void(Lockable::*Unlocker)();
	
public:
	explicit ScopedLock(const Lockable& lockable, 
						Locker locker = &Lockable::lock, 
						Unlocker unlocker = &Lockable::unlock) :
		m_lockable(lockable),
		m_unlocker(unlocker),
		m_bLocked(false)
	{
		(const_cast<Lockable &>(m_lockable).*locker)();
		m_bLocked = true;
	}

	virtual ~ScopedLock()
	{
		if( m_bLocked ) 
		{
			(const_cast<Lockable &>(m_lockable).*m_unlocker)();
			m_bLocked = false;
		}
	}

	void unlock()
	{
		(const_cast<Lockable &>(m_lockable).*m_unlocker)();
		m_bLocked = false;
	}
		
	operator bool()
	{
		return m_bLocked;
	}
	
private:
	const Lockable &m_lockable;
	Unlocker m_unlocker;
	bool m_bLocked;
};

/** ----------------------------------------------------------------------------------
 * 
 */
template <typename Lockable>
struct ScopedReadLock : public ScopedLock<Lockable>
{
	ScopedReadLock(const Lockable& lockable) : 
		ScopedLock<Lockable>(lockable, &Lockable::lock_read, &Lockable::unlock_read) 
	{
	}
};

/** ----------------------------------------------------------------------------------
 * 
 */
template <typename Lockable>
struct ScopedWriteLock : public ScopedLock<Lockable>
{
	ScopedWriteLock(const Lockable& lockable) : 
		ScopedLock<Lockable>(lockable, &Lockable::lock_write, &Lockable::unlock_write) 
	{
	}
};

#endif // __COMMON_CONCURRENT_SCOPEDLOCK_HPP__
