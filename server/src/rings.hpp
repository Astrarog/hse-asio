#ifndef RINGS_HPP
#define RINGS_HPP

#include <cstdint>

#include "io_uring.hpp"

namespace hse {

struct ring_t {

    unsigned* head;
    unsigned* tail;
    unsigned* ring_mask;
    unsigned* ring_entries;

};

struct sq_ring_t: public ring_t {

    unsigned* flags;
    unsigned* dropped;
    unsigned* array;

    std::size_t size;
    io_uring_sqe* entries;

};


struct cq_ring_t: public ring_t {

    unsigned* overflow;

    std::size_t size;
    io_uring_cqe* entries;

};

}
#endif // RINGS_HPP
