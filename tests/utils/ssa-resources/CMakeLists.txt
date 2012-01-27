set_property(DIRECTORY "." PROPERTY TEST_INCLUDE_FILE "tests_prepare.cmake")

set(CMD "${SRC_DIR}/ssa-resources")

add_test(NAME "utils/ssa-resources/01"
         COMMAND "${CMAKE_COMMAND}" "-E" "compare_files" "01/output.ass" "01/expected.ass")

add_test(NAME "utils/ssa-resources/02"
         COMMAND "${CMAKE_COMMAND}" "-E" "compare_files" "02/output.ass" "02/expected.ass")
