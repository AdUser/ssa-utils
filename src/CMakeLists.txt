set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(MODULES_SRC "common.c")

# converters
add_executable(srt2ssa             ${MODULES_SRC} "ssa.c" "srt.c" "srt2ssa.c")
add_executable(microsub2ssa        ${MODULES_SRC} "ssa.c" "microsub.c" "microsub2ssa.c")

# various utils
add_executable(ssa_resize          ${MODULES_SRC} "ssa.c" "ssa_resize.c")

#tests
add_executable(test_parse_ssa      ${MODULES_SRC} "ssa.c"      "test_parse_ssa.c")
add_executable(test_parse_srt      ${MODULES_SRC} "srt.c"      "test_parse_srt.c")
add_executable(test_parse_microsub ${MODULES_SRC} "microsub.c" "test_parse_microsub.c")

target_link_libraries(srt2ssa             ${BUILD_LIBS})
target_link_libraries(microsub2ssa        ${BUILD_LIBS})
target_link_libraries(ssa_resize          ${BUILD_LIBS})
target_link_libraries(test_parse_ssa      ${BUILD_LIBS})
target_link_libraries(test_parse_srt      ${BUILD_LIBS})
target_link_libraries(test_parse_microsub ${BUILD_LIBS})

#add_test(test_parse_srt ${MODULES_SRC} test_parse_srt.c)
