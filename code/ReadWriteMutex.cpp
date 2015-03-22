
#include <common/concurrent/ReadWriteMutex.hpp>
#include <common/concurrent/Details.hpp>
#include <common/exception/Exception.hpp>
#include <errno.h>

using namespace ml::common::concurrent;

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::ReadWriteMutex implementation
// ----------------------------------------------------------------------------------- 

namespace {

class ReadWriteAttributes : private virtual boost::noncopyable
{
public:

	explicit ReadWriteAttributes(bool shared);
	~ReadWriteAttributes();
	
private:

	friend class ml::common::concurrent::ReadWriteMutex;

	pthread_rwlockattr_t m_attr;	
};
	
}; // <internal> namespace

/** ----------------------------------------------------------------------------------
 */
ReadWriteMutex::ReadWriteMutex(bool shared)
{
	ReadWriteAttributes attr(shared);

	int result = pthread_rwlock_init(&m_rwlock, &attr.m_attr);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
}

/** ----------------------------------------------------------------------------------
 */
ReadWriteMutex::~ReadWriteMutex()
{
	pthread_rwlock_destroy(&m_rwlock);
}

/** ----------------------------------------------------------------------------------
 */
void ReadWriteMutex::lock_read()
{
	detail::lock_helper(&m_rwlock, pthread_rwlock_timedrdlock);
}

/** ----------------------------------------------------------------------------------
 */
void ReadWriteMutex::lock_write()
{
	detail::lock_helper(&m_rwlock, pthread_rwlock_timedwrlock);
}

/** ----------------------------------------------------------------------------------
 */
void ReadWriteMutex::unlock_read()
{
	detail::unlock_helper(&m_rwlock, pthread_rwlock_unlock);
}

/** ----------------------------------------------------------------------------------
 */
void ReadWriteMutex::unlock_write()
{
	detail::unlock_helper(&m_rwlock, pthread_rwlock_unlock);
}

/** ----------------------------------------------------------------------------------
 */
ReadWriteAttributes::ReadWriteAttributes(bool shared)
{
	int result = pthread_rwlockattr_init(&m_attr);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
		
	if( 0 != (result = pthread_rwlockattr_setpshared(&m_attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)) )
	{
		pthread_rwlockattr_destroy(&m_attr);
		THROW(ml::common::exception::RuntimeException, "Error encountered while setting shared status", result);
	}
}

/** ----------------------------------------------------------------------------------
 */
ReadWriteAttributes::~ReadWriteAttributes()
{
	pthread_rwlockattr_destroy(&m_attr);
}
