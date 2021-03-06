#ifndef SERVER_HPP
#define SERVER_HPP
#include <memory>
#include <vector>
#include <thread>
#include <cerrno>
#include <thread>

#include <netinet/in.h>

#include "worker.hpp"
#include "file_descriptor.hpp"

namespace hse {

class server
{
    file_descriptor accept_fd;

    std::vector<worker> workers;
    std::vector<std::thread> worker_threads;

    std::string shell;

    void handle_accept(worker& worker_, worker::io_result_t);
    void static handle_read (std::shared_ptr<single_io> io_op, worker& worker_, worker::io_result_t res);
    void static handle_write(worker& worker_, worker::io_result_t res);

public:
    server(std::string shell_ = "/bin/bash",
           std::uint32_t nworkers=std::thread::hardware_concurrency(),
           std::uint32_t uring_entires=4096,
           std::uint32_t uring_flags=0,
           in_addr ip_address = static_cast<in_addr>(INADDR_LOOPBACK),
           in_port_t port = 8888);
    void start();
};

}

#endif // SERVER_HPP
