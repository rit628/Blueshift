file(GLOB_RECURSE LIBTSQ_TESTS *.cpp)
add_executable(test_libTSQ ${LIBTSQ_TESTS})
target_link_libraries(test_libTSQ TSQ GTest::gtest_main)
gtest_discover_tests(test_libTSQ)