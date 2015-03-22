#ifndef __COMMON_CONCURRENT_DETAILS_HPP__
#define __COMMON_CONCURRENT_DETAILS_HPP__

#include <common/exception/Exception.hpp>
#include <common/logging/Logger.hpp>
#include <sys/time.h>
#include <pthread.h>

namespace ml {
namespace common {
namespace concurrent {
namespace detail {

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
					LCRIT("Lock [%p] can't be acquired for %d seconds, thread might be deadlocked\n%s",
							lockee, lock_timeout * counter, mlc::diagnostics::StackTrace().toString().c_str());
				}
				else
				{
					LWARN("Lock [%p] can't be acquired for %d seconds\n%s",
							lockee, lock_timeout * counter, mlc::diagnostics::StackTrace().toString().c_str());
				}

				break;

			case EDEADLK:
				THROW(ml::common::exception::LogicError, "Deadlock detected: attempting to acquire the lock while holding read/write lock");
				break;

			default:
				THROW(ml::common::exception::RuntimeException, "Error encountered while attempting to aquire a lock", result);
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
			THROW(ml::common::exception::RuntimeException, "Error encountered while attempting to aquire a lock", result);
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
		THROW(ml::common::exception::LogicError, "Error detected: attempting to release an unaquired mutex");
		break;

	default:
		THROW(ml::common::exception::RuntimeException, "Error encountered while attempting to release mutex lock", result);
	}
}

}
}
}
}

#endif // __COMMON_CONCURRENT_DETAILS_HPP__
