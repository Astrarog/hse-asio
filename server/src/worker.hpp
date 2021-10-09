#ifndef WORKER_HPP
#define WORKER_HPP
#include <cstdint>
#include <cstddef>
#include <span>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>

#include <liburing.h>

#include "receiver.hpp"


namespace hse {



class worker
{
public:
    using io_result_t = std::uint32_t;
    using token_t = std::uint64_t;

    // std::array<std::byte, 512> buffer;
    // buffer should be stored outside the worker
    //
    // TO DO array<iovec>
    // TO DO FIXED_BUFFERS

    using handler_t = std::function<void(io_result_t)>;

private:
    io_uring ring;
    token_t awailable_token=0;
    std::unordered_map<token_t, std::shared_ptr<handler_t>> token_table;
    cqe_receiver compilted_tasks_receiver{ring};



public:
    worker(std::uint32_t entires=4096, std::uint32_t flags=0);
    ~worker();


    worker& async_accept(int fd,
                         sockaddr * addr,
                         socklen_t* addr_len,
                         std::shared_ptr<handler_t> callback);

    worker& get_next_complitted();

    worker& async_read_some (int fd, std::span<std::byte> buffer, std::shared_ptr<handler_t> callback);

    worker& async_write_some(int fd, std::span<std::byte> buffer, std::shared_ptr<handler_t> callback);

};

}
#endif // WORKER_HPP
