#ifndef RECEIVER_HPP
#define RECEIVER_HPP
#include <unordered_map>
#include <concepts>

#include <liburing.h>

namespace hse {

class cqe_receiver
{
    io_uring& ring;
public:
    cqe_receiver(io_uring& _ring): ring(_ring) {}

    cqe_receiver& seen(io_uring_cqe& cqe);

    //after this methon seen() should be calld
    io_uring_cqe& show_next();

    io_uring_cqe copy_and_seen();
};

}

#endif // RECEIVER_HPP
