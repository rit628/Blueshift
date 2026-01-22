file(GLOB_RECURSE LIBEM_TESTS *.cpp)
add_executable(test_libEM ${LIBEM_TESTS})
target_link_libraries(test_libEM EM GTest::gtest_main)
gtest_discover_tests(test_libEM)