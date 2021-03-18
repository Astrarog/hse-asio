#include <string>
#include <system_error>

#include <sys/mman.h>
#include <unistd.h>

//#include <boost/log/trivial.hpp>

#include "io_uring_driver.hpp"
#include "io_uring.hpp"
#include "syscall_handler.hpp"

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
    sq_ring.size = params_.sq_off.array + params_.sq_entries * sizeof (unsigned);
    cq_ring.size = params_.cq_off.cqes  + params_.cq_entries * sizeof (io_uring_cqe);


    std::uintptr_t sq_ring_ptr =
        reinterpret_cast<std::uintptr_t>(
            syscall_handler(mmap)
            .set_error_return_value(MAP_FAILED)
            .set_description("sq_ring mmap")
            .run(nullptr, sq_ring.size, PROT_READ | PROT_WRITE,
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
                .run(nullptr, cq_ring.size, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE, uring_fd, IORING_OFF_CQ_RING)
            );
    }

    std::uintptr_t sqes_size = params_.sq_entries * sizeof(struct io_uring_sqe);

    std::uintptr_t sqes_ptr =
        reinterpret_cast<std::uintptr_t>(    std::uintptr_t sqes_ptr =
            reinterpret_cast<std::uintptr_t>(
                syscall_handler(mmap)
                .set_error_return_value(MAP_FAILED)
                .set_description("sqes mmap")
                .run(nullptr, sqes_ptr, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE, uring_fd, IORING_OFF_SQES)
            );
            syscall_handler(mmap)
            .set_error_return_value(MAP_FAILED)
            .set_description("sqes mmap")
            .run(nullptr, sqes_ptr, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, uring_fd, IORING_OFF_SQES)
        );

    //we can safely copy ring info
    sq_ring.info = params_.sq_off;
    cq_ring.info = params_.cq_off;

    sq_ring.ptr = sq_ring_ptr;
    cq_ring.ptr = cq_ring_ptr;

    sq_ring.entries = reinterpret_cast<io_uring_sqe*>(sqes_ptr);
    cq_ring.entries = reinterpret_cast<io_uring_cqe*>(cq_ring.ptr + cq_ring.info.cqes);

}

io_uring_driver::~io_uring_driver(){
    close(uring_fd);
}

}
