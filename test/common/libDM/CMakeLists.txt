file(GLOB_RECURSE LIBDM_TESTS *.cpp)
add_executable(test_libDM ${LIBDM_TESTS})
target_link_libraries(test_libDM dynamic_message GTest::gtest_main)
gtest_discover_tests(test_libDM)