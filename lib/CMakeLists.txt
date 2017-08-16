cmake_minimum_required(VERSION 2.8)

add_subdirectory(assembly)
add_subdirectory(ast)
add_subdirectory(parser)

add_library(scopion SHARED)

target_link_libraries(scopion assembly ast parser ${CLANG_LIBS} ${Boost_LIBRARIES} ${LLVM_LIBRARIES})

INSTALL(TARGETS scopion LIBRARY DESTINATION lib)
