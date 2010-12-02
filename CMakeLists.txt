SET(CNAME "ssa-utils")
SET(VERSION 0.02)

PROJECT(${CNAME} C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# cmake modules
INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)

IF    (NOT CMAKE_BUILD_TYPE)
  SET (CMAKE_BUILD_TYPE Debug)
  SET (CMAKE_C_FLAGS "-ggdb -Wall -pedantic")
ENDIF (NOT CMAKE_BUILD_TYPE)

# set paths
SET(CMAKE_INSTALL_PREFIX "/usr")

IF (NOT LIBDIR)
  SET (LIBDIR "lib")
ENDIF (NOT LIBDIR)

SET(REQUIRED_HEADERS
    "ctype.h" "fenv.h" "stdarg.h" "stdbool.h" "stddef.h"
    "stdint.h" "stdio.h" "stdlib.h" "string.h" "unistd.h")

FOREACH   (HDR ${REQUIRED_HEADERS})
  CHECK_INCLUDE_FILE (${HDR}  TEST_H)
  IF    (NOT TEST_H)
    MESSAGE(FATAL_ERROR "Build aborted.")
  ENDIF (NOT TEST_H)
  UNSET(TEST_H CACHE)
ENDFOREACH()

## checks
# libm[ath]
FIND_LIBRARY(LIB_MATH m)

IF    (LIB_MATH)
  SET (BUILD_LIBS ${BUILD_LIBS} ${LIB_MATH})
  SET (MATH_FOUND "TRUE")
ELSE  (LIB_MATH)
  MESSAGE (SEND_ERROR "libm not found")
ENDIF (LIB_MATH)


# gettext
FIND_PACKAGE(Gettext REQUIRED)

if    (NOT GETTEXT_FOUND)
  MESSAGE(SEND_ERROR "gettext not found")
endif (NOT GETTEXT_FOUND)

MESSAGE (STATUS "------------------------------------------")
MESSAGE (STATUS "Build type is:     ${CMAKE_BUILD_TYPE}")
MESSAGE (STATUS "")
MESSAGE (STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
MESSAGE (STATUS "Lib directory:     ${CMAKE_INSTALL_PREFIX}/${LIBDIR}")
MESSAGE (STATUS "")
MESSAGE (STATUS "Required libraries:")
MESSAGE (STATUS "  libmath: ${MATH_FOUND}")
MESSAGE (STATUS "")
MESSAGE (STATUS "Aux dependencies:")
MESSAGE (STATUS "  gettext: ${GETTEXT_FOUND}")
MESSAGE (STATUS "------------------------------------------")

ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (po)
