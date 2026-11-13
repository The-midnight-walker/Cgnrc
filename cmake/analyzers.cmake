# ====================
# sa - Static Analysis
# ====================
find_program(CPPCHECK_BIN cppcheck)
if(CPPCHECK_BIN)
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_BIN}
            --enable=all
            --std=c11
            --suppress=missingIncludeSystem
            --suppress=unreadVariable
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/include
        COMMENT "Running Cppcheck static analysis..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
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
            ${CLANG_TIDY_SOURCES}
        COMMENT "Running clang-tidy static analysis..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    message(WARNING "clang-tidy not found. Target 'clang-tidy' will be unavailable.")
endif()

# =====================
# da - Dynamic Analysis
# =====================
find_program(LCOV_BIN lcov)
find_program(GENHTML_BIN genhtml)

if(LCOV_BIN AND GENHTML_BIN)
    add_custom_target(coverage
        COMMAND ${LCOV_BIN} --directory . --zerocounters
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${LCOV_BIN} --directory . --capture --output-file coverage.info
        COMMAND ${LCOV_BIN} --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
        COMMAND ${GENHTML_BIN} coverage.info --output-directory coverage_html
        COMMENT "Generating HTML code coverage report with lcov..."
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
else()
    message(WARNING "lcov or genhtml not found. Target 'coverage' will be unavailable.")
endif()

find_program(VALGRIND_BIN valgrind)
if(VALGRIND_BIN AND TARGET test01)
    add_custom_target(da
        COMMAND ${VALGRIND_BIN}
            --leak-check=full
            --show-leak-kinds=all
            --track-origins=yes
            --error-exitcode=1
            $<TARGET_FILE:test01>
        COMMENT "Running dynamic analysis with Valgrind on unit tests..."
        DEPENDS test01
    )
else()
    add_custom_target(da
        COMMAND ${CMAKE_COMMAND} -E echo "WARNING: Valgrind not found or test01 target unavailable."
    )
endif()
