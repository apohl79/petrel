# - Try to find tcmalloc
# Once done this will define
#  TCMALLOC_FOUND        - System has tcmalloc
#  TCMALLOC_INCLUDE_DIRS - The tcmalloc include directories
#  TCMALLOC_LIBRARIES    - The libraries needed to use tcmalloc

find_path(TCMALLOC_INCLUDE_DIR NAMES gperftools/tcmalloc.h)
find_library(TCMALLOC_LIBRARY NAMES tcmalloc PATH_SUFFIXES lib64 lib)

if(TCMALLOC_INCLUDE_DIR)
  set(_version_regex "^#define[ \t]+TC_VERSION_STRING[ \t]+\"([^\"]+)\".*")
  file(STRINGS "${TCMALLOC_INCLUDE_DIR}/gperftools/tcmalloc.h"
       TCMALLOC_VERSION REGEX "${_version_regex}")
  string(REGEX REPLACE "${_version_regex}" "\\1"
         TCMALLOC_VERSION "${TCMALLOC_VERSION}")
  unset(_version_regex)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set TCMALLOC_FOUND to TRUE
# if all listed variables are TRUE and the requested version matches.
find_package_handle_standard_args(Tcmalloc REQUIRED_VARS
                                  TCMALLOC_LIBRARY TCMALLOC_INCLUDE_DIR
                                  VERSION_VAR TCMALLOC_VERSION)

if(TCMALLOC_FOUND)
  set(TCMALLOC_LIBRARIES    ${TCMALLOC_LIBRARY})
  set(TCMALLOC_INCLUDE_DIRS ${TCMALLOC_INCLUDE_DIR})
endif()

mark_as_advanced(TCMALLOC_INCLUDE_DIR TCMALLOC_LIBRARY)
