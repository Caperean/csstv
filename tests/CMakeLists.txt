add_executable(csstv_tests
    test_main.cpp
    test_image.cpp
    test_modes.cpp
)

target_include_directories(csstv_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/internal
    ${CMAKE_SOURCE_DIR}/src/modes
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(csstv_tests PRIVATE csstv_lib)

add_test(NAME csstv_tests COMMAND csstv_tests)

