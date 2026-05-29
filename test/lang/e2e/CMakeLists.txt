bls_add_test(lang LINKS compiler virtual_machine visitor GTest::gtest_main)
target_compile_options(test_lang PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/samples -Wno-c23-extensions)
# For now this workaround is required until ccache properly supports #embed
file(GLOB_RECURSE SAMPLE_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/samples/*)
if(WIN32)
    list(JOIN SAMPLE_TEST_FILES ";" CCACHE_EXTRAFILES)
else()
    list(JOIN SAMPLE_TEST_FILES ":" CCACHE_EXTRAFILES)
endif()
set_target_properties(test_lang PROPERTIES
    C_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;CCACHE_EXTRAFILES=${CCACHE_EXTRAFILES}"
    CXX_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;CCACHE_EXTRAFILES=${CCACHE_EXTRAFILES}"
)