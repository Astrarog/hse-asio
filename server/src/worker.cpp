#include <string_view>
#include <span>

#include <boost/log/trivial.hpp>

#include <liburing.h>

#include "worker.hpp"

namespace hse {

static void debug_log_sqe(io_uring_sqe* sqe, std::string_view op_type){
    BOOST_LOG_TRIVIAL(debug) << "SQE Submitted. ";
    BOOST_LOG_TRIVIAL(debug) << "   SQE=" << sqe ;
    BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << sqe->user_data ;
    BOOST_LOG_TRIVIAL(debug) << "   Opeation Type: " << op_type;

}

worker::worker(std::uint32_t entries, std::uint32_t flags){
    io_uring_queue_init(entries, &ring, flags);
}

worker::~worker(){
    io_uring_queue_exit(&ring);
}


worker& worker::get_next_complitted(){

    io_uring_cqe& completed_op = compilted_tasks_receiver.show_next();
    token_t token = completed_op.user_data;
    io_result_t result = completed_op.res;

    auto handler = token_table[token];

    try {
        (*handler)(*this, result);
    }
    catch (...) { }

    token_table.erase(token);

    compilted_tasks_receiver.seen(completed_op);

    return *this;

}

worker& worker::async_accept(int fd,
                             sockaddr * addr,
                             socklen_t* addr_len,
                             std::shared_ptr<handler_t> callback){
    struct io_uring_sqe* sqe;

    sqe = io_uring_get_sqe(&ring);

    io_uring_prep_accept(sqe, fd, addr, addr_len, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(awailable_token));

    token_table[awailable_token++]=std::move(callback);

    io_uring_submit(&ring);

    debug_log_sqe(sqe, "accept");

    return *this;

}

worker& worker::async_read_some (int fd, std::span<std::byte> buffer, std::shared_ptr<handler_t> callback){

    struct io_uring_sqe* sqe;
    sqe = io_uring_get_sqe(&ring);

    void * addr = reinterpret_cast<void *>(buffer.data());
    std::size_t nbytes = buffer.size();
    void * user_data = reinterpret_cast<void *>(awailable_token);

    io_uring_prep_read(sqe, fd, addr, nbytes, 0);
    io_uring_sqe_set_data(sqe, user_data);

    token_table[awailable_token++]=callback;

    io_uring_submit(&ring);

    debug_log_sqe(sqe, "read");
    BOOST_LOG_TRIVIAL(debug) << "   FD=" << fd ;

    return *this;

}

worker& worker::async_write_some(int fd, std::span<std::byte> buffer, std::shared_ptr<handler_t> callback){

    struct io_uring_sqe* sqe;
    sqe = io_uring_get_sqe(&ring);

    void * addr = reinterpret_cast<void *>(buffer.data());
    std::size_t nbytes = buffer.size();
    void * user_data = reinterpret_cast<void *>(awailable_token);

    io_uring_prep_write(sqe, fd, addr, nbytes, 0);
    io_uring_sqe_set_data(sqe, user_data);

    token_table[awailable_token++]=callback;

    io_uring_submit(&ring);

    debug_log_sqe(sqe, "write");
    BOOST_LOG_TRIVIAL(debug) << "   FD=" << fd ;

    return *this;

}

worker& worker::async_read_some (int fd,
                                 std::shared_ptr<handler_t> callback){

    struct io_uring_sqe* sqe;
    sqe = io_uring_get_sqe(&ring);

    void * addr = reinterpret_cast<void *>(io_buffer.data());
    std::size_t nbytes = io_buffer.size();
    void * user_data = reinterpret_cast<void *>(awailable_token);

    io_uring_prep_read(sqe, fd, addr, nbytes, 0);
    io_uring_sqe_set_data(sqe, user_data);

    token_table[awailable_token++]=callback;

    io_uring_submit(&ring);

    debug_log_sqe(sqe, "read");
    BOOST_LOG_TRIVIAL(debug) << "   FD=" << fd ;

    return *this;

}

worker& worker::async_write_some(int fd,
                                 std::shared_ptr<handler_t> callback){
    struct io_uring_sqe* sqe;

    sqe = io_uring_get_sqe(&ring);

    void * addr = reinterpret_cast<void *>(io_buffer.data());
    std::size_t nbytes = io_buffer.size();
    void * user_data = reinterpret_cast<void *>(awailable_token);

    io_uring_prep_write(sqe, fd, addr, nbytes, 0);
    io_uring_sqe_set_data(sqe, user_data);

    token_table[awailable_token++]=callback;

    io_uring_submit(&ring);

    debug_log_sqe(sqe, "write");
    BOOST_LOG_TRIVIAL(debug) << "   FD=" << fd ;

    return *this;
}

}
