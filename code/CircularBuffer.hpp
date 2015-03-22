#ifndef __COMMON_CONTAINER_CIRCULARBUFFER_HPP__
#define __COMMON_CONTAINER_CIRCULARBUFFER_HPP__

#include <common/common.hpp>

// ----------------------------------------------------------------------------------- 
// ml::common::container
// ----------------------------------------------------------------------------------- 

namespace ml {
namespace common {
namespace container {

/** ----------------------------------------------------------------------------------
 * simple circular buffer implementation.
 * 
 * @warning Due to the nature of the implementation, stored <strong>T</strong> object instances will persist
 * 			for the lifetime of the buffer, or until overridden through a <code>write</code> operation.
 */
template <typename T>
class CircularBuffer
{
public:

	enum {
		E_NOSPACE = -1,
		E_NODATA = -2
	};

public:
	/**
	 * Constructor
	 * 
	 * @param cap the capacity of this buffer
	 */
	CircularBuffer(uint32 cap);

	/**
	 * Copy constructor
	 */
	CircularBuffer(const CircularBuffer<T> & cbt);
	
	/**
	 * Destructor
	 */
	~CircularBuffer();

	/**
	 */
	CircularBuffer<T> & operator=(const CircularBuffer<T> & cbt);
	
	/**
	 * @return the current capacity
	 */
	uint32 capacity() const;
	
	/**
	 * @return the current size
	 */
	uint32 size() const;
	
	/**
	 * Write the contents of <code>pt</code> to this buffer
	 * 
	 * @param pt the buffer from which to copy
	 * @param count the number of elements to copy from pt
	 * @return the number of elements written
	 */
	int32 write(const T * pt, uint32 count);
	
	/**
	 * Read the contents of this buffer into <code>pt</code>.
	 * After reading, the elements read are no longer accessible from this buffer
	 * 
	 * @param pt the destination buffer
	 * @param count the number of elements to read
	 * @return the number of elements read
	 */
	int32 read(T *pt, uint32 count);
	
	/**
	 * Skip past <code>count</code> elements in this buffer
	 * 
	 * @param the number of elements to skip
	 * @return the number of elements skipped
	 */
	int32 skip(uint32 count);
	
	/**
	 * Peek from the buffer into <code>pt</code>
	 * After peeking, the elements remain accessible through this buffer
	 * 
	 * @param pt the destination buffer
	 * @param count the number of elements to peek
	 * @return the number of elements peeked
	 */
	int32 peek(T *pt, uint32 count) const;

private:
	uint32 MAX_SIZE;
	uint32 CAPACITY;
	uint32 m_begin;
	uint32 m_end;
	uint32 m_size;
	T * m_buffer;
};

/** ----------------------------------------------------------------------------------
 */
template <typename T>
CircularBuffer<T>::CircularBuffer(uint32 cap) :
	MAX_SIZE(cap),
	CAPACITY(cap+1),
	m_begin(0),
	m_end(0),
	m_size(0),
	m_buffer(NULL)
{
	m_buffer = new T[CAPACITY];
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
CircularBuffer<T>::CircularBuffer(const CircularBuffer<T> & cbt) :
	MAX_SIZE(cbt.MAX_SIZE),
	CAPACITY(cbt.CAPACITY),
	m_begin(cbt.m_begin),
	m_end(cbt.m_end),
	m_size(cbt.m_size),
	m_buffer(NULL)
{
	m_buffer = new T[CAPACITY];
	std::copy( &cbt.m_buffer[0], &cbt.m_buffer[CAPACITY], &m_buffer[0] );
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
CircularBuffer<T>::~CircularBuffer()
{
	delete [] m_buffer;
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
CircularBuffer<T> & CircularBuffer<T>::operator=(const CircularBuffer<T> & cbt)
{
	if( &cbt == this )
		return *this;
	
	delete [] m_buffer;

	MAX_SIZE = cbt.MAX_SIZE;
	CAPACITY = cbt.CAPACITY;
	m_begin = cbt.m_begin;
	m_end = cbt.m_end;
	m_size = cbt.m_size;
	m_buffer = new T[CAPACITY];
	std::copy( &cbt.m_buffer[0], &cbt.m_buffer[CAPACITY], &m_buffer[0] );
	
	return *this;
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
uint32 CircularBuffer<T>::capacity() const
{
	return MAX_SIZE;
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
uint32 CircularBuffer<T>::size() const
{
	return m_size;
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
int32 CircularBuffer<T>::write(const T * pt, uint32 count)
{
	// Check to see if we're going to burst
	uint32 newsize = m_size + count;
	if( newsize > MAX_SIZE )
		return E_NOSPACE;

	uint32 onright;
	if( m_end < m_begin || count <= (onright = CAPACITY - m_end) )
	{
		std::copy( &pt[0], &pt[count], &m_buffer[m_end] );
		m_end = (m_end + count) % CAPACITY;
	}
	else
	{
		std::copy( &pt[0], &pt[onright], &m_buffer[m_end] );
		std::copy( &pt[onright], &pt[count], &m_buffer[0] );
		m_end = count - onright;
	}

	m_size = newsize;
	return count;
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
int32 CircularBuffer<T>::read(T *pt, uint32 count)
{
	if( E_NODATA == peek(pt, count) )
		return E_NODATA;
		
	return skip(count);
}

/** ----------------------------------------------------------------------------------
 */
template <typename T>
int32 CircularBuffer<T>::skip(uint32 count)
{
	if( m_size <= count )
	{
		count = m_size;
		m_begin = 0;
		m_end = 0;
	}
	else 
	{
		m_begin = (m_begin + count) % CAPACITY;
	}

	m_size -= count;			
	return count;
}


/** ----------------------------------------------------------------------------------
 */
template <typename T>
int32 CircularBuffer<T>::peek(T *pt, uint32 count) const
{
	if( count > m_size )
		return E_NODATA;

	uint32 onright;
	if( m_begin < m_end || count <= (onright = CAPACITY - m_begin) ) 
	{
		std::copy( &m_buffer[m_begin], &m_buffer[m_begin + count], &pt[0] );
	}
	else
	{
		std::copy( &m_buffer[m_begin], &m_buffer[m_begin + onright], &pt[0] );
		std::copy( &m_buffer[0], &m_buffer[count - onright], &pt[onright]);
	}
		
	return count;
}

}; // container
}; // common
}; // ml

#endif // __COMMON_CONTAINER_CIRCULARBUFFER_HPP__
