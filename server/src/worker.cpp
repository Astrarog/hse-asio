#include <string_view>

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

worker& worker::async_accept(handler_t&& callback,
                             int fd,
                             sockaddr * addr,
                             socklen_t* addr_len){
    struct io_uring_sqe* sqe;

    sqe = io_uring_get_sqe(&ring);

    io_uring_prep_accept(sqe, fd, addr, addr_len, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(awailable_token));

    token_table[awailable_token]=std::move(callback);

    io_uring_submit(ring);

    debug_log_sqe(sqe, "accept");

    return *this;

}

worker& worker::next_task(){

    io_uring_cqe& completed_op = compilted_tasks_receiver.show_next();
    token_t token = completed_op.user_data;
    io_result_t result = completed_op.res;
    handler_t&& handler = token_table.extract(token).value();

    handler(result);

    return *this;

}


}
