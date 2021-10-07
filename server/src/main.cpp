#include <string>
#include <span>
#include <cstddef>
#include <string_view>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <liburing.h>

#include <boost/log/trivial.hpp>

unsigned int idx = 1;

void uring_write(io_uring* ring,
                 int fd,
                 const char data[],
                 std::size_t data_size)
{
    struct io_uring_sqe* sqe;
    struct io_uring_cqe* cqe;

    sqe = io_uring_get_sqe(ring);

    io_uring_prep_write(sqe, fd, data, data_size, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(idx++));

    io_uring_submit(ring);

    io_uring_wait_cqe(ring, &cqe);

    BOOST_LOG_TRIVIAL(debug) << "CQE=" << cqe << " done";
    BOOST_LOG_TRIVIAL(debug) << "TOKEN=" << cqe->user_data << " done";
    BOOST_LOG_TRIVIAL(debug) << "FLAGS=" << (cqe->flags);
    BOOST_LOG_TRIVIAL(debug) << "Opeation Result=" << cqe->res;
    BOOST_LOG_TRIVIAL(debug) << "errno?=EBADF " << ((-cqe->res)==EBADF);
    BOOST_LOG_TRIVIAL(debug) << "errno?=EINVAL " << ((-cqe->res)==EINVAL);
    io_uring_cqe_seen(ring, cqe);

}

constexpr char buf[] = "The term buccaneer comes from the word boucan.123\n";

int main(){

    int fd = open("test1.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    iovec iov[3];

    std::size_t bufSize = std::strlen(buf);

    struct io_uring ring{};
    io_uring_queue_init(4096, &ring, 0);

    uring_write(&ring, fd, buf, bufSize);
    uring_write(&ring, fd, buf, bufSize);


    io_uring_queue_exit(&ring);
}
