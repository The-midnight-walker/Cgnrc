if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR OR
   NOT DEFINED GCOV_BIN OR NOT DEFINED CTEST_COMMAND)
    message(FATAL_ERROR "SOURCE_DIR, BINARY_DIR, GCOV_BIN, and CTEST_COMMAND are required")
endif()

set(COVERAGE_BUILD_DIR "${BINARY_DIR}/gcov")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${COVERAGE_BUILD_DIR}"
        -DBUILD_LIB_A=ON
        -DBUILD_LIB_SO=OFF
        -DBUILD_TESTING=ON
        -DSANITIZE=OFF
        -DCOVERAGE=ON
        -DCMAKE_BUILD_TYPE=Debug
    RESULT_VARIABLE CONFIGURE_RESULT
)
if(NOT CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure the gcov build")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${COVERAGE_BUILD_DIR}"
        --target test01 test_header_self_contained
    RESULT_VARIABLE BUILD_RESULT
)
if(NOT BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build tests for gcov")
endif()

execute_process(
    COMMAND "${CTEST_COMMAND}" --output-on-failure
    WORKING_DIRECTORY "${COVERAGE_BUILD_DIR}"
    RESULT_VARIABLE TEST_RESULT
)
if(NOT TEST_RESULT EQUAL 0)
    message(FATAL_ERROR "Tests failed in the gcov build")
endif()

file(GLOB COVERAGE_SOURCES "${SOURCE_DIR}/src/*.c")
foreach(SOURCE_FILE IN LISTS COVERAGE_SOURCES)
    get_filename_component(SOURCE_BASENAME "${SOURCE_FILE}" NAME)
    execute_process(
        COMMAND "${GCOV_BIN}" -b -o
            "${COVERAGE_BUILD_DIR}/src/CMakeFiles/cgnrc_static.dir/${SOURCE_BASENAME}.gcno"
            "${SOURCE_FILE}"
        WORKING_DIRECTORY "${COVERAGE_BUILD_DIR}"
        RESULT_VARIABLE GCOV_RESULT
    )
    if(NOT GCOV_RESULT EQUAL 0)
        message(FATAL_ERROR "gcov failed for ${SOURCE_FILE}")
    endif()
endforeach()
