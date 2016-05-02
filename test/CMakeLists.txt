project(test)
cmake_minimum_required(VERSION 2.8)

set(COMP_FLAGS "-Wall -Wextra -Wlong-long -Wmissing-braces -g")

find_package(Boost 1.59.0 COMPONENTS
  system
  fiber
  context
  unit_test_framework
  REQUIRED)
find_package(OpenSSL REQUIRED)
#find_package(LUA REQUIRED)
find_package(PkgConfig)
pkg_check_modules(NGHTTP2 libnghttp2)
pkg_check_modules(NGHTTP2_ASIO libnghttp2_asio)

if(STOCK_LUA)
    find_package(LUA REQUIRED)
    set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
    set(COMP_FLAGS "${COMP_FLAGS} -DSTOCK_LUA=1")
else()
    pkg_check_modules(LUA REQUIRED luajit)
endif()

# GCC colors
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMP_FLAGS "${COMP_FLAGS} -fdiagnostics-color=auto")
endif()

# FIXME: fiber causes unused-parameter warnings, so take it out for now
set(COMP_FLAGS "${COMP_FLAGS} -Wno-unused-parameter")

if(VALGRIND)
  set(COMP_FLAGS "${COMP_FLAGS} -DBOOST_USE_VALGRIND=1")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
    set(CMAKE_MACOSX_RPATH 1)
endif()

set(CMAKE_C_FLAGS "${COMP_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -std=c++11")

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(../src)
include_directories(../src/core)
include_directories(../src/lib)
include_directories(../src/metrics)
include_directories(../src/boost/http/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${LUA_INCLUDE_DIRS})
include_directories(${NGHTTP2_INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS})
link_directories(${OPENSSL_LIBRARY_DIRS})
link_directories(${LUA_LIBRARY_DIRS})
link_directories(${NGHTTP2_LIBRARY_DIRS})

macro(add_test_target target)
  add_executable(${target} ${target}.cpp)
  target_link_libraries("${target}"
    petrel_core
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${NGHTTP2_LIBRARIES}
    ${NGHTTP2_ASIO_LIBRARIES}
    ${LUA_LIBRARIES}
    pthread m)
  add_test(${target} ${target})
endmacro()

file(GLOB tests "test_*.cpp")
foreach(test ${tests})
  string(REGEX REPLACE ".cpp" "" test_name ${test})
  string(REGEX REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" test_name ${test_name})
  add_test_target(${test_name})
endforeach()
