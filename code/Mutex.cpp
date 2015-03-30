
#include "Mutex.hpp"
#include "Details.hpp"
// #include <common/exception/Exception.hpp>
#include <errno.h>


namespace {

class MutexAttributes : private virtual boost::noncopyable
{
public:

	explicit MutexAttributes(Mutex::Type type, bool shared);	
	~MutexAttributes();
	
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
		throw("Error encountered during initialization");
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
		lock_helper(&m_mutex, pthread_mutex_lock);
	}
	else
	{
		lock_helper(&m_mutex, pthread_mutex_timedlock);
	}
}

/** ----------------------------------------------------------------------------------
 */
void Mutex::unlock()
{
	unlock_helper(&m_mutex, pthread_mutex_unlock);
}

/** ----------------------------------------------------------------------------------
 */
MutexAttributes::MutexAttributes(Mutex::Type type, bool shared)
{
	int result = pthread_mutexattr_init(&m_attr);
	if( 0 != result )
		throw("Error encountered during initialization");
		
	try 
	{
		if( 0 != (result = pthread_mutexattr_settype(&m_attr, static_cast<int>(type))) )
			throw("Error encountered while setting type");

		if( 0 != (result = pthread_mutexattr_setpshared(&m_attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)) )
			throw("Error encountered while setting shared status");
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
