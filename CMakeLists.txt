cmake_minimum_required(VERSION 3.20)
project(cdgnx VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cdgnx STATIC
        src/x86_64.cpp
)

target_include_directories(cdgnx PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_executable(cdgnx-test 
        tests/test_main.cpp
)

target_include_directories(cdgnx-test PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cdgnx-test PRIVATE
        cdgnx
)