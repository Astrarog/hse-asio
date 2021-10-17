#include <thread>
#include <fstream>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/static_thread_pool.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/system/error_code.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/core/noncopyable.hpp>

//#include <boost/asio/bind_executor.hpp>
//#include <boost/asio/read_until.hpp>
//#include <boost/asio/strand.hpp>
//#include <boost/asio/write.hpp>
//#include <boost/multiprecision/cpp_int.hpp>

namespace ba = boost::asio;

std::string random_data(std::size_t size){
    std::string data;
    data.resize(size);

    std::ifstream generator("/dev/urandom");
    generator.read(data.data(), size);
    return data;
}

class session : public boost::enable_shared_from_this<session> {
public:
    enum OPERATION_STATUS{
        STARTED,
        CONNECTED,
        WRITE,
        READ,
    };

private:
    std::string buffer;
    ba::ip::tcp::socket sock;

public:
    OPERATION_STATUS status = STARTED;

    void start(const ba::ip::tcp::endpoint& ep){
        using namespace std::placeholders;

        auto got_conneted = std::bind(&session::handle_connect, shared_from_this(), _1);
        sock.async_connect(ep, std::move(got_conneted));
    }

    session(std::size_t buffer_size,
            ba::io_context& ctx):
        buffer(random_data(buffer_size)),
        sock(ctx) {}

    void handle_connect(const boost::system::error_code& error){

        if (error)
            throw std::logic_error(error.message());

        // we need to write data

        status = CONNECTED;

        using namespace std::placeholders;
        auto got_written = std::bind(&session::handle_write, shared_from_this(), _1, _2);
        ba::async_write(sock, ba::const_buffer(buffer.data(), buffer.size()), std::move(got_written));

    }

    void handle_write(const boost::system::error_code& error, std::size_t transfered){

        if (error)
            throw std::logic_error(error.message());

//         we need to read data back

        status = WRITE;

        using namespace std::placeholders;
        auto got_read = std::bind(&session::handle_read, shared_from_this(), _1, _2);

        ba::async_read(sock, ba::buffer(buffer.data(), buffer.size()), std::move(got_read));


    }

    void handle_read(const boost::system::error_code& error, std::size_t transfered){

        if (error)
            throw std::logic_error(error.message());

        // just relax all was done
        status = READ;

    }
};

int main(int argc, char* argv[]){

    unsigned threads = 1; //std::thread::hardware_concurrency();

    ba::io_context ctx;//{int(threads)};

    // server endpoint
    ba::ip::tcp::endpoint ep( ba::ip::make_address("127.0.0.1"), 40385);

    std::size_t buffer_size = 512;

    std::size_t nsesssions = 1;

    std::vector<boost::shared_ptr<session>> all_sessions;
    all_sessions.reserve(nsesssions);
    for (std::size_t i=0, port=10000; i<nsesssions; ++i){
        all_sessions.push_back(boost::make_shared<session>(session(buffer_size, ctx)));
        all_sessions[i]->start(ep);
    }


//    ba::static_thread_pool tp{threads-1};
//    for(unsigned i=1;i<threads;++i)
//        ba::execution::execute(tp.get_executor(), [&]{
//            ctx.run();
//        });
    ctx.run();
    std::cout << "Done";
    return 0;
}
