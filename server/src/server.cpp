#include <span>
#include <cstddef>
#include <memory>
#include <functional>

#include "syscall_handler.hpp"
#include "worker.hpp"
#include "server.hpp"
#include "session.hpp"

//socket specific
#include <sys/socket.h>
#include <netinet/in.h>


namespace hse {

void server::handle_write(worker& worker_, worker::io_result_t count_written){

    if(count_written<0)
        throw std::system_error({errno, std::system_category()}, "Negative write");

}

void server::handle_read(std::unique_ptr<single_io> io_op, worker& worker_, worker::io_result_t count_read) {

    // connection closed
    if(count_read==0)
        return;

    if(count_read<0)
        throw std::system_error({errno, std::system_category()}, "Negative read");

    int from = io_op->from, to = io_op->to;

    auto& write_buf = io_op->buf;
    std::span<std::byte> write_span {reinterpret_cast<std::byte*>(write_buf.data()),
                                     write_buf.size()};

    std::shared_ptr<single_io> write_op = std::move( io_op );
    std::shared_ptr<worker::handler_t> write_done = std::make_shared<worker::handler_t>(
        [write_op](worker& w, worker::io_result_t cw) {
            server::handle_write(w, cw);
        }
    );

    worker_.async_write_some(to,
                             write_span,
                             write_done);
    // process io_op->buf
    // we have read data somewhere
    // now we need to write data further
    //

    // and read again with new io_op (cause more data may be awailable)

    std::unique_ptr<single_io> new_read_io = std::make_unique<single_io>(from,to);
    auto& new_read_buf = new_read_io->buf;
    std::span<std::byte> new_read_span {reinterpret_cast<std::byte*>(new_read_buf.data()),
                                        new_read_buf.size()};

    std::shared_ptr<worker::handler_t> next_read = std::make_shared<worker::handler_t>(
        [&new_read_io](worker& w, worker::io_result_t cr) mutable {
            server::handle_read(std::move(new_read_io), w, cr);
        }
    );

//    std::shared_ptr<worker::handler_t> prd =std::make_shared<worker::handler_t>(next_read);

    worker_.async_read_some(from,
                            new_read_span,
                            next_read);
}


void server::handle_accept(worker& worker_, worker::io_result_t accept_fd) {
    using namespace std::placeholders;

    if (accept_fd<0)
        throw std::system_error({errno, std::system_category()}, "Negative accept");

    std::shared_ptr<sockaddr_in6> incoming_socket_adderss = std::make_shared<sockaddr_in6>();
    std::shared_ptr<socklen_t> incoming_socket_adderss_len = std::make_shared<socklen_t>(sizeof (sockaddr_in6));
    auto next_accept = std::bind(&server::handle_accept, this, _1, _2);
    worker_.async_accept(accept_fd,
                         reinterpret_cast<sockaddr *>(&incoming_socket_adderss),
                         incoming_socket_adderss_len.get(),
                         std::make_unique<worker::handler_t>(next_accept));


    // ================================================================================================ //

    std::unique_ptr<single_io> socket_to_shell = std::make_unique<single_io>(1,2);
    auto& socket_buf = socket_to_shell->buf;
    std::span<std::byte> socket_span {reinterpret_cast<std::byte*>(socket_buf.data()),
                                   socket_buf.size()};
//    auto start_socket_read = std::bind(&server::handle_read, std::move(socket_to_shell), _2, _3);
    std::shared_ptr<worker::handler_t> start_socket_read = std::make_shared<worker::handler_t>(
        [&socket_to_shell](worker& w, worker::io_result_t cr) mutable {
            server::handle_read(std::move(socket_to_shell), w, cr);
        }
    );

    worker_.async_read_some(socket_to_shell->from,
                            socket_span,
                            start_socket_read);//std::make_shared<worker::handler_t>(start_socket_read));

    // ================================================================================================ //

    std::unique_ptr<single_io> shell_to_socket = std::make_unique<single_io>(4,3);
    auto& shell_buf = shell_to_socket->buf;
    std::span<std::byte> shell_span {reinterpret_cast<std::byte*>(shell_span.data()),
                                     shell_span.size()};
    std::shared_ptr<worker::handler_t> start_shell_read = std::make_shared<worker::handler_t>( //std::bind(&server::handle_read, std::move(shell_to_socket), _2, _3);
        [&shell_to_socket](worker& w, worker::io_result_t cr) mutable {
            server::handle_read(std::move(shell_to_socket), w, cr);
        }
    );

    worker_.async_read_some(shell_to_socket->from,
                            shell_span,
                            start_shell_read);//std::make_shared<worker::handler_t>(start_shell_read));

}

server::server(std::uint32_t nworkers, std::uint32_t uring_entires, std::uint32_t uring_flags): workers(nworkers){

    accept_fd = socket(AF_INET6, SOCK_STREAM, 0);

    sockaddr_in6 ipv6_addr{};
    ipv6_addr.sin6_addr = in6addr_loopback;
    ipv6_addr.sin6_port = 8888;

    syscall_handler(bind).set_description("Bind socket call")
                         .set_error_condition([](int ret){ return ret<0;})
                         .run(accept_fd, reinterpret_cast<sockaddr*>(&ipv6_addr), sizeof (ipv6_addr));


    syscall_handler(listen).set_description("Listen socket call")
                           .set_error_condition([](int ret){ return ret<0;})
                           .run(accept_fd, 4096);

    worker_threads.reserve(workers.size());

    // associate each worker with entry job

}
void server::start(){

    // and assign each worker to thread
    for (auto& worker: workers){
        auto accept = std::bind(&worker::event_loop, &worker);
        worker_threads.push_back(std::thread{accept});
    }

    // and just wait for incoming signals
    for(;;){}

}




}
