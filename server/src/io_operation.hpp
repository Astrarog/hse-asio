#ifndef IO_OPERATION_HPP
#define IO_OPERATION_HPP
#include <cstdint>
#include <cstddef>
#include <span>

#include "file_descriptor.hpp"
#include "io_uring.hpp"

namespace hse {

enum class io_operation_type {
      noop = IORING_OP_NOP,
      read = IORING_OP_READ,
      write = IORING_OP_WRITE
};

struct io_operation {

private:
    inline constexpr static std::uint64_t unused = 0;

public:
    io_operation_type type;
    std::span<std::byte> data;
    file_descriptor fd{unused};
    std::uint64_t offest = unused;


};

}
#endif // IO_OPERATION_HPP
