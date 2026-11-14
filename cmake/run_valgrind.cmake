if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR OR NOT DEFINED VALGRIND_BIN)
    message(FATAL_ERROR "SOURCE_DIR, BINARY_DIR, and VALGRIND_BIN are required")
endif()

set(VALGRIND_BUILD_DIR "${BINARY_DIR}/valgrind")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${VALGRIND_BUILD_DIR}"
        -DBUILD_LIB_A=ON
        -DBUILD_LIB_SO=OFF
        -DBUILD_TESTING=ON
        -DSANITIZE=OFF
        -DCOVERAGE=OFF
        -DCMAKE_BUILD_TYPE=Debug
    RESULT_VARIABLE CONFIGURE_RESULT
)
if(NOT CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure the Valgrind build")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${VALGRIND_BUILD_DIR}" --target test01
    RESULT_VARIABLE BUILD_RESULT
)
if(NOT BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build tests for Valgrind")
endif()

execute_process(
    COMMAND "${VALGRIND_BIN}"
        --leak-check=full
        --show-leak-kinds=all
        --track-origins=yes
        --error-exitcode=1
        "${VALGRIND_BUILD_DIR}/tests/test01"
    WORKING_DIRECTORY "${VALGRIND_BUILD_DIR}"
    RESULT_VARIABLE VALGRIND_RESULT
)
if(NOT VALGRIND_RESULT EQUAL 0)
    message(FATAL_ERROR "Valgrind reported errors or the test executable failed")
endif()
