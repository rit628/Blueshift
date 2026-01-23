function(bls_add_library library_name library_type)
    set(multiValueArgs LINKS INCLUDES)
    cmake_parse_arguments(PARSE_ARGV 2 arg_bls_add_library
        "" "" "${multiValueArgs}"
    )

    set(arg_bls_add_library_SOURCES ${arg_bls_add_library_UNPARSED_ARGUMENTS})
    list(APPEND arg_bls_add_library_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
    set(library_scope "INTERFACE")
    
    if(NOT (${library_type} STREQUAL "INTERFACE"))
        set(library_scope "PUBLIC")
        file(GLOB_RECURSE LIBRARY_SRC *.cpp)
        list(APPEND arg_bls_add_library_SOURCES ${LIBRARY_SRC})
    endif()

    add_library(${library_name} ${library_type} ${LIBRARY_SRC} ${arg_bls_add_library_SOURCES})
    target_include_directories(${library_name} ${library_scope} ${arg_bls_add_library_INCLUDES})
    target_link_libraries(${library_name} ${library_scope} ${arg_bls_add_library_LINKS})
endfunction()

function(bls_add_executable executable_name)
    set(multiValueArgs LINKS)
    cmake_parse_arguments(PARSE_ARGV 1 arg_bls_add_executable
        "" "" "${multiValueArgs}"
    )

    set(arg_bls_add_executable_SOURCES ${arg_bls_add_executable_UNPARSED_ARGUMENTS})
    list(LENGTH arg_bls_add_executable_SOURCES SOURCE_COUNT)
    if(SOURCE_COUNT EQUAL 0)
        file(GLOB_RECURSE EXECUTABLE_SRC *.cpp)
        set(arg_bls_add_executable_SOURCES ${EXECUTABLE_SRC})
    endif()
    
    add_executable(${executable_name} ${arg_bls_add_executable_SOURCES})
    target_link_libraries(${executable_name} ${arg_bls_add_executable_LINKS})
endfunction()

function(bls_add_test test_name)
    set(multiValueArgs LINKS)
    cmake_parse_arguments(PARSE_ARGV 1 arg_bls_add_test
        "" "" "${multiValueArgs}"
    )

    set(test_name test_${test_name})
    list(APPEND arg_bls_add_test_LINKS GTest::gtest_main)
    bls_add_executable(${test_name} LINKS ${arg_bls_add_test_LINKS})
    gtest_discover_tests(${test_name})
endfunction()