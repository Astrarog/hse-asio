#ifndef RECEIVER_HPP
#define RECEIVER_HPP
#include <unordered_set>
#include <concepts>

#include <liburing.h>

namespace hse {

//template <std::integral Token_T>
template <typename Token_T>
class cqe_receiver
{
    io_uring& ring;
    std::unordered_set<Token_T>& token_table;
public:
    cqe_receiver(io_uring& _ring, std::unordered_set<Token_T>& _token_table):
        ring(_ring), token_table(_token_table) {}

    //after this methon seen() should be calld
    cqe_receiver& seen(io_uring_cqe& cqe){
        io_uring_cqe_seen(&ring, cqe);
        return *this;
    }

    //after this methon seen() should be calld
    io_uring_cqe& show_next()
    {
        io_uring_cqe* cqe;

        // check the difference between wait and peek
        io_uring_wait_cqe(&ring, &cqe);

        BOOST_LOG_TRIVIAL(debug) << "CQE Arrived. ";
        BOOST_LOG_TRIVIAL(debug) << "   CQE=" << cqe ;
        BOOST_LOG_TRIVIAL(debug) << "   TOKEN=" << cqe->user_data ;
        BOOST_LOG_TRIVIAL(debug) << "   FLAGS=" << (cqe->flags);
        BOOST_LOG_TRIVIAL(debug) << "   Opeation Result=" << cqe->res;

        io_uring_cqe copy = *cpe;

        io_uring_cqe_seen(&ring, cqe);

        return *cpe;

    }

    io_uring_cqe copy_next(){
        io_uring_cqe cqe = show_next();
        seen(cqe);
        return cqe;
    }
};

}

#endif // RECEIVER_HPP
