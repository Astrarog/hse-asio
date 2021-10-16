add_executable(${PROJECT_NAME}-server
    src/file_descriptor.hpp
    src/syscall_handler.hpp
    src/main.cpp
    src/receiver.hpp                src/receiver.cpp
    src/server.cpp                  src/server.hpp
    src/worker.cpp                  src/worker.hpp

)

target_compile_features(${PROJECT_NAME}-server PRIVATE cxx_std_20)

set_target_properties(${PROJECT_NAME}-server PROPERTIES CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(liburing REQUIRED IMPORTED_TARGET liburing)


target_link_libraries(${PROJECT_NAME}-server PRIVATE PkgConfig::liburing)
target_link_libraries(${PROJECT_NAME}-server PRIVATE ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME}-server PRIVATE Threads::Threads)
