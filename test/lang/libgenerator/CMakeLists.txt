file(GLOB_RECURSE LIBGENERATOR_TESTS *.cpp)
add_executable(test_libgenerator ${LIBGENERATOR_TESTS})
target_link_libraries(test_libgenerator generator lang_common GTest::gtest_main)
gtest_discover_tests(test_libgenerator)