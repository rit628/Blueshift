#include "TSM.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

//Test that insertion and retrieval work as expected
TEST(TSMTest, InsertAndRetrieve) 
{
    TSM<int, std::string> tsm;
    tsm.insert(1, "one");
    tsm.insert(2, "two");

    auto result1 = tsm.get(1);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), "one");

    auto result2 = tsm.get(2);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "two");

    // Test retrieval of a non-existent key
    auto result3 = tsm.get(3);
    EXPECT_FALSE(result3.has_value());
}

// Test that updating a key works correctly
TEST(TSMTest, UpdateValue) 
{
    TSM<int, std::string> tsm;
    tsm.insert(1, "one");
    auto result1 = tsm.get(1);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), "one");

    // Update value
    tsm.insert(1, "uno");
    auto result2 = tsm.get(1);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "uno");
}

// Test concurrent reads and writes
TEST(TSMTest, Concurrency) 
{
    TSM<int, int> tsm;
    const int numThreads = 10;
    const int numIterations = 1000;
    
    // Writer threads: insert/update keys
    auto writer = [&tsm, numIterations](int start) {
        for (int i = 0; i < numIterations; ++i) {
            tsm.insert(start + i, i);
        }
    };

    // Reader threads: read keys
    auto reader = [&tsm, numIterations](int start) {
        for (int i = 0; i < numIterations; ++i) {
            auto res = tsm.get(start + i);
            // Since writes might not have occurred yet, we only check that if a value exists, it's as expected.
            if (res.has_value()) {
                EXPECT_EQ(res.value(), i);
            }
        }
    };

    std::vector<std::thread> threads;
    
    // Launch writer threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(writer, i * numIterations);
    }
    
    // Launch reader threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(reader, i * numIterations);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
}