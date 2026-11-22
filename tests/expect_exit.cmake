execute_process(COMMAND "${PROGRAM}" "${ARGUMENT}" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT "${result}" STREQUAL "${EXPECTED}")
    message(FATAL_ERROR "tarn-check ${ARGUMENT} exited ${result}, expected ${EXPECTED}\n${output}${errors}")
endif()
