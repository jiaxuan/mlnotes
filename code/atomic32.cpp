
#include <common/concurrent/atomic32.hpp>

#if !defined(__i386__) && !defined(__x86_64__)
	#error "atomic32 implementation currently requires x86"
#endif

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent::atomi32 implementation
// ----------------------------------------------------------------------------------- 
uint32 ml::common::concurrent::atomic32::get() const
{ 
	return m_val;
}

/** ---------------------------------------------------------------------------------- 
 * Not very atomic 
void ml::common::concurrent::atomic32::set(uint32 val)
{ 
	m_val = val;
}
 */

/** ---------------------------------------------------------------------------------- 
 */
void ml::common::concurrent::atomic32::inc()
{ 
	asm volatile ("lock; incl %0"
					:"=m" (m_val)
					:"m" (m_val)
				);
}

/** ---------------------------------------------------------------------------------- 
 */
void ml::common::concurrent::atomic32::dec()
{ 
	asm volatile ("lock; decl %0"
					:"=m" (m_val)
					:"m" (m_val)
				);
}	

/** ---------------------------------------------------------------------------------- 
 */
uint32 ml::common::concurrent::atomic32::add(uint32 val)
{ 
    asm volatile ("lock; xaddl %0,%1"
					: "=r"(val), "=m"(m_val)
					: "0"(val), "m"(m_val)
				);
	return val;
}

/** ---------------------------------------------------------------------------------- 
 */
uint32 ml::common::concurrent::atomic32::sub(uint32 val)
{
	return add(static_cast<uint32>(-static_cast<int32>(val)));
}

/** ---------------------------------------------------------------------------------- 
 */
uint32 ml::common::concurrent::atomic32::xchg(uint32 val)
{ 
	uint32 prev = val;

    asm volatile ("lock; xchgl %0, %1"
					: "=r" (prev)
					: "m" (m_val), "0"(prev)
				);
	return prev;
}

/** ---------------------------------------------------------------------------------- 
 */
uint32 ml::common::concurrent::atomic32::cmpx(uint32 cmp, uint32 val) 
{
    uint32 prev;

    asm volatile ("lock; cmpxchgl %1, %2"             
					: "=a" (prev)               
					: "r" (val), "m" (m_val), "0"(cmp)
				);
    return prev;
}

/** ---------------------------------------------------------------------------------- 
 */
bool ml::common::concurrent::atomic32::cas(uint32 cmp, uint32 val)
{
    unsigned short result;

    asm volatile (
        "lock; cmpxchg %3, %1\n"
        "sete %b0\n"
        : "=r"(result), "+m"(m_val), "+a"(cmp)
        : "r"(val)
        : "memory", "cc"
        );

    return (bool) result;
}

/** ----------------------------------------------------------------------------------
 */
std::ostream & operator<<(std::ostream &os, const ml::common::concurrent::atomic32 &a)
{
	return os << static_cast<uint32>(a);
}
