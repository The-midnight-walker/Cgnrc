# ==================
# FORMATTING & STYLE
# ==================
file(GLOB_RECURSE FORMAT_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/*.c"
    "${CMAKE_SOURCE_DIR}/*.h"
)

list(FILTER FORMAT_FILES EXCLUDE REGEX "^${CMAKE_SOURCE_DIR}/build/")

find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-16 clang-format-15)

if(CLANG_FORMAT_BIN)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_BIN} -i -style=file ${FORMAT_FILES}
        COMMENT "Formatting code with clang-format..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    message(WARNING "clang-format not found. Target 'format' unavailable.")
endif()

# =============
# DOCUMENTATION
# =============
find_package(Doxygen REQUIRED)

set(CGNRC_DOC_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")
configure_file(
    "${CMAKE_SOURCE_DIR}/Doxyfile"
    "${CMAKE_BINARY_DIR}/Doxyfile"
    @ONLY
)

# Keep the checked-in Doxyfile usable as a plain Doxygen config. Force the
# build-specific input and output paths in the generated copy, regardless of
# whether the source Doxyfile was regenerated or locally customized.
file(APPEND "${CMAKE_BINARY_DIR}/Doxyfile"
    "\n# CMake-managed project settings\n"
    "PROJECT_NAME = \"cgnrc\"\n"
    "PROJECT_NUMBER = \"${CGNRC_VERSION_FULL}\"\n"
    "OUTPUT_DIRECTORY = \"${CGNRC_DOC_OUTPUT_DIR}\"\n"
    "INPUT = \"${CMAKE_SOURCE_DIR}/include\" \"${CMAKE_SOURCE_DIR}/src\" \"${CMAKE_SOURCE_DIR}/README.md\"\n"
    "RECURSIVE = YES\n"
    "USE_MDFILE_AS_MAINPAGE = \"${CMAKE_SOURCE_DIR}/README.md\"\n"
    "OPTIMIZE_OUTPUT_FOR_C = YES\n"
    "GENERATE_LATEX = NO\n"
    "HAVE_DOT = NO\n"
)

add_custom_target(doc
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CGNRC_DOC_OUTPUT_DIR}"
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating documentation with Doxygen..."
    VERBATIM
)
