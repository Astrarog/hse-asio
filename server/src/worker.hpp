#ifndef WORKER_HPP
#define WORKER_HPP
#include <cstdint>
#include <cstddef>
#include <span>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <vector>
#include <iostream>

#include <liburing.h>

#include "receiver.hpp"
#include "file_descriptor.hpp"

namespace hse {


struct single_io_end
{
    int from;
    int to;
    std::vector<char> buf;
    single_io_end(int from_, int to_, std::vector<char> buf_): from(from_), to(to_), buf(std::move(buf_)) {}
};

struct single_io
{
    std::shared_ptr<file_descriptor> from;
    std::shared_ptr<file_descriptor> to;
    std::vector<char> buf;
    single_io(int from_, int to_): from(std::make_shared<file_descriptor>(from_)),
                                                           to(std::make_shared<file_descriptor>(to_)),
                                                           buf(64) {}
};


class worker
{
public:
    using io_result_t = std::int32_t;
    using token_t = std::uint64_t;
    using handler_t = std::function<void(worker&, io_result_t)>;

private:
    io_uring ring;
    token_t awailable_token=0;
    std::unordered_map<token_t, std::shared_ptr<handler_t>> token_table;
    cqe_receiver compilted_tasks_receiver{ring};

public:
    // std::array<std::byte, 512> buffer;
    // buffer should be stored outside the worker
    //
    // TO DO array<iovec>
    // TO DO FIXED_BUFFERS

    std::array<std::byte, 64> io_buffer;

    worker(std::uint32_t entires=4096, std::uint32_t flags=0);
    ~worker();


    worker& async_accept(int fd,
                         sockaddr * addr,
                         socklen_t* addr_len,
                         std::shared_ptr<handler_t> callback);

    worker& get_next_complitted();


    worker& async_read_some (int fd, std::shared_ptr<handler_t> callback);

    worker& async_write_some(int fd, std::shared_ptr<handler_t> callback);

    worker& async_read_some (int fd, std::span<std::byte> buffer, std::shared_ptr<handler_t> callback);

    worker& async_write_some(int fd, std::span<std::byte> buffer, std::shared_ptr<handler_t> callback);

    worker& event_loop() {
        while (1){
            this->get_next_complitted();
        }
    };

};

}
#endif // WORKER_HPP
