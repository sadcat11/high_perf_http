project(high_perf_http LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HIGH_PERF_HTTP_TEST "Build tests" ON)

add_library(hph
    src/file_descriptor.cpp
    src/http.cpp
    src/proxy.cpp
    src/socket.cpp
)

target_include_directories(hph PUBLIC include)
target_compile_options(hph PRIVATE -Wall -Wextra -Wpedantic)

add_executable(hphs src/main.cpp)
target_link_libraries(hphs PRIVATE hph)
target_compile_options(hphs PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(high_perf_http_test autotest/test_http.cpp)
target_link_libraries(high_perf_http_test PRIVATE hph)
target_compile_options(high_perf_http_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME high_perf_http_test COMMAND high_perf_http_test)