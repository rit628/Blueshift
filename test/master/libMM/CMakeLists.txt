file(GLOB_RECURSE LIBMM_TESTS *.cpp)
add_executable(test_libMM ${LIBMM_TESTS})
target_link_libraries(test_libMM MM GTest::gtest_main)
gtest_discover_tests(test_libMM)