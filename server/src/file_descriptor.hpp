#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <system_error>
#include <utility>

#include <unistd.h>

namespace hse {


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
    }

    file_descriptor(const file_descriptor& other) = default;
    file_descriptor(file_descriptor&& other) noexcept
        : fd{std::exchange(other.fd,-1)}
    {}

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
