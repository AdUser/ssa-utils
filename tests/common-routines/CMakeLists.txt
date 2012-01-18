
set(SRC_DIR "../../src")

add_executable(stack stack.c "${SRC_DIR}/common.c")
target_link_libraries(stack             ${BUILD_LIBS})
add_test(tests/common/stack stack)

add_executable(uue uue.c     "${SRC_DIR}/common.c")
target_link_libraries(uue               ${BUILD_LIBS})
add_test(tests/common/uue uue)
