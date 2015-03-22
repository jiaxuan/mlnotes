
#include <common/concurrent/Mutex.hpp>
#include <common/concurrent/Details.hpp>
#include <common/exception/Exception.hpp>
#include <errno.h>

using namespace ml::common::concurrent;

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::Mutex implementation
// ----------------------------------------------------------------------------------- 

namespace {

class MutexAttributes : private virtual boost::noncopyable
{
public:

	explicit MutexAttributes(Mutex::Type type, bool shared);	
	~MutexAttributes();
	
private:
	
	friend class ml::common::concurrent::Mutex;
	
	pthread_mutexattr_t m_attr;
};

}; // <internal> namespace


/** ----------------------------------------------------------------------------------
 * Construct an MT_Checked mutex by convention.
 */
Mutex::Mutex(Mutex::Type type, bool shared) : m_type(type)
{
	MutexAttributes attr(type, shared);

	int result = pthread_mutex_init(&m_mutex, &attr.m_attr);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
}

/** ----------------------------------------------------------------------------------
 */
Mutex::~Mutex()
{
	pthread_mutex_destroy(&m_mutex);
}

/** ----------------------------------------------------------------------------------
 */
void Mutex::lock()
{
	if (m_type == MT_Unchecked)
	{
		detail::lock_helper(&m_mutex, pthread_mutex_lock);
	}
	else
	{
		detail::lock_helper(&m_mutex, pthread_mutex_timedlock);
	}
}

/** ----------------------------------------------------------------------------------
 */
void Mutex::unlock()
{
	detail::unlock_helper(&m_mutex, pthread_mutex_unlock);
}

/** ----------------------------------------------------------------------------------
 */
MutexAttributes::MutexAttributes(Mutex::Type type, bool shared)
{
	int result = pthread_mutexattr_init(&m_attr);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
		
	try 
	{
		if( 0 != (result = pthread_mutexattr_settype(&m_attr, static_cast<int>(type))) )
			THROW(ml::common::exception::RuntimeException, "Error encountered while setting type", result);

		if( 0 != (result = pthread_mutexattr_setpshared(&m_attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)) )
			THROW(ml::common::exception::RuntimeException, "Error encountered while setting shared status", result);
	}
	catch( ... ) 
	{
		pthread_mutexattr_destroy(&m_attr);
		throw;
	}
}

/** ----------------------------------------------------------------------------------
 */
MutexAttributes::~MutexAttributes()
{
	pthread_mutexattr_destroy(&m_attr);
}
