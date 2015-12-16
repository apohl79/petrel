project(petrel)
cmake_minimum_required(VERSION 2.8)

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
pkg_check_modules(NGHTTP2 libnghttp2_asio)

set(COMP_FLAGS "-Wall -Wextra -Wlong-long -Wmissing-braces -pthread -g")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMP_FLAGS "${COMP_FLAGS} -fdiagnostics-color=auto")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

set(CMAKE_C_FLAGS "${COMP_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -std=c++1y")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/boost/http/include")
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${LUA_INCLUDE_DIR})
include_directories(${NGHTTP2_INCLUDE_DIRS})

aux_source_directory(. SOURCES)
aux_source_directory(.. SOURCES)
aux_source_directory(lib SOURCES)
aux_source_directory(metrics SOURCES)
aux_source_directory(adtech SOURCES)
aux_source_directory(boost/http/src SOURCES)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES} dl)
target_link_libraries(${PROJECT_NAME} ${NGHTTP2_LIBRARIES})

# ADTECH
target_link_libraries(${PROJECT_NAME} avro)
