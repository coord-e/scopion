cmake_minimum_required(VERSION 2.8)

add_library(Assembly STATIC
  Assembly.cpp
)

target_link_libraries(Assembly ${LLVM_LIBRARIES} ${Boost_LIBRARIES})
