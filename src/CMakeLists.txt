file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

include(CheckCCompilerFlag)
find_package(Threads REQUIRED)

set(TARGETS_TO_BUILD "")

if(BUILD_LIB_SO)
    add_library(cgnrc_shared SHARED ${SOURCE_FILES})
    set_target_properties(cgnrc_shared PROPERTIES OUTPUT_NAME ${BUILD_LIB})
    list(APPEND TARGETS_TO_BUILD cgnrc_shared)
endif()

if(BUILD_LIB_A)
    add_library(cgnrc_static STATIC ${SOURCE_FILES})
    set_target_properties(cgnrc_static PROPERTIES OUTPUT_NAME ${BUILD_LIB})
    list(APPEND TARGETS_TO_BUILD cgnrc_static)
endif()

set(COMMON_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wformat=2
    -Wformat-security
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wundef
    -Wwrite-strings
    -Wcast-align
    -Werror
    -Wcast-qual
    -Wjump-misses-init
    -Wfloat-equal
    -fno-common
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_SPECIFIC_OPTIONS
        -O0
        -g
        -gdwarf-4
    )
else() # Release
    set(BUILD_SPECIFIC_OPTIONS
        -O3
        -UDEBUG
        # Hardening
        -fstack-protector-strong
        -fstack-clash-protection
        -fcf-protection=full
        -ftrivial-auto-var-init=zero
        -D_FORTIFY_SOURCE=3
    )
endif()

set(EXTRA_COMPILE_OPTIONS "")
set(EXTRA_LINK_OPTIONS "")

if(SANITIZE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND EXTRA_COMPILE_OPTIONS
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    list(APPEND EXTRA_LINK_OPTIONS
        -fsanitize=address
        -fsanitize=undefined
    )
endif()

# Applied to shared lib only (not needed for static)
set(LINKER_SECURITY_OPTIONS
    -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,noexecstack
    -Wl,--no-undefined
)

foreach(lib ${TARGETS_TO_BUILD})

    target_compile_features(${lib} PRIVATE
        c_std_11
    )
    target_link_libraries(${lib} PUBLIC Threads::Threads)

    target_compile_definitions(${lib} PRIVATE
        _POSIX_C_SOURCE=200809L
        PROJECT_NAME="${PROJECT_NAME}"
        $<$<CONFIG:Debug>:DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        $<$<NOT:$<CONFIG:Debug>>:RELEASE>
    )

    target_include_directories(${lib} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CGNRC_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_compile_options(${lib} PRIVATE
        ${COMMON_COMPILE_OPTIONS}
        ${BUILD_SPECIFIC_OPTIONS}
        ${EXTRA_COMPILE_OPTIONS}
    )

    if(EXTRA_LINK_OPTIONS)
        target_link_options(${lib} PRIVATE ${EXTRA_LINK_OPTIONS})
    endif()

    if(${lib} STREQUAL "cgnrc_shared")
        # -fPIC only for shared lib
        set_target_properties(${lib} PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${_cgnrc_version_major}"
        )
        target_link_options(${lib} PRIVATE ${LINKER_SECURITY_OPTIONS})
    endif()

endforeach()

set(TARGETS_TO_BUILD "${TARGETS_TO_BUILD}" PARENT_SCOPE)

# ================
# BINARY STRIPPING
# ================
# Determine which lib target to strip (prefer shared, fallback to static)
if(BUILD_LIB_SO)
    set(LIB_TARGET cgnrc_shared)
elseif(BUILD_LIB_A)
    set(LIB_TARGET cgnrc_static)
endif()

find_program(OBJCOPY_PATH objcopy)
find_program(STRIP_PATH strip)

if(STRIP_PATH AND LIB_TARGET)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" lowercase_build_type)

    if(lowercase_build_type STREQUAL "debug")
        if(OBJCOPY_PATH)
            add_custom_target(strip-bin
                COMMAND ${OBJCOPY_PATH} --only-keep-debug $<TARGET_FILE:${LIB_TARGET}> $<TARGET_FILE:${LIB_TARGET}>.debug
                COMMAND ${STRIP_PATH} --strip-unneeded $<TARGET_FILE:${LIB_TARGET}>
                COMMAND ${OBJCOPY_PATH} --add-gnu-debuglink=$<TARGET_FILE:${LIB_TARGET}>.debug $<TARGET_FILE:${LIB_TARGET}>
                COMMENT "Extracting debug symbols and stripping unneeded from '${LIB_TARGET}' (Debug mode)..."
                DEPENDS ${LIB_TARGET}
            )
        else()
            message(WARNING "objcopy not found. Target 'strip-bin' will be unavailable in Debug mode.")
        endif()
    else()
        add_custom_target(strip-bin
            COMMAND ${STRIP_PATH} --strip-all $<TARGET_FILE:${LIB_TARGET}>
            COMMENT "Stripping '${LIB_TARGET}' completely with --strip-all (Release mode)..."
            DEPENDS ${LIB_TARGET}
        )
    endif()
else()
    if(NOT STRIP_PATH)
        message(WARNING "strip utility not found. Target 'strip-bin' will be unavailable.")
    endif()
endif()
