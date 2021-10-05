cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(hse-asio
            DESCRIPTION "Telnet(like) client and server implementation" 
            VERSION 0.0.0
            LANGUAGES CXX
)

set_property(GLOBAL
            PROPERTY CMAKE_CXX_KNOWN_FEATURES cxx_std_2a
)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost 1.75 REQUIRED COMPONENTS chrono container context log thread)

add_subdirectory(server)
#add_subdirectory(clent)
