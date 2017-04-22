cmake_minimum_required(VERSION 2.8)

add_executable(scopion scopion.cpp)
target_link_libraries(scopion Assembly Parser ${LLVM_LIBRARIES} ${Boost_LIBRARIES})
