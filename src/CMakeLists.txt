file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

include(CheckCCompilerFlag)
find_package(Threads REQUIRED)

set(TARGETS_LIBS "")

if(BUILD_LIB_SO)
    add_library(cgnrc_shared SHARED ${SOURCE_FILES})
    set_target_properties(cgnrc_shared PROPERTIES OUTPUT_NAME cgnrc_shared)
    list(APPEND TARGETS_LIBS cgnrc_shared)
endif()
if(BUILD_LIB_A)
    add_library(cgnrc_static STATIC ${SOURCE_FILES})
    set_target_properties(cgnrc_static PROPERTIES OUTPUT_NAME cgnrc_static)
    list(APPEND TARGETS_LIBS cgnrc_static)
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
        -g0
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

foreach(lib ${TARGETS_LIBS})

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
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/cgnrc>
        $<BUILD_INTERFACE:${CGNRC_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_compile_options(${lib} PRIVATE
        ${COMMON_COMPILE_OPTIONS}
        ${BUILD_SPECIFIC_OPTIONS}
        ${EXTRA_COMPILE_OPTIONS}
    )

    if(COVERAGE AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${lib} PRIVATE --coverage)
        target_link_options(${lib} PRIVATE --coverage)
    endif()

    if(EXTRA_LINK_OPTIONS)
        target_link_options(${lib} PRIVATE ${EXTRA_LINK_OPTIONS})
    endif()

    if(${lib} STREQUAL "cgnrc_shared")
        # -fPIC only for shared lib
        set_target_properties(${lib} PROPERTIES
            POSITION_INDEPENDENT_CODE ON
        )
        target_link_options(${lib} PRIVATE ${LINKER_SECURITY_OPTIONS})
    endif()

endforeach()

set(TARGETS_LIBS "${TARGETS_LIBS}" PARENT_SCOPE)

# ================
# BINARY STRIPPING
# ================

find_program(OBJCOPY_BIN NAMES objcopy)
find_program(STRIP_BIN NAMES strip)

if(STRIP_BIN AND TARGETS_LIBS)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" lowercase_build_type)
    foreach(LIB_TARGET IN LISTS TARGETS_LIBS)
        if(lowercase_build_type STREQUAL "debug")
            if(OBJCOPY_BIN)
                add_custom_target(strip-${LIB_TARGET}
                    COMMAND ${OBJCOPY_BIN}
                        --only-keep-debug
                        $<TARGET_FILE:${LIB_TARGET}>
                        $<TARGET_FILE:${LIB_TARGET}>.debug
                    COMMAND ${STRIP_BIN}
                        --strip-unneeded
                        $<TARGET_FILE:${LIB_TARGET}>
                    COMMAND ${OBJCOPY_BIN}
                        --add-gnu-debuglink=$<TARGET_FILE:${LIB_TARGET}>.debug
                        $<TARGET_FILE:${LIB_TARGET}>
                    COMMENT "Extracting debug symbols and stripping ${LIB_TARGET}..."
                    DEPENDS ${LIB_TARGET}
                    VERBATIM
                )
            else()
                message(WARNING
                    "objcopy not found. Debug symbols cannot be extracted."
                )
            endif()
        else()
            get_target_property(LIB_TYPE "${LIB_TARGET}" TYPE)
            if(LIB_TYPE STREQUAL "STATIC_LIBRARY")
                set(STRIP_MODE --strip-debug)
            else()
                set(STRIP_MODE --strip-unneeded)
            endif()
            add_custom_target(strip-${LIB_TARGET}
                COMMAND ${STRIP_BIN}
                    ${STRIP_MODE}
                    $<TARGET_FILE:${LIB_TARGET}>

                COMMENT "Stripping ${LIB_TARGET} completely..."
                DEPENDS ${LIB_TARGET}
                VERBATIM
            )
        endif()
    endforeach()
    # Global target to strip all libraries (if any) in one go
    add_custom_target(strip-bin)
    foreach(LIB_TARGET IN LISTS TARGETS_LIBS)
        if(TARGET strip-${LIB_TARGET})
            add_dependencies(strip-bin strip-${LIB_TARGET})
        endif()
    endforeach()
elseif(NOT STRIP_BIN)
    message(WARNING "strip utility not found.")
endif()

