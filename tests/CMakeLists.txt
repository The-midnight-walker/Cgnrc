if(BUILD_TESTING)
    function(add_cgnrc_test name)
        if(BUILD_LIB_A)
            add_executable(${name} ${name}.c)
            target_link_libraries(${name} PRIVATE cgnrc_static)
        elseif(BUILD_LIB_SO)
            add_executable(${name} ${name}.c)
            target_link_libraries(${name} PRIVATE cgnrc_shared)
        else()
            add_executable(${name}
                ${name}.c
                ${CMAKE_SOURCE_DIR}/src/debug.c
                ${CMAKE_SOURCE_DIR}/src/module.c
                ${CMAKE_SOURCE_DIR}/src/shell.c
            )
        endif()

        target_include_directories(${name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include/cgnrc
        )

        target_compile_features(${name} PRIVATE c_std_11)
        target_compile_definitions(${name} PRIVATE _POSIX_C_SOURCE=200809L DEBUG)
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror -UNDEBUG)

        if(COVERAGE AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${name} PRIVATE --coverage)
            target_link_options(${name} PRIVATE --coverage)
        endif()

        if(SANITIZE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${name} PRIVATE -fsanitize=address,undefined)
            target_link_options(${name} PRIVATE -fsanitize=address,undefined)
        endif()

        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_cgnrc_test(test01)

    add_executable(test_header_self_contained header_self_contained.c)
    target_include_directories(test_header_self_contained PRIVATE
        ${CMAKE_SOURCE_DIR}/include/cgnrc
    )
    target_compile_features(test_header_self_contained PRIVATE c_std_11)
    target_compile_options(test_header_self_contained PRIVATE
        -Wall -Wextra -Werror -UNDEBUG
    )
    add_test(NAME test_header_self_contained COMMAND test_header_self_contained)

endif()
