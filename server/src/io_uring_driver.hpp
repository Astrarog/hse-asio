#ifndef IO_URING_DRIVER_HPP
#define IO_URING_DRIVER_HPP
#include <optional>
#include <vector>

#include "rings.hpp"

namespace hse {

enum class io_operation {
      read,
      write
};

struct io_uring_setups{
    std::uint32_t flags;
    std::uint32_t sq_thread_cpu;
    std::uint32_t sq_thread_idle;
};

class io_uring_driver
{
    int uring_fd;
    sq_ring_t sq_ring;
    cq_ring_t cq_ring;
    std::uint64_t token = 0;
public:
    io_uring_driver(std::uint32_t entries=4096,
                    std::optional<io_uring_setups> params = std::nullopt);
    ~io_uring_driver();


    std::uint64_t register_operation(io_operation);

    std::uint64_t register_operation(std::vector<io_operation>);

    io_uring_driver& wait_for_token();

    io_uring_driver& wait_for_all();

};

}

#endif // IO_URING_DRIVER_HPP
