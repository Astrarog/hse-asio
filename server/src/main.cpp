#include <string>
#include <span>
#include <cstddef>
#include <string_view>
#include <iostream>
#include <functional>
#include <exception>
#include <memory>

#include "file_descriptor.hpp"
#include "worker.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//socket specific
#include <sys/socket.h>
#include <netinet/in.h>


#include <liburing.h>


using namespace hse;


int initialize_socket(){
    int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);


    sockaddr_in6 ipv6_addr{};
    ipv6_addr.sin6_addr = in6addr_loopback;
    ipv6_addr.sin6_port = 8888;
    bind(sock_fd, reinterpret_cast<sockaddr*>(&ipv6_addr), sizeof (ipv6_addr));

    listen(sock_fd, 4096);

    return sock_fd;
}


int main(){

    hse::file_descriptor sock_fd = initialize_socket();

    hse::worker sait_worker{};

//    char write_buf[64];
    char* read_buf = new char[32 * 1024];

    auto incoming_sock = std::make_shared<sockaddr_in6>();
    auto sock_len = std::make_shared<socklen_t>(sizeof (sockaddr_in6));

    using handler_t = hse::worker::handler_t;

    std::shared_ptr<handler_t> session_read;


    std::shared_ptr<handler_t> accept_handler;
    accept_handler = std::make_shared<handler_t>([&](hse::worker&, hse::worker::io_result_t res){
        if (res<0)
            throw std::system_error({errno, std::system_category()}, "Negative accept");
        sait_worker.async_accept(sock_fd, reinterpret_cast<sockaddr*>(incoming_sock.get()), sock_len.get(), accept_handler);

        int session_fd = res;
        session_read = std::make_shared<handler_t>([&, session_fd](hse::worker&, hse::worker::io_result_t res){
            if (res<0)
                throw std::system_error({errno, std::system_category()}, "Negative read");
            if (res==0)
                return ; // session end
            std::cout << std::string_view(read_buf, res);
            sait_worker.async_read_some(session_fd, std::span{reinterpret_cast<std::byte*>(read_buf), 64}, session_read);
        });
        sait_worker.async_read_some(session_fd, std::span{reinterpret_cast<std::byte*>(read_buf), 64}, session_read);

    });

    sait_worker.async_accept(sock_fd, reinterpret_cast<sockaddr*>(incoming_sock.get()), sock_len.get(), accept_handler);

    sait_worker.event_loop();

}
