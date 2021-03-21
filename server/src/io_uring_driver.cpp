#include <string>
#include <system_error>
#include <span>
#include <mutex>
#include <limits>

#include <sys/mman.h>
#include <unistd.h>

#include <boost/log/trivial.hpp>

#include "syscall_handler.hpp"
#include "io_uring_driver.hpp"
#include "io_uring.hpp"
#include "io_operation.hpp"

namespace hse {


void debug_check_features(std::uint32_t feat){
    BOOST_LOG_TRIVIAL(debug) << "io_uring features:";

    // only IORING_FEAT_SINGLE_MMAP is supported in 5.12 kernel
    if(IORING_FEAT_SINGLE_MMAP & feat)
        BOOST_LOG_TRIVIAL(debug) << "    IORING_FEAT_SINGLE_MMAP is enabled";
}


io_uring_driver::io_uring_driver(std::uint32_t entries,
                                 std::optional<io_uring_setups> params){
    io_uring_params params_{};
    if(params!=std::nullopt) {
        params_.flags = params->flags;
        params_.sq_thread_cpu = params->sq_thread_cpu;
        params_.sq_thread_idle = params->sq_thread_idle;
    }

    uring_fd = syscall_handler(io_uring_setup)
                    .set_error_return_value(-1)
                    .set_description("io_uring_setup")
                    .run(entries, &params_);

    BOOST_LOG_TRIVIAL(debug) << "io_uring setup params:";
    BOOST_LOG_TRIVIAL(debug) << "    sq_entries = " <<  params_.sq_entries;
    BOOST_LOG_TRIVIAL(debug) << "    cq_entries = " <<  params_.cq_entries;

    debug_check_features(params_.features);

    // At this point kernel has written the info about rings
    std::size_t sq_ring_size = params_.sq_off.array + params_.sq_entries * sizeof (unsigned);
    std::size_t cq_ring_size = params_.cq_off.cqes  + params_.cq_entries * sizeof (io_uring_cqe);


    std::uintptr_t sq_ring_ptr =
        reinterpret_cast<std::uintptr_t>(
            syscall_handler(mmap)
            .set_error_return_value(MAP_FAILED)
            .set_description("sq_ring mmap")
            .run(nullptr, sq_ring_size, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, uring_fd, IORING_OFF_SQ_RING)
        );

    std::uintptr_t cq_ring_ptr;


    // check if there is an extra need to mmap CQring
    if (IORING_FEAT_SINGLE_MMAP & params_.features){
        // no need to do extra mmap
        cq_ring_ptr = sq_ring_ptr;
    }
    else {
        // no feature available => another mmap for CQring
        cq_ring_ptr =
            reinterpret_cast<std::uintptr_t>(
                syscall_handler(mmap)
                .set_error_return_value(MAP_FAILED)
                .set_description("io_uring_setup")
                .run(nullptr, cq_ring_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE, uring_fd, IORING_OFF_CQ_RING)
            );
    }

    std::uintptr_t sqes_size = params_.sq_entries * sizeof(struct io_uring_sqe);

    std::uintptr_t sqes_ptr =
        reinterpret_cast<std::uintptr_t>(
            syscall_handler(mmap)
            .set_error_return_value(MAP_FAILED)
            .set_description("sqes mmap")
            .run(nullptr, sqes_size, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, uring_fd, IORING_OFF_SQES)
        );

    sq_ring.head         = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.head        );
    sq_ring.tail         = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.tail        );
    sq_ring.ring_mask    = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.ring_mask   );
    sq_ring.ring_entries = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.ring_entries);
    sq_ring.flags        = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.flags       );
    sq_ring.dropped      = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.dropped     );
    sq_ring.array        = reinterpret_cast<unsigned*>(sq_ring_ptr + params_.sq_off.array       );


    cq_ring.head         = reinterpret_cast<unsigned*>(cq_ring_ptr + params_.cq_off.head        );
    cq_ring.tail         = reinterpret_cast<unsigned*>(cq_ring_ptr + params_.cq_off.tail        );
    cq_ring.ring_mask    = reinterpret_cast<unsigned*>(cq_ring_ptr + params_.cq_off.ring_mask   );
    cq_ring.ring_entries = reinterpret_cast<unsigned*>(cq_ring_ptr + params_.cq_off.ring_entries);
    cq_ring.overflow     = reinterpret_cast<unsigned*>(cq_ring_ptr + params_.cq_off.overflow    );


    sq_ring.size = sq_ring_size;
    cq_ring.size = cq_ring_size;

    sq_ring.entries = reinterpret_cast<io_uring_sqe*>(sqes_ptr);
    cq_ring.entries = reinterpret_cast<io_uring_cqe*>(cq_ring_ptr + params_.cq_off.cqes);

}

std::uint64_t io_uring_driver::register_op(io_operation op) {
    // TO DO : OVERFLOW CHECK
    unsigned tail;
    token_t token;
    {
        std::lock_guard guard{lock_registration};
        tail = *(sq_ring.tail);
        ++(*(sq_ring.tail));
        token = (post_last_registered)++;
    }
    // at this point we know the position of empty SQE, so we can hold it
    // and we have registered out operation, so we can fill operation data

    unsigned index = tail & *(sq_ring.ring_mask);
    io_uring_sqe& sqe = sq_ring.entries[index];

    // needed because of io_uring implementation
    sq_ring.array[index] = index;


    sqe.opcode = static_cast<int>(op.type);
    sqe.fd = op.fd;
    sqe.off = op.offest;
    sqe.addr = reinterpret_cast<std::uintptr_t>(op.data.data());
    sqe.len = op.data.size();

    sqe.flags = 0;
    sqe.ioprio = 0;
    sqe.rw_flags = 0;
    sqe.__pad2[0] = sqe.__pad2[1] = sqe.__pad2[2] = 0;


    sqe.user_data = token;
    // opeartion have prepared, so we cat return the operation token
    // which can be used to initiate and wait for operation completion

    BOOST_LOG_TRIVIAL(debug) << "registered new operation with token=" << token << " at adderss " << &sqe;
    return token;

}

io_uring_driver& io_uring_driver::initiate(token_t op){

    // check whether any pending operation exists
    if(post_last_submitted == post_last_registered)
        return *this;


    // no need to check whether post_last_registered == post_last_submitted in overflow
    // cause ring buffer only 4096 entries long
    std::lock_guard guard{lock_submition};

    bool was_token_overflow = (post_last_registered < post_last_submitted);

    bool was_registered = (op >= post_last_submitted) && ((op < post_last_registered) || was_token_overflow);

    // check if operation refers to pending (and not submited) operation
    if (was_registered){
        // operation was registered by someone and he wants to initiate it


        // calculating how many opearions should be submitted to kernel through io_uring
        std::size_t to_submit  = [&]{
            if (!was_token_overflow)
                return op - post_last_submitted + 1;
            else{
                std::size_t to_submit_top    = (std::numeric_limits<token_t>::max() - post_last_submitted) +1;
                std::size_t to_submit_bottom = op +1;
                return to_submit_top + to_submit_bottom;
            }
        }();
        BOOST_LOG_TRIVIAL(debug) << "Correct initiation operation request with token=" << op;
        while(post_last_submitted!=op+1){
            pending_operations.insert(post_last_submitted++);
            BOOST_LOG_TRIVIAL(debug) << "   initiated op with token=" << post_last_submitted-1;
        }

        BOOST_LOG_TRIVIAL(debug) << "    there is " << to_submit << " operations to be submitted by apllication";
        for (std::size_t total_consumed = 0; total_consumed<to_submit; ) {
            int consumed = syscall_handler(io_uring_enter)
                    .set_description("initiation of requests via io_uring_enter")
                    .set_error_return_value(-1)
                    (uring_fd, to_submit - total_consumed, 0, 0, nullptr);
            BOOST_LOG_TRIVIAL(debug) << "    the kernel consumed " << consumed  << " operations";
            total_consumed+=consumed;
        }
    }

    // may be
    // else throw exseption
    return *this;
}

io_uring_driver& io_uring_driver::initiate_all(){
    return this->initiate(post_last_registered - 1);
}

io_uring_driver& io_uring_driver::wait(token_t op) {

    if(pending_operations.count(op)==0)
        return *this; //already done or not initialized or junk operation

    std::lock_guard guard{lock_wait};

    // TO FIX
    //bool was_token_overflow = (post_last_registered < post_last_submitted);
    //bool was_initiated = (post_last_submitted <= op) || (was_token_overflow && (op < post_last_registered));

    //if(!was_initiated)
    //    return *this; //may be throw exseption
    // TO FIX


    BOOST_LOG_TRIVIAL(debug) << "Correct wait request of operation with token=" << op;
    unsigned& head = *(cq_ring.head);
    unsigned& tail = *(cq_ring.tail);
    token_t observed_token;
    do  {
        unsigned index = head & *(cq_ring.ring_mask);
        io_uring_cqe& cqe = cq_ring.entries[index];
        observed_token = cqe.user_data;
        if(head!=tail){
            pending_operations.erase(observed_token);
            BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << observed_token << " done";
            BOOST_LOG_TRIVIAL(debug) << "   haed=" << head << " tail=" << tail;
            BOOST_LOG_TRIVIAL(debug) << "   Opeation Result=" << cqe.res;
            BOOST_LOG_TRIVIAL(debug) << "   errno?=EBADF " << (errno==EBADF);
            if(cqe.res < 0)
                throw std::system_error(errno, std::system_category(), "negative result operation");
            ++head;
        }
    } while (observed_token != op);
    // no need to check head!=tail cause it is guarantee that operation happens not later

    BOOST_LOG_TRIVIAL(debug) << "   request with token=" << op << " completed";
    return *this;
}

io_uring_driver& io_uring_driver::wait_all(){

    BOOST_LOG_TRIVIAL(debug) << "wait_all request ";
    while(pending_operations.size()>0) {
        unsigned& head = *(cq_ring.head);
        unsigned& tail = *(cq_ring.tail);
        std::lock_guard guard{lock_wait};
        unsigned index = head & *(cq_ring.ring_mask);
        io_uring_cqe& cqe = cq_ring.entries[index];
        token_t observed_token = cqe.user_data;
        if(head!=tail){
            pending_operations.erase(observed_token);
            BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << observed_token << " done";
            BOOST_LOG_TRIVIAL(debug) << "   haed=" << head << " tail=" << tail;
            BOOST_LOG_TRIVIAL(debug) << "   Opeation result=" << cqe.res;
            if(cqe.res < 0)
                throw std::system_error(errno, std::system_category(), "negative result operation");
            ++head;
        }
    }
    return *this;

}


}
