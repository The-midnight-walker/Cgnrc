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
find_package(Doxygen)

if(DOXYGEN_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen..."
        VERBATIM
    )
else()
    message(STATUS "Doxygen or Doxyfile not found. Target 'doc' unavailable.")
endif()
