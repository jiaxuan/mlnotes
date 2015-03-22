
#ifndef __COMMON_CONCURRENT_THREAD_HPP__
#define __COMMON_CONCURRENT_THREAD_HPP__

#include <common/common.hpp>
#include <common/concurrent/atomic32.hpp>
#include <string>
#include <pthread.h>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 

namespace ml {
namespace common {
namespace concurrent {

/** ----------------------------------------------------------------------------------
 * Thread base class implementation. 
 * 
 * @warning Derived class implementations <strong>must</strong> ensure to call 
 * 			<code>stop()</code> and <code>join()</code> when tearing down to ensure 
 * 			proper thread termination and cleanup
 * 
 * Sample code:
 * <code>
 * 
 * 		class MyThread : private Thread
 * 		{
 * 		public:
 * 			MyThread() : m_count(0) {
 * 				std::cout << "MyThread()\n";
 * 				// start the thread running
 * 				start();
 * 			}
 * 			~MyThread() {
 * 				stop();
 * 				// make sure we clean up properly
 * 				join();
 * 				std::cout << "~MyThread()\n";
 * 			}
 * 
 * 		private:
 * 			void onThreadStart() {
 * 				std::cout << "Started\n";
 * 			}
 * 
 * 			bool onProcess() {
 * 				if( ++m_count > 5 )
 * 					return Thread::STOP;
 * 				std::cout << "Run #" << m_count << "\n";
 * 				return Thread::CONTINUE;
 * 			}
 * 
 * 			void onThreadStop() {
 * 				std::cout << "Stopped\n";
 * 			}
 * 		private:
 * 			int m_count;
 * 		};
 * 
 * 		int main()
 * 		{
 * 			MyThread mythread;
 * 		}
 * 
 * </code>
 */
 	
class Thread : protected virtual boost::noncopyable
{
public:

	typedef pthread_t Identifier;
	
	enum State 
	{
		TS_READY	= 1,
		TS_STARTING	= 2,
		TS_RUNNING	= 4,
		TS_STOPPING	= 8,
		TS_STOPPED  = 16
	};
	
	static const bool STOP;
	static const bool CONTINUE;
	
public:

	/**
	 * Constructor initializes internal members. 
	 * 
	 * @param desc a description of the task this thread is performing, which may later be used for
	 * 			   tracking purposes
	 */
	Thread(const char * desc = "<Anonymous>");	
	/**
	 * virtual destructor to support safe deletion through base class pointer
	 */
	virtual ~Thread();

	/**
	 * @return the Identifier value corresponding to the current thread
	 */
	static Thread::Identifier self();
	
protected:
	
	/**
	 * @warning this function should not be overridden
	 * 
	 * @return the Identifier value corresponding to the thread instance
	 */
	Thread::Identifier id() const;
	
	/**
	 * Call this function to start executing the thread, changing the state to {@link State::TS_STARTING}. 
	 * The derived instance should expect an impending call to onThreadStart()
	 * 
	 * @warning this function should not be overridden
	 * 
	 * @throw ml::common::exception::InvalidStateException if called more than once
	 * @throw ml::common::concurrent::RuntimeException if underlying thread creation fails
	 */
	void start();

	/**
	 * Call this function to commence the thread termination process.
	 * 
	 * No action will be taken if the thread has not yet been started, or if the thread has already
	 * been stopped.
	 * 
	 * Calling on a running thread will change to state to {@link TS_STOPPING}, eventually result in 
	 * a call to <code>onThreadStop()</code>, followed by thread termination
	 * 
	 * @warning this function should not be overridden
	 */
	void stop();

	/**
	 * Call this function to block until this thread has terminated
	 * 
	 * The function will return immediately if either the thread has not yet been started, or if the
	 * thread has already been stopped.
	 * 
	 * @warning this function should not be overridden
	 */
	void join();
	
	/**
	 * @warning this function should not be overridden
	 * 
	 * @return the current thread state
	 */
	Thread::State state() const;

	/**
	 * @warning: this function should not be overridden
	 * 
	 * @return the current thread description
	 */
	 const std::string & desc() const;

private:

	/**
	 * Called when the thread initially commences execution. If an exception is thrown from 
	 * this function, the thread will terminate <strong>without</strong> <code>onThreadStop()</code> being 
	 * called.
	 * 
	 * Calling stop() from within this function will result in the thread terminating on successful
	 * completion of <code>onThreadStart()</code>, and <strong>will</strong> result in <code>onThreadStop()</code> 
	 * being called
	 * 
	 * @warning This should neither be publicly exposed, or called directly by deriving implementations
	 */
	virtual void onThreadStart() { }	
	
	/**
	 * Called if <code>onThreadStart()</code> completes successfully and <code>stop()</code> hasn't been 
	 * requested.
	 * 
	 * The thread's state will change to {@link State::TS_RUNNING} and will continue running and <code>onProcess()</code> 
	 * will be repeatedly called until either:
	 * 		- <code>stop()</code> has been requested OR
	 * 		- <code>onProcess()</code> returns {@link Thread::STOP}
	 * 
	 * @warning This should neither be publicly exposed, or called directly by deriving implementations
	 * 
	 * @return {@link Thread::STOP} to signify that the thread can terminate and <code>onThreadStop()</code> should be called 
	 * 		   {@link Thread::CONTINUE} to signify that processing is not complete and <code>onProcess()</code> should be called again
	 */
	virtual bool onProcess() = 0;
	
	/**
	 * Called immediately prior to thread termination, provided the call to <code>onThreadStart()</code> 
	 * returned successfully
	 * 
	 * @warning This should neither be publicly exposed, or called directly by deriving implementations
	 */
	virtual void onThreadStop() { }

private:

	static void * run(void *pvThread);

private:	
	const std::string m_desc;
	Identifier m_id;
	ml::common::concurrent::atomic32 m_state;
	std::string m_createStack;
};
		
}; // concurrent
}; // common
}; // ml

#endif //__COMMON_CONCURRENT_THREAD_HPP__
