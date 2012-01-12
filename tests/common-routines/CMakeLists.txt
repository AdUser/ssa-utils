
set(SRC_DIR "../../src")

add_executable(stack stack.c "${SRC_DIR}/common.c")
target_link_libraries(stack             ${BUILD_LIBS})
add_test(tests/common/stack stack)

