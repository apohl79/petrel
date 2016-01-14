project(petrel_example)
cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig)
pkg_check_modules(PETREL petrel REQUIRED)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL Linux))
  set(LIB_SUFFIX 64)
else(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL Linux))
  set(LIB_SUFFIX "")
endif(LIB64 AND (CMAKE_SYSTEM_NAME STREQUAL Linux))
set(INSTALL_LIB_DIR lib${LIB_SUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR)

set(COMP_FLAGS "-Wall -Wextra -Wlong-long -Wmissing-braces -g")

# GCC colors
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMP_FLAGS "${COMP_FLAGS} -fdiagnostics-color=auto")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

set(CMAKE_C_FLAGS "${COMP_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -std=c++1y")

include_directories(${PETREL_INCLUDE_DIRS})

aux_source_directory(. SOURCES)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${PETREL_LIBRARIES})
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_LIB_DIR})
