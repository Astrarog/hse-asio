#include <unistd.h>
#include <sys/syscall.h>

#include "io_uring.hpp"


#ifdef __alpha__
/*
 * alpha is the only exception, all other architectures
 * have common numbers for new system calls.
 */
# ifndef __NR_io_uring_setup
#  define __NR_io_uring_setup		535
# endif
# ifndef __NR_io_uring_enter
#  define __NR_io_uring_enter		536
# endif
# ifndef __NR_io_uring_register
#  define __NR_io_uring_register	537
# endif
#else /* !__alpha__ */
# ifndef __NR_io_uring_setup
#  define __NR_io_uring_setup		425
# endif
# ifndef __NR_io_uring_enter
#  define __NR_io_uring_enter		426
# endif
# ifndef __NR_io_uring_register
#  define __NR_io_uring_register	427
# endif
#endif

int io_uring_setup(__u32 entries, struct io_uring_params *p){
    return syscall(__NR_io_uring_setup, entries, p);
}

int io_uring_register(unsigned int fd, unsigned int opcode,
                      void *arg, unsigned int nr_args){
    return syscall(__NR_io_uring_register, fd, opcode, arg, nr_args);
}

int io_uring_enter(unsigned int fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags,
                   sigset_t *sig){
    return syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig);
}
