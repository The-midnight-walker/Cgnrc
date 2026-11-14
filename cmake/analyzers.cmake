# ====================
# sa - Static Analysis
# ====================
set(STATIC_ANALYSIS_TARGETS "")

find_program(CPPCHECK_BIN cppcheck)
if(CPPCHECK_BIN)
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_BIN}
            --enable=all
            --std=c11
            --inline-suppr
            -I ${CMAKE_SOURCE_DIR}/include
            --suppress=missingIncludeSystem
            --suppress=unusedFunction
            --suppress=staticFunction
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/include
        COMMENT "Running Cppcheck static analysis..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    list(APPEND STATIC_ANALYSIS_TARGETS cppcheck)
else()
    message(WARNING "cppcheck not found. Target 'cppcheck' will be unavailable.")
endif()

find_program(FLAWFINDER_BIN flawfinder)
if(FLAWFINDER_BIN)
    add_custom_target(flawfinder
        COMMAND ${FLAWFINDER_BIN}
            --quiet
            ${CMAKE_SOURCE_DIR}/src
        COMMENT "Running Flawfinder security analysis..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    list(APPEND STATIC_ANALYSIS_TARGETS flawfinder)
else()
    message(WARNING "flawfinder not found. Target 'flawfinder' will be unavailable.")
endif()

find_program(CLANG_TIDY_BIN clang-tidy)
if(CLANG_TIDY_BIN)
    file(GLOB_RECURSE CLANG_TIDY_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.c"
    )
    add_custom_target(clang-tidy
        COMMAND ${CLANG_TIDY_BIN}
            -p ${CMAKE_BINARY_DIR}
            # This checker flags bounded snprintf buffers and stream output
            # without distinguishing them from unsafe memory-buffer APIs.
            -checks=-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling
            ${CLANG_TIDY_SOURCES}
        COMMENT "Running clang-tidy static analysis..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    list(APPEND STATIC_ANALYSIS_TARGETS clang-tidy)
else()
    message(WARNING "clang-tidy not found. Target 'clang-tidy' will be unavailable.")
endif()

if(STATIC_ANALYSIS_TARGETS)
    add_custom_target(sa
        COMMAND ${CMAKE_COMMAND} -E echo "Running static analysis suite..."
        DEPENDS ${STATIC_ANALYSIS_TARGETS}
        VERBATIM
    )
else()
    add_custom_target(sa
        COMMAND ${CMAKE_COMMAND} -E echo
            "No static analysis tools found (cppcheck, flawfinder, clang-tidy)."
        VERBATIM
    )
endif()

# =====================
# da - Dynamic Analysis
# =====================
find_program(GCOV_BIN gcov)
if(GCOV_BIN AND CMAKE_C_COMPILER_ID STREQUAL "GNU" AND BUILD_TESTING)
    add_custom_target(gcov
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_BINARY_DIR}
            -DGCOV_BIN=${GCOV_BIN}
            -DCTEST_COMMAND=${CMAKE_CTEST_COMMAND}
            -P ${CMAKE_SOURCE_DIR}/cmake/run_gcov.cmake
        COMMENT "Running tests and generating gcov coverage..."
        USES_TERMINAL
        VERBATIM
    )
    set(GCOV_ANALYSIS_TARGET gcov)
else()
    add_custom_target(gcov
        COMMAND ${CMAKE_COMMAND} -E echo
            "gcov analysis requires GNU C, gcov, and BUILD_TESTING=ON."
        VERBATIM
    )
    set(GCOV_ANALYSIS_TARGET gcov)
endif()

find_program(LCOV_BIN lcov)
find_program(GENHTML_BIN genhtml)

if(LCOV_BIN AND GENHTML_BIN)
    add_custom_target(lcov
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_BINARY_DIR}
            -DLCOV_BIN=${LCOV_BIN}
            -DGENHTML_BIN=${GENHTML_BIN}
            -DCTEST_COMMAND=${CMAKE_CTEST_COMMAND}
            -P ${CMAKE_SOURCE_DIR}/cmake/run_lcov.cmake
        COMMENT "Running tests and generating the lcov HTML coverage report..."
        USES_TERMINAL
        VERBATIM
    )
else()
    add_custom_target(lcov
        COMMAND ${CMAKE_COMMAND} -E echo
            "lcov analysis requires lcov, genhtml, and BUILD_TESTING=ON."
        VERBATIM
    )
    message(WARNING "lcov or genhtml not found. Target 'lcov' will report unavailable.")
endif()

find_program(VALGRIND_BIN valgrind)
if(VALGRIND_BIN AND TARGET test01)
    add_custom_target(valgrind
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_BINARY_DIR}
            -DVALGRIND_BIN=${VALGRIND_BIN}
            -P ${CMAKE_SOURCE_DIR}/cmake/run_valgrind.cmake
        COMMENT "Running unit tests under Valgrind..."
        USES_TERMINAL
        VERBATIM
    )
else()
    add_custom_target(valgrind
        COMMAND ${CMAKE_COMMAND} -E echo
            "Valgrind or test01 is unavailable; skipping memory analysis."
        VERBATIM
    )
endif()

add_custom_target(da
    COMMAND ${CMAKE_COMMAND} -E echo "Running dynamic analysis suite..."
    DEPENDS valgrind lcov ${GCOV_ANALYSIS_TARGET}
    VERBATIM
)
