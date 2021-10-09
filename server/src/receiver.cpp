#include "receiver.hpp"

namespace hse {

//after this methon seen() should be calld
cqe_receiver& cqe_receiver::seen(io_uring_cqe& cqe){
    io_uring_cqe_seen(&ring, &cqe);
    return *this;
}

//after this methon seen() should be calld
io_uring_cqe& cqe_receiver::show_next(){
    io_uring_cqe* cqe;

    // check the difference between wait and peek
    io_uring_wait_cqe(&ring, &cqe);

    BOOST_LOG_TRIVIAL(debug) << "CQE Arrived. ";
    BOOST_LOG_TRIVIAL(debug) << "   CQE=" << cqe ;
    BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << cqe->user_data ;
    BOOST_LOG_TRIVIAL(debug) << "   FLAGS=" << (cqe->flags);
    BOOST_LOG_TRIVIAL(debug) << "   Opeation Result=" << cqe->res;

    return *cqe;

}

io_uring_cqe cqe_receiver::copy_and_seen(){
    io_uring_cqe cqe = show_next();
    seen(cqe);
    return cqe;
}

}
