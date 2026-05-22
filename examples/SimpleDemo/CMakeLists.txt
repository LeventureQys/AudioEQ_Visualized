cmake_minimum_required(VERSION 3.20)
project(SimpleDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(SimpleDemo main.cpp)

target_include_directories(SimpleDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/../../src
    ${CMAKE_SOURCE_DIR}/../../thirdparty/code/volk
    ${CMAKE_SOURCE_DIR}/../../thirdparty/code/stb
    ${CMAKE_SOURCE_DIR}/../../thirdparty/code/glm
)

target_link_libraries(SimpleDemo PRIVATE
    AudioEQ
    Qt6::Widgets
)
