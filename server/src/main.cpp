#include <cstdlib>
#include <string>
#include <cerrno>
#include <csignal>

#include <boost/program_options.hpp>

#include "server.hpp"

namespace po = boost::program_options;
using namespace hse;


void sigint_handler(int /* signum */){
    std::cout << "Closing..." << std::endl;
    std::exit(0);
}

int main(int argc, char* argv[]){

    std::uint32_t threads;
    std::string shell;

    char* shell_env = std::getenv("SHELL");
    auto hc = std::thread::hardware_concurrency();
    std::uint32_t max_threads = ( hc>0 ? std::max(hc-1u, 1u): 1);

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help")
        ("threads,t", po::value<std::uint32_t>(&threads)->default_value(max_threads), "set compression level")
        ("shell,s", po::value<std::string>(&shell), "set compression level")
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

    if (vm.count("shell")) {

    }

    shell = ( vm.count("shell")
                ? vm["shell"].as<std::string>()
                : (shell_env ? shell_env : "/bin/bash" ));

    std::signal(SIGINT, sigint_handler);
    std::signal(SIGPIPE, SIG_IGN);

    server telnet_like(std::move(shell), 1);

    telnet_like.start();


}
