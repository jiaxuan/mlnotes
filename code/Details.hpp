#ifndef __COMMON_CONCURRENT_DETAILS_HPP__
#define __COMMON_CONCURRENT_DETAILS_HPP__

// #include <common/exception/Exception.hpp>
#include "Logger.hpp"
#include <sys/time.h>
#include <pthread.h>

template <typename Lock, typename LockFunction>
struct LockHelper;

template <typename Lock>
struct LockHelper<Lock, int (*)(Lock *, const struct timespec *)>
{
	typedef int (* LockFunction)(Lock *, const struct timespec *);

	static void lock(Lock * lockee, LockFunction locker)
	{
		static const int lock_timeout = 60; // timeout in 60 seconds
		std::size_t counter = 0;

		struct timespec ts;
		ts.tv_nsec = 0;

		while (true)
		{
			ts.tv_sec = time(nullptr) + lock_timeout;

			int result = locker(lockee, &ts);

			switch( result )
			{
			case 0:	// OK
				return;

			case ETIMEDOUT:
				if (++counter > 5)
				{
					LCRIT("Lock [%p] can't be acquired for %d seconds, thread might be deadlocked\n",
							// lockee, lock_timeout * counter, mlc::diagnostics::StackTrace().toString().c_str());
							lockee, lock_timeout * counter);
				}
				else
				{
					LWARN("Lock [%p] can't be acquired for %d seconds\n",
							// lockee, lock_timeout * counter, mlc::diagnostics::StackTrace().toString().c_str());
							lockee, lock_timeout * counter);
				}

				break;

			case EDEADLK:
				throw("Deadlock detected: attempting to acquire the lock while holding read/write lock");
				break;

			default:
				throw("Error encountered while attempting to aquire a lock");
			}
		}
	}
};

template <typename Lock>
struct LockHelper<Lock, int (*)(Lock *)>
{
	typedef int (* LockFunction)(Lock *);

	static void lock(Lock * lockee, LockFunction locker)
	{
		int result = locker(lockee);

		if (result)
		{
			throw("Error encountered while attempting to aquire a lock");
		}
	}
};

template <typename Lock, typename LockFunction>
void lock_helper(Lock * lockee, LockFunction locker)
{
	LockHelper<Lock, LockFunction>::lock(lockee, locker);
}

template <typename Lock, typename LockFunction>
void unlock_helper(Lock * lockee, LockFunction && unlocker)
{
	int result = unlocker(lockee);

	switch( result )
	{
	case 0:	// OK
		return;

	case EPERM:
		throw("Error detected: attempting to release an unaquired mutex");
		break;

	default:
		throw("Error encountered while attempting to release mutex lock");
	}
}

#endif // __COMMON_CONCURRENT_DETAILS_HPP__
