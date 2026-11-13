cmake_minimum_required(VERSION 3.14)

set(_cgnrc_version_major "")
set(_cgnrc_version_patchlevel "")
set(_cgnrc_version_sublevel "")
set(_cgnrc_version_extra "")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _cgnrc_version_lines)
foreach(_line IN LISTS _cgnrc_version_lines)
    if(_line MATCHES "^[ \t]*VERSION[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(_cgnrc_version_major "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*PATCHLEVEL[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(_cgnrc_version_patchlevel "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*SUBLEVEL[ \t]*=[ \t]*([0-9]+)[ \t]*$")
        set(_cgnrc_version_sublevel "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*EXTRAVERSION[ \t]*=[ \t]*(.*[^ \t])[ \t]*$")
        set(_cgnrc_version_extra "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^[ \t]*EXTRAVERSION[ \t]*=[ \t]*$")
        set(_cgnrc_version_extra "")
    endif()
endforeach()

if(_cgnrc_version_major STREQUAL "" OR
   _cgnrc_version_patchlevel STREQUAL "" OR
   _cgnrc_version_sublevel STREQUAL "")
    message(FATAL_ERROR
        "VERSION must define numeric VERSION, PATCHLEVEL, and SUBLEVEL values"
    )
endif()

set(_cgnrc_numeric_version
    "${_cgnrc_version_major}.${_cgnrc_version_patchlevel}.${_cgnrc_version_sublevel}"
)
set(CGNRC_VERSION_FULL "${_cgnrc_numeric_version}${_cgnrc_version_extra}")
project(cgnrc VERSION "${_cgnrc_numeric_version}" LANGUAGES C)

# Clear project-scoped cache entries left by older builds named "kfgx".
unset(kfgx_BINARY_DIR CACHE)
unset(kfgx_SOURCE_DIR CACHE)
unset(kfgx_IS_TOP_LEVEL CACHE)

# ===================
# CMAKE CONFIGURATION
# ===================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SANITIZE "Enable sanitizers (only if build type is Debug)" ON)

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

# =====================
# TARGETS & TESTS
# =====================
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CGNRC_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${CGNRC_GENERATED_INCLUDE_DIR}/cgnrc")
configure_file(
    cmake/version.h.in
    "${CGNRC_GENERATED_INCLUDE_DIR}/cgnrc/version.h"
    @ONLY
)
add_subdirectory(src)
add_subdirectory(tests)

# =====================
# MODULES (after targets so analyzers can reference test targets)
# =====================
include(cmake/miscs.cmake)
include(cmake/analyzers.cmake)

install(TARGETS ${TARGETS_TO_BUILD}
    EXPORT cgnrcTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
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
