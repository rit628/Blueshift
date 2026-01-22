file(GLOB_RECURSE LIBPARSER_TESTS *.cpp)
add_executable(test_libparser ${LIBPARSER_TESTS})
target_link_libraries(test_libparser parser lang_common GTest::gtest_main)
gtest_discover_tests(test_libparser)