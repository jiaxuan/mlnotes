#include "Thread.hpp"
// #include "common/exception/Exception.hpp"
#include "Logger.hpp"
// #include "common/diagnostics/StackTrace.hpp"

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::Thread implementation
// ----------------------------------------------------------------------------------- 

const bool Thread::STOP 		= true;
const bool Thread::CONTINUE 	= false;

/** ----------------------------------------------------------------------------------
 */
Thread::Thread(const char * desc) :
	m_desc(desc),
	m_id(0),
	m_state(TS_READY)
{
    // m_createStack = ml::common::diagnostics::StackTrace().toString();
}

/** ----------------------------------------------------------------------------------
 */
Thread::~Thread()
{
	if( m_state != TS_READY )
	{
		// LWARN("Thread '%s' tearing down without having been stopped & joined.\n%s", 
		// 	  m_desc.c_str(),
		// 	  mlc::diagnostics::StackTrace().toString().c_str()
		// 	  );
			  
		stop();
		join();
	}
}

/** ----------------------------------------------------------------------------------
 */
Thread::Identifier Thread::self()
{
	return static_cast<Thread::Identifier>(pthread_self());
}

/** ----------------------------------------------------------------------------------
 */
Thread::Identifier Thread::id() const
{
	// if( 0 == (state() & (TS_RUNNING | TS_STOPPING)) )
	// 	LWARN("Thread::id() called for thread '%s' which is not running", m_desc.c_str());
		
	return m_id;
}

/** ----------------------------------------------------------------------------------
 */
void Thread::start()
{
    if( state() != TS_READY )
        throw("Attempting to start() a thread that has not been fully stopped");
		// THROW(ml::common::exception::InvalidStateException, m_desc +
        //       " : Attempting to start() a thread that has not been fully stopped");
	
	// Set this before starting the thread to avoid a race condition where the onThreadStart can
	// be called before the state is set to TS_STARTING
	m_state = TS_STARTING;

	int result;
	if( 0 != (result = pthread_create(&m_id, NULL, Thread::run, this)) )
	{
		m_state = TS_READY;
        throw(" An error occurred when attempting to start the underlying thread");
		// THROW(ml::common::exception::RuntimeException, m_desc + 
        //       " : An error occurred when attempting to start the underlying thread: " +
        //       strerror(errno), result);
	}
}

/** ----------------------------------------------------------------------------------
 */
void Thread::stop()
{
	// If the thread has been started, or is running ...
	if( state() & (TS_STARTING | TS_RUNNING) )
		m_state = TS_STOPPING;
}

/** ----------------------------------------------------------------------------------
 */
void Thread::join() 
{
	if( m_state != TS_READY )
	{
		if( 0 != pthread_join( m_id, NULL ) )
		{
			// LWARN("Join failed on thread '%s'.\n%s", 
			// 	  m_desc.c_str(),
			// 	  mlc::diagnostics::StackTrace().toString().c_str()
			// 	  );
		}
		else
		{
			// If the join succeeded, we're not ready to be restarted
			m_state = TS_READY;
		}
	}
}

/** ----------------------------------------------------------------------------------
 */
Thread::State Thread::state() const
{
	return m_state;
}

/** ----------------------------------------------------------------------------------
 */
const std::string & Thread::desc() const
{
	return m_desc;
}

/** ----------------------------------------------------------------------------------
 */
void * Thread::run(void *pvThread)
{
	Thread &thread = * static_cast<Thread *>(pvThread);
	// LMETA("Thread Created By %s" , thread.m_createStack.c_str() );
	try 
	{
		thread.onThreadStart();
		thread.m_state.compare_exchange(TS_STARTING, TS_RUNNING);
		
		try 
		{
			while( (thread.state() != TS_STOPPING) && (thread.onProcess() == Thread::CONTINUE) )
				;
		}
		catch( const std::exception &e ) 
		{
			LCRIT("%s thread(%s)", e.what(), thread.desc().c_str());
		}
		catch( ... )
		{
			LCRIT("Caught an unknown exception : thread(%s)", thread.desc().c_str());
		}

		thread.m_state = TS_STOPPING;
		thread.onThreadStop();
	}
	catch( const std::exception &e ) 
	{
		LCRIT("%s thread(%s)", e.what(), thread.desc().c_str());
	}
	catch( ... )
	{
		LCRIT("Caught an unknown exception : thread(%s)", thread.desc().c_str());
	}

	thread.m_state = TS_STOPPED;
	return NULL;	
}
