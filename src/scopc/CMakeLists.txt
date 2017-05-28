cmake_minimum_required(VERSION 2.8)

add_executable(scopc_bin scopc.cpp)
set_target_properties(scopc_bin PROPERTIES OUTPUT_NAME scopc)
target_link_libraries(scopc_bin scopion ${LLVM_LIBRARIES} ${Boost_LIBRARIES})

INSTALL(TARGETS scopc_bin RUNTIME DESTINATION bin)
