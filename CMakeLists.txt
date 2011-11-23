SET(CNAME "ssa-utils")
SET(VERSION 0.03)

PROJECT(${CNAME} C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# cmake modules
INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)

IF    (NOT CMAKE_BUILD_TYPE)
  SET (CMAKE_BUILD_TYPE Debug)
  SET (CMAKE_C_FLAGS "-ggdb -Wall -pedantic -DDEBUG")
ENDIF (NOT CMAKE_BUILD_TYPE)

# set paths
IF    (NOT CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF (NOT CMAKE_INSTALL_PREFIX)

SET(INSTALL_BIN "${CMAKE_INSTALL_PREFIX}/bin")
SET(INSTALL_LIB "${CMAKE_INSTALL_PREFIX}/lib")

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
  SET (MATH_FOUND "FOUND")
ELSE  (LIB_MATH)
  MESSAGE (SEND_ERROR "libm not found")
ENDIF (LIB_MATH)


# gettext
FIND_PACKAGE(Gettext REQUIRED)

IF    (NOT GETTEXT_FOUND)
  MESSAGE(SEND_ERROR "gettext not found")
ELSE  (NOT GETTEXT_FOUND)
  SET (GETTEXT_FOUND "FOUND")
ENDIF (NOT GETTEXT_FOUND)

MESSAGE (STATUS "------------------------------------------")
MESSAGE (STATUS "Build type is:        ${CMAKE_BUILD_TYPE}")
MESSAGE (STATUS "")
MESSAGE (STATUS "Install prefix:       ${CMAKE_INSTALL_PREFIX}")
MESSAGE (STATUS "Binaries directory:   ${INSTALL_BIN}")
MESSAGE (STATUS "")
MESSAGE (STATUS "Required libraries:")
MESSAGE (STATUS "  libmath: ${MATH_FOUND}")
MESSAGE (STATUS "")
MESSAGE (STATUS "Aux dependencies:")
MESSAGE (STATUS "  gettext: ${GETTEXT_FOUND}")
MESSAGE (STATUS "------------------------------------------")

ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (po)
