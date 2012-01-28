set(SRC_DIR "../../../src")
set(SRC_FILES "${SRC_DIR}/common.c" "${SRC_DIR}/ssa.c")

#add_executable(fontname_flags   "${SRC_FILES}"  "fontname_flags.c")
add_executable(fontname_flags "${SRC_DIR}/common.c" "${SRC_DIR}/ssa.c" "fontname_flags.c")
target_link_libraries(fontname_flags ${BUILD_LIBS})
add_test(modules/ssa/fontname_flags fontname_flags)
