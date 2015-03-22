#ifndef __COMMON_CONCURRENT_THREADPOOL_REACTOR_HPP__
#define __COMMON_CONCURRENT_THREADPOOL_REACTOR_HPP__

#include <common/concurrent/ThreadPool.hpp>
#include <common/text/Format.hpp>
#include <common/text/StringBuilder.hpp>
#include <common/util/Shims.hpp>
#include <vector>

// -----------------------------------------------------------------------------------
// ml::common::concurrent
// -----------------------------------------------------------------------------------

namespace ml {
namespace common {
namespace concurrent {

using namespace ml::common::util::shims;

/** ----------------------------------------------------------------------------------
 * Pool of ThreadPools
 * You can have your own way of selecting which threadpool to choose by providing
 * your own AffinityFunctor.
 *
 * Note that increase in capacity is not thread-safe as it is in ThreadPool class
 */
struct NonStrictRoundRobin
{
    template < typename T >
    uint64 operator ()(T const &)
    {
        static uint64 count(0);
        return count++;
    }
};

template <
            typename WorkItem = boost::function0<void>,
            typename Queue = ml::common::container::WorkQueue < WorkItem >,
            typename AffinityFunctor = NonStrictRoundRobin
         >
class ThreadPoolReactor
{
public:

	ThreadPoolReactor(uint32 capacity, uint32 capacityPerPool, const std::string & name = "unnamed");
	~ThreadPoolReactor();

	void	stop();
	uint32 	capacity() const;
    uint32  perPoolCapacity() const;
	void 	increaseCapacity(uint32 capacity, uint32 capacityPerPool);
    void    increasePerPoolCapacity(uint32 capacityPerPool);

    std::string toString() const;

    bool execute(const WorkItem & item)
    {
        int index = m_affinity(item) % capacity();
        // FIXME: item doesn't have toString() method
        // LDBUG("Enqueueing into queue no. %d of %s, item [%s]", index, m_name.c_str(), item.toString().c_str());
        return m_threadPools[index]->execute(item);
    }

    //	Ok...  I hope you know what you are doing...
    bool execute(const WorkItem & item, uint64 const hashCode)
    {
        int index = hashCode % capacity();

        return m_threadPools[index]->execute(item);
    }

private:
    typedef ThreadPoolT<WorkItem, Queue> Pool;
    typedef boost::shared_ptr< Pool > Pool_ptr;
    typedef std::deque<Pool_ptr> PoolsArray;

	PoolsArray                  m_threadPools;
    AffinityFunctor             m_affinity;
    const std::string           m_name;
};



template < typename WorkItem, typename Queue, typename AffinityFunctor >
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::ThreadPoolReactor(uint32 capacity, uint32 capacityPerPool, const std::string & name)
: m_name(name)
{
    increaseCapacity(capacity, capacityPerPool);
}

/** ----------------------------------------------------------------------------------
 */
template < typename WorkItem, typename Queue, typename AffinityFunctor >
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::~ThreadPoolReactor()
{
    stop();
}

/** ----------------------------------------------------------------------------------
 */
template < typename WorkItem, typename Queue, typename AffinityFunctor >
void
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::stop()
{
    foreach(iter, m_threadPools)
        (*iter)->stop();
}

/** ----------------------------------------------------------------------------------
 */
template < typename WorkItem, typename Queue, typename AffinityFunctor >
void
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::increaseCapacity(uint32 capacity, uint32 capacityPerPool)
{
    LINFO("ThreadPoolReactor(%s): Capacity %d CapacityPerPool %d", m_name.c_str(), capacity, capacityPerPool);

    int oldPerPoolCapacity = perPoolCapacity();
    increasePerPoolCapacity(capacityPerPool);

    while( capacity-- > 0 )
    {
        std::string threadPoolName(m_name);
        threadPoolName += ":";
        threadPoolName += mlc::text::Format::int32ToString(capacity);
        LINFO("ThreadPoolReactor(%s): Creating %s threadpool with capacity %d", m_name.c_str(), threadPoolName.c_str(), capacityPerPool);
        m_threadPools.push_back(Pool_ptr(new Pool(oldPerPoolCapacity + capacityPerPool, threadPoolName)));
    }
}

/** ----------------------------------------------------------------------------------
 */
template < typename WorkItem, typename Queue, typename AffinityFunctor >
void
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::increasePerPoolCapacity(uint32 capacityPerPool)
{
    foreach(iter, m_threadPools)
        (*iter)->increaseCapacity(capacityPerPool);
}

/** ----------------------------------------------------------------------------------
 */
template < typename WorkItem, typename Queue, typename AffinityFunctor >
uint32
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::capacity() const
{
    return static_cast<uint32>(m_threadPools.size());
}

/** ----------------------------------------------------------------------------------
 */
template < typename WorkItem, typename Queue, typename AffinityFunctor >
uint32
ThreadPoolReactor < WorkItem, Queue, AffinityFunctor >::perPoolCapacity() const
{
    if (capacity() > 0)
        return static_cast<uint32>(m_threadPools[0]->size());

    return 0;
}

template < typename WorkItem, typename Queue, typename AffinityFunctor >
std::string ThreadPoolReactor<WorkItem, Queue, AffinityFunctor>::toString() const
{
	mlc::text::StringBuilder sb;
    uint32 total = 0;

	for (const auto & i : m_threadPools)
	{
        uint32 queueSize = i->queueSize();
        total += queueSize;

		sb << "[" << queueSize << ", " << i->getInputCounter() << "/" << i->getOutputCounter() << "] ";
	}

    sb << total;

	return sb.str();
}

}; // concurrent
}; // common
}; // ml

#endif //__COMMON_CONCURRENT_THREADPOOL_REACTOR_HPP__
