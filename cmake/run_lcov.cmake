if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR OR
   NOT DEFINED LCOV_BIN OR NOT DEFINED GENHTML_BIN OR
   NOT DEFINED CTEST_COMMAND)
    message(FATAL_ERROR
        "SOURCE_DIR, BINARY_DIR, LCOV_BIN, GENHTML_BIN, and CTEST_COMMAND are required"
    )
endif()

set(LCOV_BUILD_DIR "${BINARY_DIR}/lcov")
set(REPORT_DIR "${LCOV_BUILD_DIR}/coverage_html")
set(RAW_INFO "${LCOV_BUILD_DIR}/coverage.raw.info")
set(FILTERED_INFO "${LCOV_BUILD_DIR}/coverage.info")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${LCOV_BUILD_DIR}"
        -DBUILD_LIB_A=ON
        -DBUILD_LIB_SO=OFF
        -DBUILD_TESTING=ON
        -DSANITIZE=OFF
        -DCOVERAGE=ON
        -DCMAKE_BUILD_TYPE=Debug
    RESULT_VARIABLE CONFIGURE_RESULT
)
if(NOT CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure the lcov build")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${LCOV_BUILD_DIR}"
        --target test01 test_header_self_contained
    RESULT_VARIABLE BUILD_RESULT
)
if(NOT BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build tests for lcov")
endif()

file(REMOVE_RECURSE "${REPORT_DIR}")
file(REMOVE "${RAW_INFO}" "${FILTERED_INFO}")

execute_process(
    COMMAND "${CTEST_COMMAND}" --output-on-failure
    WORKING_DIRECTORY "${LCOV_BUILD_DIR}"
    RESULT_VARIABLE TEST_RESULT
)
if(NOT TEST_RESULT EQUAL 0)
    message(FATAL_ERROR "Tests failed in the lcov build")
endif()

execute_process(
    COMMAND "${LCOV_BIN}" --capture
        --directory "${LCOV_BUILD_DIR}"
        --output-file "${RAW_INFO}"
    WORKING_DIRECTORY "${LCOV_BUILD_DIR}"
    RESULT_VARIABLE CAPTURE_RESULT
)
if(NOT CAPTURE_RESULT EQUAL 0)
    message(FATAL_ERROR "lcov failed to capture coverage data")
endif()

execute_process(
    COMMAND "${LCOV_BIN}" --remove "${RAW_INFO}"
        "*/tests/*"
        --output-file "${FILTERED_INFO}"
    WORKING_DIRECTORY "${LCOV_BUILD_DIR}"
    RESULT_VARIABLE FILTER_RESULT
)
if(NOT FILTER_RESULT EQUAL 0)
    message(FATAL_ERROR "lcov failed to filter system and test sources")
endif()

execute_process(
    COMMAND "${GENHTML_BIN}" "${FILTERED_INFO}"
        --output-directory "${REPORT_DIR}"
    WORKING_DIRECTORY "${LCOV_BUILD_DIR}"
    RESULT_VARIABLE HTML_RESULT
)
if(NOT HTML_RESULT EQUAL 0)
    message(FATAL_ERROR "genhtml failed to generate the coverage report")
endif()

message(STATUS "lcov report generated: ${REPORT_DIR}/index.html")
