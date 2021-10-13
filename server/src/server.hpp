#ifndef SERVER_HPP
#define SERVER_HPP
#include <memory>
#include <vector>
#include <thread>
#include <cerrno>
#include <netinet/in.h>

#include "worker.hpp"
#include "file_descriptor.hpp"

namespace hse {

class server
{
    file_descriptor accept_fd;

    std::vector<worker> workers;
    std::vector<std::thread> worker_threads;

    void handle_accept(worker& worker_, worker::io_result_t);
    void static handle_read (std::shared_ptr<single_io> io_op, worker& worker_, worker::io_result_t res);
    void static handle_write(worker& worker_, worker::io_result_t res);

public:
    server(std::uint32_t nworkers, std::uint32_t uring_entires=4096, std::uint32_t uring_flags=0);
    void start();
};

}

#endif // SERVER_HPP
