#include <thread>
#include <fstream>
#include <string>
#include <functional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/static_thread_pool.hpp>
#include <boost/system/error_code.hpp>
#include <boost/enable_shared_from_this.hpp>
//#include <boost/asio/bind_executor.hpp>
//#include <boost/asio/buffer.hpp>
//#include <boost/asio/execution/execute.hpp>
//#include <boost/asio/read_until.hpp>
//#include <boost/asio/strand.hpp>
//#include <boost/asio/write.hpp>
//#include <boost/beast/core/tcp_stream.hpp>
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
    std::string buffer;
    ba::ip::tcp::socket sock;

public:

    session(std::size_t buffer_size,
            ba::io_context& ctx,
            ba::ip::tcp::endpoint& ep):
        buffer(random_data(buffer_size)),
        sock(ctx)
    {
        using namespace std::placeg
        auto next_accept = std::bind(&server::handle_accept, this, _1, _2);
        sock.async_connect(ep, )
    }

    void handle_connect(const boost::system::error_code& error){

    }

    void handle_write(const boost::system::error_code& error){

    }

    void handle_read(const boost::system::error_code& error){

    }
};

int main(int argc, char* argv[]){

    unsigned threads = std::thread::hardware_concurrency();

    ba::io_context ctx{int(threads)};



    // here we need to initiate our async operations.
    std::size_t nsesssions = 20;
    for (std::size_t i=0; i<nsesssions; ++i){

    }


    ba::static_thread_pool tp{threads-1};
    for(unsigned i=1;i<threads;++i)
        bae::execute(tp.get_executor(),[&]{
            ctx.run();
        });
    ctx.run();
    return 0;
}
