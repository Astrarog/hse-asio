#ifndef IO_URING_HPP
#define IO_URING_HPP
#include <linux/io_uring.h>
#include <signal.h>

int io_uring_setup(__u32 entries, struct io_uring_params *p);

int io_uring_register(unsigned int fd, unsigned int opcode,
                      void *arg, unsigned int nr_args);

int io_uring_enter(unsigned int fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags,
                   sigset_t *sig);

#endif // IO_URING_HPP
