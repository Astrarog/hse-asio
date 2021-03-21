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
      write = IORING_OP_WRITE,
      write_vec = IORING_OP_WRITEV,
      read_vec = IORING_OP_READV
};

struct io_operation {
    io_operation_type type;
    std::span<std::byte> data;
    file_descriptor fd;
    std::uint64_t offest = 0;

};

}
#endif // IO_OPERATION_HPP
