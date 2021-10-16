#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <system_error>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

namespace hse {

void inline set_close_on_exec(int fd){
    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    if ( fcntl(fd, F_SETFD, flags) < 0)
        throw std::system_error(errno,std::system_category(), "Error on setting close on exec flag");
}


class file_descriptor
{
protected:
    int fd=-1;

public:
    void close() {
        ::close(fd);
        fd=-1;
    }

    inline file_descriptor(){}

    inline file_descriptor(int desc): fd{desc} {
        if (this->fd<0)
            throw std::system_error(errno,std::system_category(), "Negative file descriptor");
        set_close_on_exec(this->fd);
    }

    file_descriptor(const file_descriptor& other) = default;
    file_descriptor(file_descriptor&& other) noexcept
        : fd{std::exchange(other.fd,-1)} {
    }

    file_descriptor& operator=(file_descriptor&& other) noexcept {
        if(this!=&other) {
            this->close();
            fd = std::exchange(other.fd,-1);
        }
        return *this;
    }

    inline ~file_descriptor(){
        if (fd!=-1)
            ::close(fd);
    }

    inline operator int() const noexcept {
        return fd;
    }

};

}
#endif // FILE_DESCRIPTOR_HPP
