add_executable(${PROJECT_NAME}-server
    src/async_file_descriptor.hpp
    src/composite_async_ops.hpp
    src/defer_reactor_adapter.hpp
    src/main.cpp
    src/select_reactor.cpp
    src/select_reactor.hpp
    src/task.hpp
    src/tcp_listener.cpp
    src/tcp_listener.hpp
    src/thread_pool.cpp
    src/thread_pool.hpp
    src/file_descriptor.cpp
    src/file_descriptor.hpp
    src/errno.hpp
    src/charconv.hpp
    src/format.hpp
    src/pipe.hpp
    src/reactor.hpp
)

target_compile_features(${PROJECT_NAME}-server PRIVATE cxx_std_20)

set_target_properties(${PROJECT_NAME}-server PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME}-server PUBLIC Boost::headers)
target_link_libraries(${PROJECT_NAME}-server PRIVATE Threads::Threads)
