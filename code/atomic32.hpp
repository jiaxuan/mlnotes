
#ifndef __COMMON_CONCURRENT_ATOMIC_HPP__
#define __COMMON_CONCURRENT_ATOMIC_HPP__

#include <common/common.hpp>
#include <ostream>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 

namespace ml {
namespace common {
namespace concurrent {

/** ----------------------------------------------------------------------------------
 * Atomic 32-bit number class
 * 
 * Sample usage:
 * <code>
 * 		uint8 v1 = 1;
 * 		uint16 v2 = 2;
 * 		int16 v3 = 3;
 * 		uint32 v4 = 4;
 * 		int32 v5 = 5;
 * 		uint64 v6 = 6;
 * 		int64 v7 = 7;
 * 
 * 		atomic32 a0, a1(v1), a2(v2), a3(v3), a4(v4), a5(v5), a6(v6), a7(v7);
 * 
 * 		++a0;
 * 		--a1;
 * 		a2 += v1;
 * 		a3 -= v7;
 * 		a4 = v6;
 * 		a5 = a7.exchange(v7);
 * 		a6.compare_exchange(6, v5);
 * 
 * 		if( a1 < 2 )
 * 			std::cerr << "Blah!\n";
 * </code>
 */
class atomic32
{
public:

	/**
	 * Default constructor. Initial value is set to zero
	 */
	atomic32() : m_val(0)
	{ 
	}

	/**
	 * Copy constructor. Initial value is set to <code>val</code>
	 */
	template <typename T>	
	explicit atomic32(const T &val) : m_val(static_cast<uint32>(val))
	{
	}
	
	/**
	 * Cast operator. Atomically return our value
	 * 
	 * @return our current value as T
	 */
	template <typename T>
	operator T() const
	{
		return T( get() );
	}

	/**
	 * Compare for equality 
	 * 
	 * @param val the value to compare against
	 * @return true if the uint32 values are equal
	 */
	template <typename T>
	bool operator==(const T &val)
	{
		return get() == static_cast<uint32>(val);
	}

	/**
	 * Compare for inequality 
	 * 
	 * @param val the value to compare against
	 * @return true if the uint32 values are not equal
	 */
	template <typename T>
	bool operator!=(const T &val)
	{
		return get() != static_cast<uint32>(val);
	}
	
	/**
	 * Greater-than
	 * 
	 * @param val the value to compare against
	 * @return true if our value is greater than <code>val</code>
	 */
	template <typename T>
	bool operator>(const T &val)
	{
		return get() > static_cast<uint32>(val);
	}

	/**
	 * Greater-than or equal to
	 * 
	 * @param val the value to compare against
	 * @return true if our value is greater than or equal to <code>val</code>
	 */
	template <typename T>
	bool operator>=(const T &val)
	{
		return get() >= static_cast<uint32>(val);
	}

	/**
	 * Less-than
	 * 
	 * @param val the value to compare against
	 * @return true if our value is less than <code>val</code>
	 */
	template <typename T>
	bool operator<(const T &val)
	{
		return get() < static_cast<uint32>(val);
	}

	/**
	 * Less-than or equal to
	 * 
	 * @param val the value to compare against
	 * @return true if our value is less than or equal to <code>val</code>
	 */
	template <typename T>
	bool operator<=(const T &val)
	{
		return get() <= static_cast<uint32>(val);
	}
		
	/**
	 * Assignment operator. Atomically set our value to <code>val</code>
	 * 
	 * @param val the new value to set
	 * @return a references to ourselves
	 */
	template <typename T>
	atomic32 & operator=(const T &val)
	{
		// not very atomic -> set( static_cast<uint32>(val) );
        (void) xchg(static_cast<uint32>(val));
        
		return *this;
	}

	/**
	 * Increment and assign operator. Atomically increments our value by <code>val</code>
	 * 
	 * @param val the amount to increment by
	 * @return a references to ourselves
	 */
	template <typename T>
	atomic32 & operator+=(const T &val)
	{
		add( static_cast<uint32>(val) );
		return *this;
	}

	/**
	 * Decrement and assign operator. Atomically decrements our value by <code>val</code>
	 * 
	 * @param val the amount to decrement by
	 * @return a references to ourselves
	 */
	template <typename T>
	atomic32 & operator-=(const T &val)
	{
		sub( static_cast<uint32>(val) );
		return *this;
	}

	/**
	 * Pre-fix increment operator. Atomically increments our value
	 * 
	 * @return reference to ourselves
	 */	
	atomic32 & operator++()
	{
		inc();
		return *this;
	}

	/**
	 * Post-fix increment operator. Atomically increments our value
	 * 
	 * @return our previous value
	 */	
	uint32 operator++(int)
	{
		return add(1);
	}

	/**
	 * Pre-fix decrement operator. Atomically decrements our value
	 * 
	 * @return reference to ourselves
	 */	
	atomic32 & operator--()
	{
		dec();
		return *this;
	}

	/**
	 * Post-fix decrement operator. Atomically increments our value
	 * 
	 * @return our previous value
	 */	
	uint32 operator--(int)
	{
		return sub(1);
	}

	/**
	 * Atomically assign a new value <code>val</code>, and return the original value
	 * 
	 * @param val the new value to assign
	 * @return our original value
	 */
	template <typename T>
	T exchange(const T &val)
	{
		return T( xchg(static_cast<uint32>(val)) );
	}
	
	/**
	 * Atomically check if current value equals <code>cmp</code>, and if so, update its
	 * value to <code>val</code> and return the original value. Otherwise, return the
	 * current value.
	 * 
	 * @param cmp value to compare against our current value
	 * @param val new value to assign if our current value equals cmp
	 * @return our original value
	 */
	template <typename T>
	T compare_exchange(const T &cmp, const T &val)
	{
		return T( cmpx(static_cast<uint32>(cmp), static_cast<uint32>(val)) );
	}
	
	/**
	 * Atomically check if current value equals <code>cmp</code>, and if so, update its
	 * value to <code>val</code> and return true. Otherwise, return false.
	 *
	 * @param cmp value to compare against our current value
	 * @param val new value to assign if our current value equals cmp
	 * @return true if swap had place
	 */
	template <typename T>
	bool compare_and_swap(const T &cmp, const T &val)
	{
		return cas(static_cast<uint32>(cmp), static_cast<uint32>(val));
	}

private:
	uint32 get() const;
	// not atomic --> void set(uint32 val);
	void inc();
	void dec();
	uint32 add(uint32 val);
	uint32 sub(uint32 val);
	uint32 xchg(uint32 val);
	uint32 cmpx(uint32 cmp, uint32 val);
	bool cas(uint32 cmp, uint32 val);

private:	

	mutable volatile uint32 m_val;
};

}; // concurrent
}; // common
}; // ml

/**
 * Compare for equality
 * 
 * @param val the value to compare against
 * @param a the atomic32 to compare against
 * @return true if <code>val</code> is equal to <code>a</code>
 */
template <typename T>
bool operator==(const T &val, const ml::common::concurrent::atomic32 &a)
{
	return static_cast<uint32>(val) == static_cast<uint32>(a);
}

/**
 * Compare for inequality
 * 
 * @param val the value to compare against
 * @param a the atomic32 to compare against
 * @return true if <code>val</code> is not equal to <code>a</code>
 */
template <typename T>
bool operator!=(const T &val, const ml::common::concurrent::atomic32 &a)
{
	return static_cast<uint32>(val) != static_cast<uint32>(a);
}

/**
 * Less than
 * 
 * @param val the value to compare against
 * @param a the atomic32 to compare against
 * @return true if <code>val</code> is less than <code>a</code>
 */
template <typename T>
bool operator<(const T &val, const ml::common::concurrent::atomic32 &a)
{
	return static_cast<uint32>(val) < static_cast<uint32>(a);
}

/**
 * Less than or equal to
 * 
 * @param val the value to compare against
 * @param a the atomic32 to compare against
 * @return true if <code>val</code> is less than or equal to <code>a</code>
 */
template <typename T>
bool operator<=(const T &val, const ml::common::concurrent::atomic32 &a)
{
	return static_cast<uint32>(val) <= static_cast<uint32>(a);
}

/**
 * Greater than
 * 
 * @param val the value to compare against
 * @param a the atomic32 to compare against
 * @return true if <code>val</code> is greater than <code>a</code>
 */
template <typename T>
bool operator>(const T &val, const ml::common::concurrent::atomic32 &a)
{
	return static_cast<uint32>(val) > static_cast<uint32>(a);
}

/**
 * Greater than or equal to
 * 
 * @param val the value to compare against
 * @param a the atomic32 to compare against
 * @return true if <code>val</code> is greater than or equal to <code>a</code>
 */
template <typename T>
bool operator>=(const T &val, const ml::common::concurrent::atomic32 &a)
{
	return static_cast<uint32>(val) >= static_cast<uint32>(a);
}

/**
 * Write the value of <code>a</code> to output stream <code>os</code>
 * 
 * @param os the output stream to write to
 * @param a the <code>atomic32</code> value to output
 * @return reference to the output stream
 */
extern std::ostream & operator<<(std::ostream &os, const ml::common::concurrent::atomic32 &a);

#endif //__COMMON_CONCURRENT_ATOMIC_HPP__
