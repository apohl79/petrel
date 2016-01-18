project(petrel)
cmake_minimum_required(VERSION 2.8)

file(STRINGS ${CMAKE_SOURCE_DIR}/VERSION PROJECT_VERSION)

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(PETREL_VERSION ${PROJECT_VERSION})
else(CMAKE_BUILD_TYPE MATCHES "Release")
  set(PETREL_VERSION "${PROJECT_VERSION}-DEV")
endif(CMAKE_BUILD_TYPE MATCHES "Release")

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL Linux))
  set(LIB_SUFFIX 64)
else(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL Linux))
  set(LIB_SUFFIX "")
endif(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL Linux))
set(INSTALL_LIB_DIR lib${LIB_SUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR)

find_package(Boost 1.59.0 COMPONENTS
  system
  thread
  program_options
  date_time
  filesystem
  fiber
  context
  REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LUA REQUIRED)
find_package(PkgConfig)
pkg_check_modules(NGHTTP2 libnghttp2)
pkg_check_modules(NGHTTP2_ASIO libnghttp2_asio)

set(COMP_FLAGS "-Wall -Wextra -Wmissing-braces -pthread -g")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMP_FLAGS "${COMP_FLAGS} -fdiagnostics-color=auto")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

set(CMAKE_C_FLAGS "${COMP_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_VERSION=\\\"${PETREL_VERSION}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIB_PREFIX=\\\"${CMAKE_SHARED_MODULE_PREFIX}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIB_SUFFIX=\\\"${CMAKE_SHARED_MODULE_SUFFIX}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIBDIR_SUFFIX=\\\"${LIB_SUFFIX}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPETREL_LIB_INSTALL_DIR=\\\"${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}\\\"")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/metrics")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/boost/http/include")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${LUA_INCLUDE_DIR})
include_directories(${NGHTTP2_INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS})
link_directories(${OPENSSL_LIBRARY_DIRS})
link_directories(${LUA_LIBRARY_DIRS})
link_directories(${NGHTTP2_LIBRARY_DIRS})

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
target_link_libraries(${LIB_CORE_NAME} dl)
install(TARGETS ${LIB_CORE_NAME} DESTINATION ${INSTALL_LIB_DIR})

add_executable(${PROJECT_NAME} main.cpp)
add_dependencies(${PROJECT_NAME} ${LIB_CORE_NAME})
target_link_libraries(${PROJECT_NAME} ${LIB_CORE_NAME})
target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# libraries
#add_subdirectory(lib/name)

if(SEGMENTED_STACKS STREQUAL "on")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMP_FLAGS "${COMP_FLAGS} -fsplit-stack -DBOOST_USE_SEGMENTED_STACKS")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")
    set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold")
    target_link_libraries(${LIB_CORE_NAME} gcc)
    target_link_libraries(${PROJECT_NAME} gcc)
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
endif(SEGMENTED_STACKS STREQUAL "on")

# install header files
file(GLOB HEADERS "core/server.h" "core/log.h" "lib/library.h" "metrics/*.h")
install(FILES "petrel.h" DESTINATION include)
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

# pkgconfig
configure_file(${CMAKE_SOURCE_DIR}/pkgconfig/petrel.pc.in ${CMAKE_BINARY_DIR}/pkgconfig/petrel.pc @ONLY)
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig DESTINATION ${INSTALL_LIB_DIR})
