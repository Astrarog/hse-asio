#ifndef IO_URING_DRIVER_HPP
#define IO_URING_DRIVER_HPP
#include <optional>
#include <vector>
#include <span>
#include <cstddef>
#include <unordered_set>
#include <mutex>
#include <atomic>

#include "io_uring.hpp"
#include "rings.hpp"
#include "io_operation.hpp"

namespace hse {

struct io_uring_setups{
    std::uint32_t flags;
    std::uint32_t sq_thread_cpu;
    std::uint32_t sq_thread_idle;
};

class io_uring_driver
{
    using token_t = std::uint64_t;

    file_descriptor uring_fd;
    sq_ring_t sq_ring;
    cq_ring_t cq_ring;

    token_t post_last_registered = 0;
    token_t post_last_submitted = 0;
    token_t post_last_done = 0;
    std::unordered_set<token_t> pending_operations;

    std::mutex lock_registration;
    std::mutex lock_submition;
    std::mutex lock_wait;


public:
    io_uring_driver(std::uint32_t entries=4096,
                    std::optional<io_uring_setups> params = std::nullopt);

    std::uint64_t register_op(io_operation = {io_operation_type::noop, {}});

    // TO DO ??
    std::uint64_t register_op(std::vector<io_operation>);

    io_uring_driver& initiate(token_t operation_token);
    io_uring_driver& initiate_all();

    io_uring_driver& wait(token_t operation_token);
    io_uring_driver& wait_all();

};

}

#endif // IO_URING_DRIVER_HPP

