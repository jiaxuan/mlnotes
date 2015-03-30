#ifndef _ML_COMMON_CONCURRENT_GETTID_HPP_
#define _ML_COMMON_CONCURRENT_GETTID_HPP_

#include <sys/types.h>
#include <linux/unistd.h>
#include <unistd.h>


inline pid_t gettid(void)
{
#ifdef __NR_gettid
    return syscall(__NR_gettid);
#else
    return -ENOSYS;
#endif
}

#endif
