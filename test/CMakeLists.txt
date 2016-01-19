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
find_package(LUA REQUIRED)
find_package(PkgConfig)
pkg_check_modules(NGHTTP2 libnghttp2)
pkg_check_modules(NGHTTP2_ASIO libnghttp2_asio)

# GCC colors
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMP_FLAGS "${COMP_FLAGS} -fdiagnostics-color=auto")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

if(NOT Boost_USE_STATIC_LIBS)
  set(COMP_FLAGS "${COMP_FLAGS} -DBOOST_TEST_DYN_LINK")
endif()

set(CMAKE_C_FLAGS "${COMP_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -std=c++11")

include_directories(../src)
include_directories(../src/core)
include_directories(../src/lib)
include_directories(../src/metrics)
include_directories(../src/boost/http/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${LUA_INCLUDE_DIR})
include_directories(${NGHTTP2_INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS})
link_directories(${OPENSSL_LIBRARY_DIRS})
link_directories(${LUA_LIBRARY_DIRS})
link_directories(${NGHTTP2_LIBRARY_DIRS})

add_executable(test_counter test_counter.cpp)
target_link_libraries(test_counter
  ${Boost_LIBRARIES})
add_test(test_counter test_counter)

add_executable(test_router test_router.cpp)
target_link_libraries(test_router
  ${Boost_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${NGHTTP2_LIBRARIES}
  ${NGHTTP2_ASIO_LIBRARIES}
  pthread)
add_test(test_router test_router)

add_executable(test_hash test_hash.cpp)
target_link_libraries(test_hash
  petrel_core
  ${Boost_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${NGHTTP2_LIBRARIES}
  ${LUA_LIBRARIES})
add_test(test_hash test_hash)
