#ifndef RINGS_HPP
#define RINGS_HPP

#include <cstdint>

#include "io_uring.hpp"


struct sq_ring_t{
    io_sqring_offsets info;
    std::size_t size;
    std::uintptr_t ptr;
    io_uring_sqe* entries;

};


struct cq_ring_t{
    io_cqring_offsets info;
    std::size_t size;
    std::uintptr_t ptr;
    io_uring_cqe* entries;
};


#endif // RINGS_HPP
