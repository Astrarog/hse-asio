#ifndef WORKER_HPP
#define WORKER_HPP
#include <cstdint>
#include <cstddef>
#include <span>
#include <functional>
#include <vector>
#include <unordered_map>

#include <liburing.h>

#include "receiver.hpp"


namespace hse {



class worker
{
    using io_result_t = std::uint32_t;
    using token_t = std::uint64_t;

    // std::array<std::byte, 512> buffer;
    // buffer should be stored outside the worker
    //
    // TO DO array<iovec>
    // TO DO FIXED_BUFFERS

    using handler_t = std::function<token_t(io_result_t)>;

    io_uring ring;
    token_t awailable_token=0;
    std::unordered_map<token_t, handler_t> token_table;
    cqe_receiver compilted_tasks_receiver{ring};



public:
    worker(std::uint32_t entires=4096, std::uint32_t flags=0);
    ~worker();


    worker& async_accept(handler_t&& callback,
                         int fd,
                         sockaddr * addr,
                         socklen_t* addr_len);

    worker& next_task();

//    worker& async_read_some(std::span<std::byte> buffer, handler_t&& callback);

};

}
#endif // WORKER_HPP
