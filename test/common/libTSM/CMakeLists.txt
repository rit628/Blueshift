file(GLOB_RECURSE LIBTSM_TESTS *.cpp)
add_executable(test_libTSM ${LIBTSM_TESTS})
target_link_libraries(test_libTSM TSM GTest::gtest_main)
gtest_discover_tests(test_libTSM)