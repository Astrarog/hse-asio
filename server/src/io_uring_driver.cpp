#include <string>
#include <system_error>
#include <span>
#include <mutex>

#include <sys/mman.h>
#include <unistd.h>

//#include <boost/log/trivial.hpp>

#include "syscall_handler.hpp"
#include "io_uring_driver.hpp"
#include "io_uring.hpp"
#include "io_operation.hpp"

namespace hse {


void debug_check_features(std::uint32_t feat){
//    BOOST_LOG_TRIVIAL(debug) << "io_uring features:";

//    // only IORING_FEAT_SINGLE_MMAP is supported in 5.12 kernel
//    if(IORING_FEAT_SINGLE_MMAP & feat)
//        BOOST_LOG_TRIVIAL(debug) << "    IORING_FEAT_SINGLE_MMAP is enabled";
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

//    BOOST_LOG_TRIVIAL(debug) << "io_uring setup params:";
//    BOOST_LOG_TRIVIAL(debug) << "    sq_entries = " <<  params_.sq_entries;
//    BOOST_LOG_TRIVIAL(debug) << "    cq_entries = " <<  params_.cq_entries;

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

std::uint64_t io_uring_driver::register_operation(io_operation op) {

    std::unique_lock guard{lock};

    unsigned tail = *(sq_ring.tail);
    unsigned index = tail & *(sq_ring.ring_mask);
    // at this point we know the position of empty SQE, so we can hold it
    ++(*(sq_ring.tail));

    guard.unlock();

    io_uring_sqe& sqe = sq_ring.entries[index];

    sqe.opcode = static_cast<int>(op.type);
    sqe.fd = op.fd;
    sqe.off = op.offest;
    sqe.addr = reinterpret_cast<std::uintptr_t>(op.data.data());
    sqe.len = op.data.size();

    sqe.flags = 0;
    sqe.ioprio = 0;
    sqe.rw_flags = 0;
    sqe.__pad2[0] = sqe.__pad2[1] = sqe.__pad2[2] = 0;

    // token to register the operation
    token_t token = last_token.fetch_add(1, std::memory_order_acquire);
    sqe.user_data = token;
    return token;

}


io_uring_driver::~io_uring_driver(){
    close(uring_fd);
}

}
