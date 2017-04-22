cmake_minimum_required(VERSION 2.8)

add_executable(scopc scopc.cpp)
target_link_libraries(scopc Assembly Parser ${LLVM_LIBRARIES} ${Boost_LIBRARIES})
