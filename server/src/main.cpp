#include <string>
#include <span>
#include <cstddef>
#include <string_view>
#include <iostream>

#include "file_descriptor.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//socket specific
#include <sys/socket.h>
#include <netinet/in.h>


#include <liburing.h>

#include <boost/log/trivial.hpp>

std::uint64_t token = 1;

std::uint64_t uring_write(io_uring* ring,
                 int fd,
                 const char data[],
                 std::size_t data_size)
{
    struct io_uring_sqe* sqe;

    sqe = io_uring_get_sqe(ring);

    io_uring_prep_write(sqe, fd, data, data_size, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(token));

    io_uring_submit(ring);

    BOOST_LOG_TRIVIAL(debug) << "SQE Submitted. ";
    BOOST_LOG_TRIVIAL(debug) << "   SQE=" << sqe ;
    BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << sqe->user_data ;
    BOOST_LOG_TRIVIAL(debug) << "   Opeation Type:  write";
    BOOST_LOG_TRIVIAL(debug) << "   text=\"" << std::string(data, data_size) << '"';

    return token++;

}

std::uint64_t uring_read(io_uring* ring,
                 int fd,
                 char data[],
                 std::size_t data_size)
{
    struct io_uring_sqe* sqe;

    sqe = io_uring_get_sqe(ring);

    io_uring_prep_read(sqe, fd, data, data_size, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(token));

    io_uring_submit(ring);

    BOOST_LOG_TRIVIAL(debug) << "SQE Submitted. ";
    BOOST_LOG_TRIVIAL(debug) << "   SQE=" << sqe ;
    BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << sqe->user_data ;
    BOOST_LOG_TRIVIAL(debug) << "   Opeation Type:  read";

    return token++;

}


std::uint64_t uring_accept(io_uring* ring,
                 int fd,
                 sockaddr *addr,
                 socklen_t* addr_len)
{

    struct io_uring_sqe* sqe;

    sqe = io_uring_get_sqe(ring);

    io_uring_prep_accept(sqe, fd, addr, addr_len, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(token));

    io_uring_submit(ring);

    BOOST_LOG_TRIVIAL(debug) << "SQE Submitted. ";
    BOOST_LOG_TRIVIAL(debug) << "   SQE=" << sqe ;
    BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << sqe->user_data ;
    BOOST_LOG_TRIVIAL(debug) << "   Opeation Type:  accept";

    return token++;

}

io_uring_cqe& next_cqe(io_uring* ring)
{
    struct io_uring_cqe* cqe;

    io_uring_wait_cqe(ring, &cqe);

    BOOST_LOG_TRIVIAL(debug) << "CQE Arrived. ";
    BOOST_LOG_TRIVIAL(debug) << "   CQE=" << cqe ;
    BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << cqe->user_data ;
    BOOST_LOG_TRIVIAL(debug) << "   FLAGS=" << (cqe->flags);
    BOOST_LOG_TRIVIAL(debug) << "   Opeation Result=" << cqe->res;

    return *cqe;

}


int initialize_socket(){
    int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);


    sockaddr_in6 ipv6_addr{};
    ipv6_addr.sin6_addr = in6addr_any;
    ipv6_addr.sin6_port = 8888;
    bind(sock_fd, reinterpret_cast<sockaddr*>(&ipv6_addr), sizeof (ipv6_addr));

    listen(sock_fd, 4096);

    BOOST_LOG_TRIVIAL(debug) << "LISTENING FOR INCOMING COONNECTIONS";

    return sock_fd;
}

int main(){

    hse::file_descriptor sock_fd = initialize_socket();

    struct io_uring ring{};
    io_uring_queue_init(4096, &ring, 0);


    sockaddr* incoming_sock;
    socklen_t sock_len = sizeof (sockaddr_in6);

    std::uint64_t accept_token = uring_accept(&ring, sock_fd, incoming_sock, &sock_len);


    char write_buf[64];
    char read_buf[64];
    std::uint64_t read_token;
    std::uint64_t write_token;
    int accepted_socket_fd = -1;
    do{

        struct io_uring_cqe cqe = next_cqe(&ring);

        std::uint64_t completed_token = cqe.user_data;


        if (completed_token==accept_token){
            accepted_socket_fd = cqe.res;
        }

        if (completed_token == read_token ){
            // if res == 0 it means that we can close the socket
            auto res = cqe.res;
            auto* local= malloc(64);
            std::memcpy(local, read_buf, 64);
            std::cout << " [MGS] " <<  std::string(reinterpret_cast<char*>(local), res) << std::endl;
            free(local);
        }

        if (accepted_socket_fd != -1 ){
            read_token = uring_read(&ring, accepted_socket_fd, read_buf, 64);
            BOOST_LOG_TRIVIAL(debug) << "read_token=" << read_token;
        }
        io_uring_cqe_seen(&ring, &cqe);





    }while(1);


    io_uring_queue_exit(&ring);
}
