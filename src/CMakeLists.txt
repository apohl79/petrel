project(petrel)
cmake_minimum_required(VERSION 2.8)

file(STRINGS ${CMAKE_SOURCE_DIR}/VERSION PROJECT_VERSION)

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(PETREL_VERSION ${PROJECT_VERSION})
else(CMAKE_BUILD_TYPE MATCHES "Release")
  set(PETREL_VERSION "${PROJECT_VERSION}-DEV")
endif(CMAKE_BUILD_TYPE MATCHES "Release")

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
  set(LIB_SUFFIX 64)
else(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
  set(LIB_SUFFIX "")
endif(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
set(INSTALL_LIB_DIR lib${LIB_SUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR)

find_package(Boost 1.62.0 COMPONENTS
  system
  thread
  program_options
  date_time
  filesystem
  fiber
  context
  REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig)
pkg_check_modules(NGHTTP2 REQUIRED libnghttp2)
pkg_check_modules(NGHTTP2_ASIO REQUIRED libnghttp2_asio)

set(COMP_FLAGS "-Wall -Wextra -Wmissing-braces -pthread -g")

option(STOCK_LUA "Use stock lua instead of luajit." off)
if(STOCK_LUA)
    find_package(LUA REQUIRED)
    set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
    set(COMP_FLAGS "${COMP_FLAGS} -DSTOCK_LUA=1")
else()
    pkg_check_modules(LUA REQUIRED luajit)
endif()

option(DISABLE_JEMALLOC "Do not use jemalloc.")
if(NOT DISABLE_JEMALLOC STREQUAL "on")
    find_package(JEMalloc)
endif()

option(DISABLE_TCMALLOC "Do not use tcmalloc.")
if(NOT DISABLE_TCMALLOC STREQUAL "on")
    find_package(TCMalloc)
endif()

if(JEMALLOC_FOUND)
    message(STATUS "Linking jemalloc")
elseif(TCMALLOC_FOUND)
    message(STATUS "Linking tcmalloc")
endif()

# FIXME: fiber causes unused-parameter warnings, so take it out for now
set(COMP_FLAGS "${COMP_FLAGS} -Wno-unused-parameter")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMP_FLAGS "${COMP_FLAGS} -fdiagnostics-color=auto")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
    set(CMAKE_MACOSX_RPATH 1)
endif()

option(VALGRIND "Enable valgrind support. Note: you have to build boost with valgrind=on." off)

if(VALGRIND)
    message(STATUS "Enabling valgrind support")
    set(COMP_FLAGS "${COMP_FLAGS} -DBOOST_USE_VALGRIND=1")
endif()

option(GOOGLE_PROFILER "Enable google profiler support." off)

if(GOOGLE_PROFILER)
    set(COMP_FLAGS "${COMP_FLAGS} -DGOOGLE_PROFILER=1")
endif()

set(CMAKE_C_FLAGS "${COMP_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_VERSION=\\\"${PETREL_VERSION}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIB_PREFIX=\\\"${CMAKE_SHARED_MODULE_PREFIX}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIB_SUFFIX=\\\"${CMAKE_SHARED_MODULE_SUFFIX}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIBDIR_SUFFIX=\\\"${LIB_SUFFIX}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIB_INSTALL_DIR=\\\"${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}\\\"")

# install headers
file(GLOB HEADERS
  "core/server.h"
  "core/log.h"
  "core/branch.h"
  "core/lua_inc.h"
  "core/resolver_cache.h"
  "lib/library.h"
  "metrics/*.h"
  "fiber/*.hpp"
  "fiber/detail/*.hpp")
foreach(header ${HEADERS})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/include/petrel" header_to ${header})
    add_custom_command(
        OUTPUT "${header_to}"
        COMMAND cmake -E copy "${header}" "${header_to}"
        DEPENDS "${header}")
    list(APPEND HEADERS_DEST "${header_to}")
endforeach()
add_custom_target(headers_target ALL DEPENDS ${HEADERS_DEST})
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/petrel DESTINATION include)
install(FILES "petrel.h" DESTINATION include)

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/metrics")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/boost/http/include")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${LUA_INCLUDE_DIRS})
include_directories(${NGHTTP2_INCLUDE_DIRS})
if(JEMALLOC_FOUND)
    include_directories(${JEMALLOC_INCLUDE_DIRS})
elseif(TCMALLOC_FOUND)
    include_directories(${TCMALLOC_INCLUDE_DIRS})
endif()

link_directories(${Boost_LIBRARY_DIRS})
link_directories(${OPENSSL_LIBRARY_DIRS})
link_directories(${LUA_LIBRARY_DIRS})
link_directories(${NGHTTP2_LIBRARY_DIRS})
if(JEMALLOC_FOUND)
    link_directories(${JEMALLOC_LIBRARY_DIRS})
elseif(TCMALLOC_FOUND)
    link_directories(${TCMALLOC_LIBRARY_DIRS})
endif()

set(LIB_CORE_NAME ${PROJECT_NAME}_core)
aux_source_directory(core LIB_CORE_SOURCES)
aux_source_directory(metrics LIB_CORE_SOURCES)
aux_source_directory(lib/builtin LIB_CORE_SOURCES)
aux_source_directory(boost/http/src LIB_CORE_SOURCES)
add_library(${LIB_CORE_NAME} SHARED ${LIB_CORE_SOURCES})
set_target_properties(${LIB_CORE_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION})
target_link_libraries(${LIB_CORE_NAME} ${NGHTTP2_LIBRARIES})
target_link_libraries(${LIB_CORE_NAME} ${NGHTTP2_ASIO_LIBRARIES})
target_link_libraries(${LIB_CORE_NAME} ${Boost_LIBRARIES})
target_link_libraries(${LIB_CORE_NAME} ${OPENSSL_LIBRARIES})
target_link_libraries(${LIB_CORE_NAME} ${LUA_LIBRARIES})
target_link_libraries(${LIB_CORE_NAME} dl m)
install(TARGETS ${LIB_CORE_NAME} DESTINATION ${INSTALL_LIB_DIR})

add_executable(${PROJECT_NAME} main.cpp)
add_dependencies(${PROJECT_NAME} headers_target)
add_dependencies(${PROJECT_NAME} ${LIB_CORE_NAME})
target_link_libraries(${PROJECT_NAME} ${LIB_CORE_NAME})
if(JEMALLOC_FOUND)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(${PROJECT_NAME} -Wl,--no-as-needed ${JEMALLOC_LIBRARIES} -Wl,--as-needed)
    else()
        target_link_libraries(${PROJECT_NAME} ${JEMALLOC_LIBRARIES})
    endif()
elseif(TCMALLOC_FOUND)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(${PROJECT_NAME} -Wl,--no-as-needed ${TCMALLOC_LIBRARIES} -Wl,--as-needed)
    else()
        target_link_libraries(${PROJECT_NAME} ${TCMALLOC_LIBRARIES})
    endif()
endif()
if(GOOGLE_PROFILER)
    target_link_libraries(${PROJECT_NAME} profiler)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

option(SEGMENTED_STACKS "Enable segmented stacks support. Note: You have to build boost with segmented-stacks=on." off)

if(SEGMENTED_STACKS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Enabling segmented stacks support")
        set(COMP_FLAGS "${COMP_FLAGS} -fsplit-stack -DBOOST_USE_SEGMENTED_STACKS")
        set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")
        set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold")
        target_link_libraries(${LIB_CORE_NAME} gcc)
        target_link_libraries(${PROJECT_NAME} gcc)
    endif()
endif()

# pkgconfig
configure_file(${CMAKE_SOURCE_DIR}/pkgconfig/petrel.pc.in ${CMAKE_BINARY_DIR}/pkgconfig/petrel.pc @ONLY)
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig DESTINATION ${INSTALL_LIB_DIR})
