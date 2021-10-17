#include <cstdlib>
#include <string>
#include <cerrno>
#include <csignal>
#include <iomanip>

#include <unistd.h>
#include <netinet/in.h>

#include <boost/program_options.hpp>

#include "server.hpp"


namespace po = boost::program_options;
using namespace hse;


void sigint_handler(int /* signum */){
    std::cout << std::endl;
    std::cout << "Received end signal. Closing..." << std::endl;
    std::exit(0);
}

int main(int argc, char* argv[]){

    std::uint32_t threads;
    std::string shell;
    std::uint32_t enteties = 4096;
    std::uint32_t flags = 0;
    in_addr ip_address = static_cast<in_addr>(INADDR_LOOPBACK);
    in_port_t port = 8888u;

    char* shell_env = std::getenv("SHELL");
    auto hc = std::thread::hardware_concurrency();
    std::uint32_t max_threads = std::max(hc, 1u);

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help")
        ("threads,t", po::value<std::uint32_t>(&threads)->default_value(max_threads), "Set the number of threads")
        ("shell,s", po::value<std::string>(&shell)->default_value("/bin/bash"), "Set the shell. Absolute path required. Environment variable SHELL is used if this switch is not set.")
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
        shell = vm["shell"].as<std::string>();
        if(access(shell.c_str(), X_OK) == -1){
            std::cout << "Error: " << std::quoted(shell)<< " cannot be executed. Check the filename and its permissions.";
            return 1;
        }
    } else if (shell_env) {
        shell = shell_env;
    }

    std::signal(SIGINT, sigint_handler);
    std::signal(SIGPIPE, SIG_IGN);

    server telnet_like(std::move(shell),
                       threads,
                       enteties,
                       flags,
                       ip_address,
                       port);

    telnet_like.start();


}
