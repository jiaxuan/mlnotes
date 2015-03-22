
#include <common/concurrent/Condition.hpp>
#include <common/exception/Exception.hpp>
#include <errno.h>
#include <sys/time.h>

using namespace ml::common::concurrent;
using namespace ml::common::util;

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::Condition implementation
// ----------------------------------------------------------------------------------- 

namespace {

class ConditionAttributes : private virtual boost::noncopyable
{
public:

	explicit ConditionAttributes(bool shared);
	~ConditionAttributes();
	
private:

	friend class ml::common::concurrent::Condition;

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
		THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
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
		THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to notify", result);
}

/** ----------------------------------------------------------------------------------
 */
void Condition::notifyAll()
{
	int result = pthread_cond_broadcast(&m_cond);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to notifyAll", result);
}

/** ----------------------------------------------------------------------------------
 */
void Condition::wait( Mutex & mtx )
{
	int result = pthread_cond_wait(&m_cond, &mtx.m_mutex);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to wait", result);
}

/** ----------------------------------------------------------------------------------
 */
bool Condition::wait( Mutex & mtx, uint32 millis )
{
	return wait(mtx, Time(0, millis * 1000));
}

/** ----------------------------------------------------------------------------------
 */
bool Condition::wait( Mutex & mtx, const Time & delta )
{
	Time t( Time() + delta );
	
	int result = 0;
	struct timeval tv = t;
	struct timespec ts;
	
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;
	
	switch( result = pthread_cond_timedwait(&m_cond, &mtx.m_mutex, &ts) )
	{
	case ETIMEDOUT:
		return false;
	case 0:
		return true;
	default:
		THROW(ml::common::exception::RuntimeException, "Error encountered when attempting to timed wait", result);
	}
}

/** ----------------------------------------------------------------------------------
 */
ConditionAttributes::ConditionAttributes(bool shared)
{
	int result = pthread_condattr_init(&m_attr);
	if( 0 != result )
		THROW(ml::common::exception::RuntimeException, "Error encountered during initialization", result);
		
	if( 0 != (result = pthread_condattr_setpshared(&m_attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)) )
	{
		pthread_condattr_destroy(&m_attr);
		THROW(ml::common::exception::RuntimeException, "Error encountered while setting shared status", result);
	}
}

/** ----------------------------------------------------------------------------------
 */
ConditionAttributes::~ConditionAttributes()
{
	pthread_condattr_destroy(&m_attr);
}
