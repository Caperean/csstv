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

# All PD modes are exercised by the test suite (encode + decode).
target_compile_definitions(csstv_lib PUBLIC
    CSSTV_ENCODER_MODE_PD50=1
    CSSTV_ENCODER_MODE_PD90=1
    CSSTV_ENCODER_MODE_PD120=1
    CSSTV_ENCODER_MODE_PD160=1
    CSSTV_ENCODER_MODE_PD180=1
    CSSTV_ENCODER_MODE_PD240=1
    CSSTV_ENCODER_MODE_PD290=1
    CSSTV_DECODER_MODE_PD50=1
    CSSTV_DECODER_MODE_PD90=1
    CSSTV_DECODER_MODE_PD120=1
    CSSTV_DECODER_MODE_PD160=1
    CSSTV_DECODER_MODE_PD180=1
    CSSTV_DECODER_MODE_PD240=1
    CSSTV_DECODER_MODE_PD290=1
    CSSTV_ENABLE_ENCODER=1
    CSSTV_ENABLE_DECODER=1
)

enable_testing()
add_subdirectory(tests)