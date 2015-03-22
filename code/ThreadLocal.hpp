#ifndef __COMMON_CONCURRENT_THREADLOCAL_HPP__
#define __COMMON_CONCURRENT_THREADLOCAL_HPP__

#include <pthread.h>
#include <common/common.hpp>
#include <common/exception/Exception.hpp>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 
 
namespace ml {
namespace common {
namespace concurrent {
	
/** ----------------------------------------------------------------------------------
 * Thread local wrapper
 * 
 * @warning declare objects of this type static
 * 
 */
template< class T>
class ThreadLocal
{
public:
	ThreadLocal();
	~ThreadLocal();
	
	void set( T* obj);
	
	T* get();

private:
	ThreadLocal( const ThreadLocal&);
	ThreadLocal& operator=( const ThreadLocal&);
	
	pthread_key_t	key;
};



template< class T>
ThreadLocal<T>::ThreadLocal()
{
	int res = pthread_key_create( &key, 0);
	
	if( res!=0)
	{
		THROW( exception::RuntimeException, "Creating key");
	}
}

template< class T>
ThreadLocal<T>::~ThreadLocal()
{
	pthread_key_delete( key);
}

template< class T>
void 
ThreadLocal<T>::set( 
	T* obj)
{
	int res = pthread_setspecific( key, static_cast< void*>( obj));
	
	if( res!=0)
	{
		THROW( exception::RuntimeException, "Setting key");
	}
}

template< class T>
T* 
ThreadLocal<T>::get()
{
	return static_cast<T*>( pthread_getspecific( key));
}

}
}
}

#endif 
