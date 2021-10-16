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


    std::signal(SIGINT, sigint_handler);
    std::signal(SIGPIPE, SIG_IGN);

    char* shell_env = std::getenv("SHELL");

    std::string shell = ( shell_env ? shell_env : "/bin/bash" );

    server telnet_like(std::move(shell), 1);

    telnet_like.start();


}
