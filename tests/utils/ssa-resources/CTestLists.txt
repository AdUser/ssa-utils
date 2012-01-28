SET(SRC_DIR "../../../src")
SET(CMD "${SRC_DIR}/ssa-resources")

EXECUTE_PROCESS(COMMAND "${CMD}" "import" "-f" "testchunk_1.bin" "-i" "01/source.ass"
                OUTPUT_FILE "01/output.ass")

EXECUTE_PROCESS(COMMAND "${CMD}" "info" "-i" "02/source.ass"
                OUTPUT_FILE "02/output.txt")
