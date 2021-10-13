#include <cstdlib>
#include <string>

#include "server.hpp"

using namespace hse;

int main(){

    char* shell_env = std::getenv("SHELL");

    std::string shell = (shell_env? shell_env : "/bin/bash");

    server telnet_like(std::move(shell), 10);

    telnet_like.start();


}
