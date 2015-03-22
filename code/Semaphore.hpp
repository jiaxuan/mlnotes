#ifndef SEMAPHORE_HEADER_GUARD
#define SEMAPHORE_HEADER_GUARD

#include <boost/utility.hpp>
#include <memory>

namespace ml {
namespace common {
namespace concurrent {

class Semaphore : boost::noncopyable {
    class Impl;
public:
    explicit Semaphore(unsigned value = 0);
    ~Semaphore();
    void post();
    void wait();
    bool tryWait();
    int getValue() const;
private:
    std::auto_ptr<Impl> pimpl;
};

}
}
}

#endif /*SEMAPHORE_HEADER_GUARD*/
