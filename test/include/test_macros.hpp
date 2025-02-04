#pragma once
#ifdef NO_GROUP_TESTS
    #define GROUP_TEST_F(TestBase, GroupName, TestName) \
        TEST_F(TestBase, TestName)
#else
    #define GROUP_TEST_F(TestBase, GroupName, TestName) \
        TEST_F(TestBase, GroupName##_##TestName)
#endif
