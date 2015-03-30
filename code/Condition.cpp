
#include "Condition.hpp"
#include <errno.h>
#include <sys/time.h>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::Condition implementation
// ----------------------------------------------------------------------------------- 

namespace {

class ConditionAttributes : private virtual boost::noncopyable
{
public:

	explicit ConditionAttributes(bool shared);
	~ConditionAttributes();
	

	pthread_condattr_t m_attr;	
};

}; // <internal> namespace

/** ----------------------------------------------------------------------------------
 * Construct an Condition
 */
Condition::Condition(bool shared)
{
	ConditionAttributes attr(shared);

	int result = pthread_cond_init(&m_cond, &attr.m_attr);
	if( 0 != result )
		// THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
		throw("Error encountered during initialization");
}

/** ----------------------------------------------------------------------------------
 */
Condition::~Condition()
{
	pthread_cond_destroy(&m_cond);
}

/** ----------------------------------------------------------------------------------
 */
void Condition::notify()
{
	int result = pthread_cond_signal(&m_cond);
	if( 0 != result )
		// THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to notify", result);
		throw("Error encountered when attempting to notify");
}

/** ----------------------------------------------------------------------------------
 */
void Condition::notifyAll()
{
	int result = pthread_cond_broadcast(&m_cond);
	if( 0 != result )
		// THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to notifyAll", result);
		throw("Error encountered when attempting to notifyAll");
}

/** ----------------------------------------------------------------------------------
 */
void Condition::wait( Mutex & mtx )
{
	int result = pthread_cond_wait(&m_cond, &mtx.m_mutex);
	if( 0 != result )
		// THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to wait", result);
		throw("Error encountered when attempting to wait");
}

/** ----------------------------------------------------------------------------------
 */
bool Condition::wait( Mutex & mtx, uint32 millis )
{
	int result = 0;
	struct timespec ts;
	
	ts.tv_sec = millis/1000;
	ts.tv_nsec = (millis % 1000) * 1000000;
	
	switch( result = pthread_cond_timedwait(&m_cond, &mtx.m_mutex, &ts) )
	{
	case ETIMEDOUT:
		return false;
	case 0:
		return true;
	default:
		// THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to timed wait", result);
		throw("Error encountered when attempting to timed wait");
	}
}

/** ----------------------------------------------------------------------------------
 */
ConditionAttributes::ConditionAttributes(bool shared)
{
	int result = pthread_condattr_init(&m_attr);
	if( 0 != result )
		// THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
		throw("Error encountered during initialization");
		
	if( 0 != (result = pthread_condattr_setpshared(&m_attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)) )
	{
		pthread_condattr_destroy(&m_attr);
		// THROW(ml::common::exception::RuntimeException, "Error encountered while setting shared status", result);
		throw("Error encountered while setting shared status");
	}
}

/** ----------------------------------------------------------------------------------
 */
ConditionAttributes::~ConditionAttributes()
{
	pthread_condattr_destroy(&m_attr);
}
