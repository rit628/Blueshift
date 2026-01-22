file(GLOB_RECURSE LIBANALYZER_TESTS *.cpp)
add_executable(test_libanalyzer ${LIBANALYZER_TESTS})
target_link_libraries(test_libanalyzer analyzer lang_common GTest::gtest_main)
gtest_discover_tests(test_libanalyzer)