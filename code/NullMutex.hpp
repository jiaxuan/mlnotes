#ifndef __COMMON_CONCURRENT_NULLMUTEX_HPP__
#define __COMMON_CONCURRENT_NULLMUTEX_HPP__

#include <common/common.hpp>

// -----------------------------------------------------------------------------------
// ml::common::concurrent
// -----------------------------------------------------------------------------------

namespace ml {
namespace common {
namespace concurrent {

class NullMutex : private virtual boost::noncopyable
{
public:

    struct scoped_read_lock {scoped_read_lock(NullMutex const &){}};
    struct scoped_write_lock {scoped_write_lock(NullMutex const &){}};
    struct scoped_lock {scoped_lock(NullMutex const &){}};

public:

    NullMutex() {};
    ~NullMutex() {};

};

} // concurrent
} // common
} // ml

#endif //__COMMON_CONCURRENT_NULLMUTEX_HPP__
