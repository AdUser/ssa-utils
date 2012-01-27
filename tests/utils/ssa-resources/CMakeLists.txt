
set(SRC_DIR "../../../src")
set(CMD "${SRC_DIR}/ssa-resources")
set(CMP "../../compare")

add_test(NAME "tests/utils/ssa-resources/01-1"
         COMMAND "${CMD}" "import" "-i" "01/src.ass" "-o" "01/test.ass" "-f" "01/test.ttf")
add_test(NAME "tests/utils/ssa-resources/01-2"
         COMMAND "${CMP}" "01/test.ass" "01/dst.ass")
