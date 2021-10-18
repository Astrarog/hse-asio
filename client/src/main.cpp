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
#include <boost/program_options.hpp>

namespace ba = boost::asio;
namespace po = boost::program_options;

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
            return;

        // we need to write data

        status = CONNECTED;

        using namespace std::placeholders;
        auto got_written = std::bind(&session::handle_write, shared_from_this(), _1, _2);
        ba::async_write(sock, ba::const_buffer(buffer.data(), buffer.size()), std::move(got_written));

    }

    void handle_write(const boost::system::error_code& error, std::size_t transfered){

        if (error)
            return;

//         we need to read data back

        status = WRITE;

        using namespace std::placeholders;
        auto got_read = std::bind(&session::handle_read, shared_from_this(), _1, _2);

        ba::async_read(sock, ba::buffer(buffer.data(), buffer.size()), std::move(got_read));


    }

    void handle_read(const boost::system::error_code& error, std::size_t transfered){

        if (error)
            return;

        // just relax all was done
        status = READ;

    }
};

int main(int argc, char* argv[]){

    std::int32_t hc = std::thread::hardware_concurrency();
    std::uint32_t threads = std::thread::hardware_concurrency();

    // server endpoint
    std::string address;
    int port;


    std::size_t buffer_size;
    std::size_t nsesssions;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help")
        ("threads,t", po::value<std::uint32_t>(&threads)->default_value(1u), "Set the number of threads")
        ("address,a", po::value<std::string>(&address)->default_value("127.0.0.1"), "Set the address of server to connect")
        ("port,p", po::value<int>(&port)->default_value(8888), "Set the port of server to connect")
        ("bsize,b", po::value<std::size_t>(&buffer_size)->default_value(512u), "Set the size of the data to be transfered")
        ("sessions,s", po::value<std::size_t>(&nsesssions)->default_value(1u), "Set the number of parallel sessions for simultaneous data transfer")
    ;

    po::variables_map vm;
    po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    if (vm.count("threads") && threads == 0u) {
        std::cout << "There  should be at least 1 thread of execution. Automaically setting --threads=1." << "\n";
        threads = 1u;
    }

    if (vm.count("port") && port < 0) {
        std::cout << "Port should be positve number" << "\n";
        return 1;
    }


    ba::io_context ctx{int(threads)};

    ba::ip::tcp::endpoint ep( ba::ip::make_address(address), port);
    std::vector<boost::shared_ptr<session>> all_sessions;
    all_sessions.reserve(nsesssions);
    for (std::size_t i=0; i<nsesssions; ++i){
        all_sessions.push_back(boost::make_shared<session>(session(buffer_size, ctx)));
        all_sessions[i]->start(ep);
    }


    auto start = std::chrono::steady_clock::now();
    // Start time mesaurment

    ba::static_thread_pool tp{threads-1};
    for(unsigned i=1;i<threads;++i)
        ba::execution::execute(tp.get_executor(), [&]{
            ctx.run();
        });
    ctx.run();

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> time = end-start;

    std::uint64_t started   = std::count_if(all_sessions.begin(), all_sessions.end(), [](boost::shared_ptr<session> s){return s->status == session::STARTED;})
                , connected = std::count_if(all_sessions.begin(), all_sessions.end(), [](boost::shared_ptr<session> s){return s->status == session::CONNECTED;})
                , write     = std::count_if(all_sessions.begin(), all_sessions.end(), [](boost::shared_ptr<session> s){return s->status == session::WRITE;})
                , read      = std::count_if(all_sessions.begin(), all_sessions.end(), [](boost::shared_ptr<session> s){return s->status == session::READ;})
            ;

// format
// threads:sessions:nbytes:time(ms):STARTED:CONNECTED:WRITE:READ
    std::cout << threads << ':' << nsesssions << ':' << buffer_size << ':' << time.count() << ':' << started << ':' << connected << ':' << write << ':' << read  << std::endl;

    return 0;
}
