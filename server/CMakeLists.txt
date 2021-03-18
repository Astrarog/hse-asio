add_executable(${PROJECT_NAME}-server
    src/main.cpp
    src/rings.hpp
    src/syscall_handler.hpp
    src/io_uring_driver.cpp    src/io_uring_driver.hpp
    src/io_uring.cpp           src/io_uring.hpp

)

target_compile_features(${PROJECT_NAME}-server PRIVATE cxx_std_20)

set_target_properties(${PROJECT_NAME}-server PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME}-server PRIVATE Boost::headers)
target_link_libraries(${PROJECT_NAME}-server PRIVATE Boost::log)
target_link_libraries(${PROJECT_NAME}-server PRIVATE Threads::Threads)
