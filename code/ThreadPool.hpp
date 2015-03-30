
#ifndef __COMMON_CONCURRENT_THREADPOOL_HPP__
#define __COMMON_CONCURRENT_THREADPOOL_HPP__

#include <common/common.hpp>
#include <common/concurrent/Task.hpp>
#include <common/container/WorkQueue.hpp>
#include <common/logging/Logger.hpp>

#include <atomic>
#include <list>

/** ----------------------------------------------------------------------------------
 * Function-based thread pool
 * 
 * Sample usage:
 * <code>
 * 		void threadpoolfunc() {
 * 		}
 * 
 * 		class threadpoolobj {
 * 			void operator()() {
 * 				// do stuff
 * 			}
 * 			void otherfunc(int i) {
 * 				// do stuff
 * 			}
 * 		};
 * 
 * 		// construct a thread pool with 3 pre-created threads
 * 		ml::common::concurrent::ThreadPool pool(3);
 * 
 * 		// call threadpoolfunc asynchronously
 * 		pool.execute(&threadpoolfunc);
 * 
 * 		threadpoolobj tpo;
 * 		// call tpo.otherfunc(1) asynchronously
 * 		pool.execute( boost::bind(&threadpoolobj::otherfunc, &tpo, 1) );
 * 
 * 		// call threadpoolobj::operator()() asynchronously
 * 		pool.execute(threadpoolobj());
 * </code>
 */

/*
 * ThreadPool Template which can be used to create ThreadPools
 * with different Queue data structures
 */
template < 
            typename WorkItem = boost::function0<void>,
            typename Queue = ml::common::container::WorkQueue < WorkItem >
         >
class ThreadPoolT
{
public:

	ThreadPoolT(uint32 capacity = 64, const std::string & name = "unnamed");
	~ThreadPoolT();

	void	stop();
	void	clear();
	uint32 	size() const;
	uint32  queueSize() const;
	void 	increaseCapacity(uint32 capacity);

	unsigned long long getInputCounter() const { return m_inputCounter.load(std::memory_order_relaxed); }
	unsigned long long getOutputCounter() const { return m_outputCounter.load(std::memory_order_relaxed); }

	void disable();
	void enable();

	template <typename Func> 
    bool execute(const Func &f)
    {
        LDBUG("Enqueueing into [%s], new size is %d", m_name.c_str(), m_queue.size()+1);
        bool result = m_queue.put( f );
        m_inputCounter.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

private:

	bool onProcess();

private:
	typedef boost::shared_ptr<Task> Task_ptr;
	
	Queue                      m_queue;
	std::list< Task_ptr >      m_tasks;
    const std::string          m_name;

    std::atomic_ullong m_inputCounter;
    std::atomic_ullong m_outputCounter;
};

/** 
 * Default ThreadPool is made of simple boost functors and
 * simple WorkQueue.
 */
typedef ThreadPoolT < > ThreadPool;

#include "ThreadPool.cpp"

 
#endif //__COMMON_CONCURRENT_THREADPOOL_HPP__
