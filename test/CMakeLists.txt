cmake_minimum_required(VERSION 3.1.0)

project(LiveGrapherTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/host/include")
file(GLOB SRCS
    "${PROJECT_SOURCE_DIR}/host/cpp/livegrapher/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    )
add_executable(LiveGrapherTest ${SRCS})

target_compile_options(LiveGrapherTest PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)
target_link_libraries(LiveGrapherTest Threads::Threads)
