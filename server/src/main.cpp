#include <string>
#include <span>
#include <cstddef>
#include <string_view>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "io_operation.hpp"
#include "io_uring_driver.hpp"


using namespace hse;

int main(){
    int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC);
    iovec iov[3];

    const char *buf[] = {
                    "The term buccaneer comes from the word boucan.\n",
                    "A boucan is a wooden frame used for cooking meat.\n",
                    "Buccaneer is the West Indies name for a pirate.\n" };

    for (int i = 0; i < 3; i++) {
            iov[i].iov_base = reinterpret_cast<std::byte*>(const_cast<char*>(buf[i]));
            iov[i].iov_len = std::string(buf[i]).size() + 1;
    }


    std::span<std::byte> buffer (reinterpret_cast<std::byte*>(iov), 3);
    io_operation write_std =
        {io_operation_type::write_vec,
         buffer, fd, 0};


    hse::io_uring_driver drv;
    auto token0 = drv.register_op(write_std);
    auto token1 = drv.register_op();
    auto token2 = drv.register_op();
    auto token3 = drv.register_op();
    drv.initiate(token0)
            .wait(token0)
            .initiate_all()
            .wait_all();


}
