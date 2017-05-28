cmake_minimum_required(VERSION 2.8)

add_executable(assembly_test main.cpp)
target_link_libraries(assembly_test scopion gtest gtest_main pthread)

add_test(
    NAME assembly
    COMMAND $<TARGET_FILE:assembly_test>
    )

set_property(
    TEST assembly
    PROPERTY LABELS lib assembly
    )
