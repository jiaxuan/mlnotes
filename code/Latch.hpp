#ifndef __COMMON_CONCURRENT_LATCH_HPP_
#define __COMMON_CONCURRENT_LATCH_HPP_

#include <common/common.hpp>
#include <common/concurrent/atomic32.hpp>
#include <common/concurrent/Condition.hpp>

namespace ml { namespace common { namespace concurrent {
	// ml::common::concurrent

class Latch
	: private boost::noncopyable
{
	public:
		enum Timeout { WAIT_FOREVER = -1, NO_WAIT = 0, ONE_SECOND = 1000 };
		
		Latch();
		~Latch();
		
		bool attempt(int32 = WAIT_FOREVER);
		
		void open();
		void close();
		
		void reset();
		
	private:
        enum LatchState{ eOPEN_LATCH = true, eCLOSE_LATCH = false };

		Mutex          m_mutex;
		Condition      m_cond;
		atomic32       m_unlatched;
};

} } } // end namespace

#endif // __COMMON_CONCURRENT_LATCH_HPP_
