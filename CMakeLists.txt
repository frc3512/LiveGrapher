cmake_minimum_required(VERSION 3.1.0)

project(LiveGrapher)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Network Widgets PrintSupport REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories("${PROJECT_SOURCE_DIR}")
file(GLOB SRCS
    "${PROJECT_SOURCE_DIR}/*.qrc"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
    )
add_executable(LiveGrapher ${SRCS})

target_link_libraries(LiveGrapher Qt5::Network Qt5::Widgets Qt5::PrintSupport)
