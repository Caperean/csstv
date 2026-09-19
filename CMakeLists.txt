cmake_minimum_required(VERSION 3.16)
project(CSSTV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE CSSTV_SOURCES
    src/*.cpp
)

add_library(csstv_lib ${CSSTV_SOURCES})

target_include_directories(csstv_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/internal
    ${CMAKE_SOURCE_DIR}/src/modes
    ${CMAKE_SOURCE_DIR}/tests
    
)

enable_testing()
add_subdirectory(tests)