#include <span>
#include <cstddef>
#include <memory>
#include <functional>

#include "syscall_handler.hpp"
#include "worker.hpp"
#include "server.hpp"

//socket specific
#include <sys/socket.h>
#include <netinet/in.h>


namespace hse {

void server::handle_write(worker& worker_, worker::io_result_t count_written){

    if(count_written<0)
        throw std::system_error({errno, std::system_category()}, "Negative write");

}

void server::handle_read(std::shared_ptr<single_io> io_op, worker& worker_, worker::io_result_t count_read) {

    // connection closed
    if(count_read==0)
        return;

    if(count_read<0)
        throw std::system_error({errno, std::system_category()}, "Negative read");


    // we have read data somewhere
    // now we need to write data further

    int from = *(io_op->from), to = *(io_op->to);

    std::shared_ptr<single_io_end> write_op = std::make_shared<single_io_end>(from, to, std::move(io_op->buf));
    auto& write_buf = write_op->buf;
    write_buf.resize(count_read);

    std::span<std::byte> write_span {reinterpret_cast<std::byte*>(write_buf.data()),
                                     write_buf.size()};


    std::shared_ptr<worker::handler_t> write_done = std::make_shared<worker::handler_t>(
        [write_op](worker& w, worker::io_result_t cw) {
            server::handle_write(w, cw);
        }
    );

    worker_.async_write_some(to,
                             write_span,
                             write_done);


    // and read again with new io_op (cause more data may be awailable)

    std::shared_ptr<single_io> new_read_io = io_op;
    auto& new_read_buf = new_read_io->buf;
    new_read_buf.resize(64);
    std::span<std::byte> new_read_span {reinterpret_cast<std::byte*>(new_read_buf.data()),
                                        new_read_buf.size()};

    std::shared_ptr<worker::handler_t> next_read = std::make_shared<worker::handler_t>(
        [new_read_io](worker& w, worker::io_result_t cr) mutable {
            server::handle_read(new_read_io, w, cr);
        }
    );

    worker_.async_read_some(from,
                            new_read_span,
                            next_read);
}


void server::handle_accept(worker& worker_, worker::io_result_t socket_fd) {
    using namespace std::placeholders;

    if (socket_fd<0)
        throw std::system_error({errno, std::system_category()}, "Negative accept");

    std::shared_ptr<sockaddr_in6> incoming_socket_adderss = std::make_shared<sockaddr_in6>();
    std::shared_ptr<socklen_t> incoming_socket_adderss_len = std::make_shared<socklen_t>(sizeof (sockaddr_in6));
    auto next_accept = std::bind(&server::handle_accept, this, _1, _2);
    worker_.async_accept(accept_fd,
                         reinterpret_cast<sockaddr *>(&incoming_socket_adderss),
                         incoming_socket_adderss_len.get(),
                         std::make_shared<worker::handler_t>(next_accept));


    // TO DO:
    int from_pipe = 0, to_pipe = 1;

    // ================================================================================================ //

    std::shared_ptr<single_io> socket_to_shell = std::make_shared<single_io>(socket_fd, to_pipe);
    auto& socket_buf = socket_to_shell->buf;
    std::span<std::byte> socket_span {reinterpret_cast<std::byte*>(socket_buf.data()),
                                   socket_buf.size()};

    std::shared_ptr<worker::handler_t> start_socket_read = std::make_shared<worker::handler_t>(
        [socket_to_shell](worker& w, worker::io_result_t cr) mutable {
            server::handle_read(socket_to_shell, w, cr);
        }
    );

    worker_.async_read_some(socket_fd,
                            socket_span,
                            start_socket_read);

    // ================================================================================================ //

    std::shared_ptr<single_io> shell_to_socket = std::make_shared<single_io>(from_pipe, socket_fd);
    auto& shell_buf = shell_to_socket->buf;
    std::span<std::byte> shell_span {reinterpret_cast<std::byte*>(shell_buf.data()),
                                     shell_buf.size()};
    std::shared_ptr<worker::handler_t> start_shell_read = std::make_shared<worker::handler_t>(
        [shell_to_socket](worker& w, worker::io_result_t cr) mutable {
            server::handle_read(shell_to_socket, w, cr);
        }
    );

    worker_.async_read_some(from_pipe,
                            shell_span,
                            start_shell_read);

}

server::server(std::uint32_t nworkers, std::uint32_t uring_entires, std::uint32_t uring_flags): workers(nworkers){

    accept_fd = socket(AF_INET6, SOCK_STREAM, 0);

    if(accept_fd < 0)
        throw std::system_error({errno, std::system_category()}, "Negative socket call");
    sockaddr_in6 ipv6_addr{};
    ipv6_addr.sin6_addr = in6addr_loopback;
    ipv6_addr.sin6_port = 8888;

    syscall_handler(bind).set_description("Bind socket call")
//                         .set_error_condition([](int ret){ return ret<0;})
                         .run(accept_fd, reinterpret_cast<sockaddr*>(&ipv6_addr), sizeof (ipv6_addr));


    syscall_handler(listen).set_description("Listen socket call")
                           .set_error_condition([](int ret){ return ret<0;})
                           .run(accept_fd, 4096);

    worker_threads.reserve(workers.size());

    using namespace std::placeholders;
    for (auto& worker: workers){
        std::shared_ptr<sockaddr_in6> incoming_socket_adderss = std::make_shared<sockaddr_in6>();
        std::shared_ptr<socklen_t> incoming_socket_adderss_len = std::make_shared<socklen_t>(sizeof (sockaddr_in6));
        auto initial_accept = std::bind(&server::handle_accept, this, _1, _2);
        worker.async_accept(accept_fd,
                            reinterpret_cast<sockaddr *>(&incoming_socket_adderss),
                            incoming_socket_adderss_len.get(),
                            std::make_shared<worker::handler_t>(initial_accept));
    }

}
void server::start(){

    if(workers.size()==1)
        workers[0].event_loop();
    // and assign each worker to thread
    for (auto& worker: workers){
        auto start_loop = std::bind(&worker::event_loop, &worker);
        worker_threads.push_back(std::thread{start_loop});
    }

    // and just wait for incoming signals
    for(;;){}

}




}
