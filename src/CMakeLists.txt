set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(MODULES_SRC "common.c")

# converters
add_executable(srt2ssa             ${MODULES_SRC} "ssa.c" "srt.c" "srt2ssa.c")
add_executable(microsub2ssa        ${MODULES_SRC} "ssa.c" "microsub.c" "microsub2ssa.c")

# various utils
IF    (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_executable(ssa-resize          ${MODULES_SRC} "ssa.c" "ssa-resize.c")
add_executable(ssa-resources       ${MODULES_SRC} "ssa.c" "ssa-resources.c")
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

add_executable(ssa-retime          ${MODULES_SRC} "ssa.c" "ssa-retime.c")

#tests
IF    (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_executable(test_parse_ssa      ${MODULES_SRC} "ssa.c"      "test_parse_ssa.c")
add_executable(test_parse_srt      ${MODULES_SRC} "srt.c"      "test_parse_srt.c")
add_executable(test_parse_microsub ${MODULES_SRC} "microsub.c" "test_parse_microsub.c")
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

target_link_libraries(srt2ssa             ${BUILD_LIBS})
target_link_libraries(microsub2ssa        ${BUILD_LIBS})
target_link_libraries(ssa-retime          ${BUILD_LIBS})

IF    (CMAKE_BUILD_TYPE STREQUAL "Debug")
target_link_libraries(ssa-resize          ${BUILD_LIBS})
target_link_libraries(ssa-resources       ${BUILD_LIBS})
target_link_libraries(test_parse_ssa      ${BUILD_LIBS})
target_link_libraries(test_parse_srt      ${BUILD_LIBS})
target_link_libraries(test_parse_microsub ${BUILD_LIBS})
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

#add_test(test_parse_srt ${MODULES_SRC} test_parse_srt.c)

install(TARGETS srt2ssa microsub2ssa ssa-retime
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
