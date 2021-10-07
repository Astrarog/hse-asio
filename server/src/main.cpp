#include <string>
#include <span>
#include <cstddef>
#include <string_view>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <liburing.h>

#include <boost/log/trivial.hpp>


void uring_write(io_uring* ring,
                 int fd,
                 iovec data[],
                 std::size_t data_size)
{
    struct io_uring_sqe sqe{};
    struct io_uring_cqe* cqe;

    sqe = *io_uring_get_sqe(ring);

    io_uring_prep_writev(&sqe, fd, data, 1, 0);
    io_uring_submit(ring);

    io_uring_wait_cqe(ring, &cqe);

    BOOST_LOG_TRIVIAL(debug) << "TOKEN=" << cqe->user_data << " done";
    BOOST_LOG_TRIVIAL(debug) << "Opeation Result=" << cqe->res;
    BOOST_LOG_TRIVIAL(debug) << "errno?=EBADF " << ((-cqe->res)==EBADF);
    io_uring_cqe_seen(ring, cqe);

}


int main(){

    int fd = open("test1.txt", O_RDWR | O_CREAT | O_TRUNC | O_DIRECT , 0666);
    iovec iov[3];

    const char *buf[] = {
                    "The term buccaneer comes from the word boucan.\n",
                    "A boucan is a wooden frame used for cooking meat.\n",
                    "Buccaneer is the West Indies name for a pirate.\n"
    };

    for (int i = 0; i < 3; i++) {
            iov[i].iov_base = reinterpret_cast<std::byte*>(const_cast<char*>(buf[i]));
            iov[i].iov_len = std::string(buf[i]).size() + 1;
    }


    struct io_uring ring{};
    io_uring_queue_init(4096, &ring, 0);

    uring_write(&ring, fd, iov, 3);
    uring_write(&ring, fd, iov, 3);

    io_uring_queue_exit(&ring);
}
