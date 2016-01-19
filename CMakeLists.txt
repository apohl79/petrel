project(petrel-root)
cmake_minimum_required(VERSION 2.8)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

add_subdirectory(src)

if(NOT DISABLE_TESTS STREQUAL "on")
  enable_testing()
  add_subdirectory(test)
endif(NOT DISABLE_TESTS STREQUAL "on")
