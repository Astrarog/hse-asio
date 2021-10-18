add_executable(${PROJECT_NAME}-client
    src/main.cpp
)

target_compile_features(${PROJECT_NAME}-client PRIVATE cxx_std_20)

set_target_properties(${PROJECT_NAME}-client PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME}-client PRIVATE ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME}-client PRIVATE Threads::Threads)
