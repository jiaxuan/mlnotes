#include "Semaphore.hpp"
#include <common/exception/Exception.hpp>
#include <semaphore.h>
#include <errno.h>

using ml::common::exception::SystemException;

namespace ml {
namespace common {
namespace concurrent {

class Semaphore::Impl : boost::noncopyable {
public:
    Impl(unsigned value) {
        if(-1 == ::sem_init(&m_sem, 0, value)) {
            const int code = errno;
            FTHROWC(SystemException, code, "Couldn't initialize semaphore");
        }
    }
    ~Impl() {
        ::sem_destroy(&m_sem);
    }
    void post() {
        if(-1 == ::sem_post(&m_sem)) {
            const int code = errno;
            FTHROWC(SystemException, code, "Couldn't increment semaphore");
        }
    }
    void wait() {
        for(;-1 == ::sem_wait(&m_sem);) {
            const int code = errno;
            if(EINTR == code)
                continue;
            FTHROWC(SystemException, code, "Couldn't decrement semaphore");
        }
    }
    bool tryWait() {
        for(;-1 == ::sem_trywait(&m_sem);) {
            const int code = errno;
            if(code == EAGAIN)
                return false;
            if(code == EINTR)
                continue;
            FTHROWC(SystemException, code, "Couldn't decrement semaphore");
        }
        return true;
    }
    int getValue() const {
        int result = 0;
        if(-1 == ::sem_getvalue(const_cast<sem_t*>(&m_sem), &result)) {
            const int code = errno;
            FTHROWC(SystemException, code, "Couldn't get semaphore's value");
        }
        return result;
    }
private:
    sem_t m_sem;
};

Semaphore::Semaphore(unsigned value): pimpl(new Impl(value)) {}

Semaphore::~Semaphore() {}

void Semaphore::post() {
    pimpl->post();
}

void Semaphore::wait() {
    pimpl->wait();
}

bool Semaphore::tryWait() {
    return pimpl->tryWait();
}

int Semaphore::getValue() const {
    return pimpl->getValue();
}

}
}
}
