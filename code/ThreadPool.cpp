

using namespace ml::common::concurrent;
#include <common/text/Format.hpp>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::ThreadPoolT implementation
// ----------------------------------------------------------------------------------- 

template < class WorkItem, class Queue >
ThreadPoolT < WorkItem, Queue >::ThreadPoolT(uint32 capacity, const std::string & name)
: m_name(name), m_inputCounter(0), m_outputCounter(0)
{
	try
	{
		increaseCapacity(capacity);
	}
	catch (...)
	{
		stop(); // need to disable the work item queue otherwise threads can't shutdown
		throw;
	}
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
ThreadPoolT < WorkItem, Queue >::~ThreadPoolT()
{
	stop();
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
void ThreadPoolT < WorkItem, Queue >::stop()
{
	m_queue.disable();
	m_tasks.clear();
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
void ThreadPoolT < WorkItem, Queue >::clear()
{
	m_queue.clear();
}


/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
void ThreadPoolT < WorkItem, Queue >::disable()
{
	m_queue.disable();
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
void ThreadPoolT < WorkItem, Queue >::enable()
{
	m_queue.enable();
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
void ThreadPoolT < WorkItem, Queue >::increaseCapacity(uint32 capacity)
{
    LINFO("ThreadPoolT(%s): Capacity %d", m_name.c_str(), capacity);
    
	while( capacity-- > 0 ) 
    {
        std::string taskName(m_name);
        taskName += ":";
        taskName += mlc::text::Format::int32ToString(capacity);
        LINFO("ThreadPoolT(%s): Creating %s thread task", m_name.c_str(), taskName.c_str());
		m_tasks.push_back( Task_ptr(new Task(boost::bind(&ThreadPoolT::onProcess, this), taskName.c_str())) );
    }
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
uint32 ThreadPoolT < WorkItem, Queue >::size() const
{
	return static_cast<uint32>(m_tasks.size());
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
uint32 ThreadPoolT < WorkItem, Queue >::queueSize() const
{
	return m_queue.size();
}

/** ----------------------------------------------------------------------------------
 */
template < class WorkItem, class Queue >
bool ThreadPoolT < WorkItem, Queue >::onProcess()
{
	WorkItem item;
	
	if( ! m_queue.get(item) )
	{
		return mlc::concurrent::Thread::STOP;
	}
	else	
	{
		try
		{
			item();
			m_outputCounter.fetch_add(1, std::memory_order_relaxed);
		}
		catch (const std::exception &e)
		{
			// log the exception, otherwise we silently stop processing
			LCRIT("Exception caught in thread-pool. %s", e.what());
			// squash the exception, we don't want our threads to die.
		}
		catch (...)
		{
			// log the exception, otherwise we silently stop processing
			LCRIT("Exception caught in thread-pool.");
			// squash the exception, we don't want our threads to die.
		}
		return mlc::concurrent::Thread::CONTINUE;
	}
}
