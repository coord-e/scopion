cmake_minimum_required(VERSION 2.8)

add_executable(scopion_bin scopion.cpp)
set_target_properties(scopion_bin PROPERTIES OUTPUT_NAME scopion)
target_link_libraries(scopion_bin scopion ${LLVM_LIBRARIES} ${Boost_LIBRARIES})

INSTALL(TARGETS scopion_bin RUNTIME DESTINATION bin)
