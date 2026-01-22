file(GLOB_RECURSE LIBLEXER_TESTS *.cpp)
add_executable(test_liblexer ${LIBLEXER_TESTS})
target_link_libraries(test_liblexer lexer lang_common GTest::gtest_main)
gtest_discover_tests(test_liblexer)