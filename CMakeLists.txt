cmake_minimum_required(VERSION 3.14)

set(_version_major "")
set(_version_patchlevel "")
set(_version_sublevel "")
set(_version_extra "")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _version_lines)
foreach(_line IN LISTS _version_lines)
    if(_line MATCHES "^[ \t]*VERSION[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(_version_major "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*PATCHLEVEL[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(_version_patchlevel "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*SUBLEVEL[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(_version_sublevel "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*EXTRAVERSION[ \t]*=[ \t]*(.*[^ \t])[ \t]*$")
        set(_version_extra "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*EXTRAVERSION[ \t]*=[ \t]*$")
        set(_version_extra "")
    endif()
endforeach()

if(_version_major STREQUAL "" OR
   _version_patchlevel STREQUAL "" OR
   _version_sublevel STREQUAL "")
    message(FATAL_ERROR
        "VERSION must define numeric VERSION, PATCHLEVEL, and SUBLEVEL values"
    )
endif()

set(_version
    "${_version_major}.${_version_patchlevel}.${_version_sublevel}"
)
set(CGNRC_VERSION_FULL "${_version}${_version_extra}")
project(cgnrc VERSION "${_version}" LANGUAGES C)

# Keep installation local to the build tree; this project installs no programs.
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/cgnrc" CACHE PATH
    "Private cgnrc install prefix" FORCE)

# ===================
# CMAKE CONFIGURATION
# ===================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SANITIZE "Enable sanitizers (only if build type is Debug)" ON)
option(COVERAGE "Enable gcov instrumentation for this build" OFF)

set(BUILD_LIB "cgnrc" CACHE STRING "Library name")
option(BUILD_LIB_SO "Build shared library" OFF)
option(BUILD_LIB_A  "Build static library" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Debug or Release" FORCE)
endif()

if(NOT BUILD_LIB_SO AND NOT BUILD_LIB_A)
    message(FATAL_ERROR
        "No library type selected.\n"
        "Enable BUILD_LIB_SO and/or BUILD_LIB_A"
    )
endif()

# ===============
# TARGETS & TESTS
# ===============
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE STRING
    "Installed public headers directory" FORCE)
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING
    "Installed library directory" FORCE)

set(CGNRC_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${CGNRC_GENERATED_INCLUDE_DIR}/cgnrc")
configure_file(
    cmake/version.h.in
    "${CGNRC_GENERATED_INCLUDE_DIR}/cgnrc/version.h"
    @ONLY
)

add_subdirectory(src)
add_subdirectory(tests)

# =======
# INSTALL
# =======
include(cmake/miscs.cmake)
include(cmake/analyzers.cmake)

find_program(CGNRC_STRIP_EXECUTABLE NAMES strip)
if(NOT CGNRC_STRIP_EXECUTABLE)
    message(FATAL_ERROR "The 'strip' utility is required to install cgnrc libraries")
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" _cgnrc_build_type_upper)
if(NOT _cgnrc_build_type_upper STREQUAL "RELEASE")
    set(_cgnrc_install_type_check
        "message(FATAL_ERROR \"Installation requires CMAKE_BUILD_TYPE=Release (configured: '${CMAKE_BUILD_TYPE}')\")"
    )
else()
    set(_cgnrc_install_type_check "")
endif()

install(CODE "
    ${_cgnrc_install_type_check}
    if(NOT \"\$<CONFIG>\" STREQUAL \"Release\")
        message(FATAL_ERROR \"Installation requires the Release configuration\")
    endif()
    message(STATUS \"Checking Release configuration and stripping cgnrc libraries before install\")
")

# Generate the documentation as part of installation, even when the caller
# invokes only the install target and has not built the optional doc target.
install(CODE "
    file(REMOVE_RECURSE \"${CGNRC_DOC_OUTPUT_DIR}\")
    file(REMOVE_RECURSE
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}\"
    )
    execute_process(
        COMMAND \"${DOXYGEN_EXECUTABLE}\" \"${CMAKE_BINARY_DIR}/Doxyfile\"
        WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}\"
        RESULT_VARIABLE _doxygen_result
        ERROR_VARIABLE _doxygen_error
    )
    if(NOT _doxygen_result EQUAL 0)
        message(FATAL_ERROR \"Doxygen documentation generation failed: \${_doxygen_error}\")
    endif()
    if(NOT EXISTS \"${CGNRC_DOC_OUTPUT_DIR}/html/index.html\")
        message(FATAL_ERROR \"Doxygen completed without creating ${CGNRC_DOC_OUTPUT_DIR}/html/index.html\")
    endif()
    message(STATUS \"Generated cgnrc documentation\")
")

# Remove directories from the previous package layout so reinstallations leave
# only the current include/cgnrc and lib tree in the private install prefix.
install(CODE "
    file(REMOVE_RECURSE
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/include/headers\"
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/libs\"
    )
")

foreach(_target IN LISTS TARGETS_LIBS)
    get_target_property(_target_type "${_target}" TYPE)
    if(_target_type STREQUAL "STATIC_LIBRARY")
        set(_strip_mode --strip-debug)
    elseif(_target_type STREQUAL "SHARED_LIBRARY")
        set(_strip_mode --strip-unneeded)
    else()
        message(FATAL_ERROR "Unsupported install target type for ${_target}: ${_target_type}")
    endif()

    install(CODE "
        set(_library \"\$<TARGET_FILE:${_target}>\")
        if(NOT EXISTS \"\${_library}\")
            message(FATAL_ERROR \"Library artifact does not exist: \${_library}\")
        endif()
        execute_process(
            COMMAND \"${CGNRC_STRIP_EXECUTABLE}\" ${_strip_mode} \"\${_library}\"
            RESULT_VARIABLE _strip_result
            ERROR_VARIABLE _strip_error
        )
        if(NOT _strip_result EQUAL 0)
            message(FATAL_ERROR \"Failed to strip ${_target}: \${_strip_error}\")
        endif()
        message(STATUS \"Stripped ${_target}\")
    ")
endforeach()

install(TARGETS ${TARGETS_LIBS}
    EXPORT cgnrcTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cgnrc"
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY "${CGNRC_DOC_OUTPUT_DIR}/html/"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
install(FILES "${CGNRC_GENERATED_INCLUDE_DIR}/cgnrc/version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cgnrc"
)
install(EXPORT cgnrcTargets
    FILE cgnrcTargets.cmake
    NAMESPACE cgnrc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cgnrc
)

configure_package_config_file(
    cmake/cgnrcConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/cgnrcConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cgnrc"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cgnrcConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cgnrcConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cgnrcConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cgnrc"
)
