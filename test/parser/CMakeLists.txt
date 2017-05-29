cmake_minimum_required(VERSION 2.8)

add_executable(parser_test main.cpp)
target_link_libraries(parser_test scopion gtest gtest_main pthread)

add_test(
    NAME parser
    COMMAND $<TARGET_FILE:parser_test>
    )

set_property(
    TEST parser
    PROPERTY LABELS lib parser
    )
