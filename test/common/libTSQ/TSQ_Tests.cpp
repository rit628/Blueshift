#include "TSQ.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>


class TSQTest : public ::testing::Test 
{
protected:
    TSQ<int> tsq;
};

TEST_F(TSQTest, clearQueue_Test) 
{
    tsq.write(60);
    tsq.write(70);
    tsq.clearQueue();
    EXPECT_TRUE(tsq.isEmpty());
    EXPECT_EQ(tsq.getSize(), 0);
}

TEST_F(TSQTest, getSize_Test)
{
    tsq.write(60);
    tsq.write(70);
    tsq.write(80);
    EXPECT_EQ(tsq.getSize(), 3);
}

TEST_F(TSQTest, Write_Test) 
{
    tsq.write(10);
    EXPECT_FALSE(tsq.isEmpty());
    EXPECT_EQ(tsq.getSize(), 1);
}

TEST_F(TSQTest, Read_Test) 
{
    tsq.write(20);
    EXPECT_EQ(tsq.read(), 20);
    EXPECT_EQ(tsq.getSize(), 0);
}

TEST_F(TSQTest, MultiThreaded_WriteAndRead) {
    TSQ<int> sharedQueue;
    
    std::thread writer([&sharedQueue]() 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        sharedQueue.write(99);
    });

    std::thread reader([&sharedQueue]() 
    {
        int value = sharedQueue.read();
        EXPECT_EQ(value, 99);
    });

    writer.join();
    reader.join();
}

TEST_F(TSQTest, Read_Blocks_Until_Data_Available) 
{
    TSQ<int> queue;
    int result = 0;
    bool readCompleted = false;

    std::thread reader([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        result = queue.read();  // This should block until write() is called
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        readCompleted = true;

        // Verify that read() took at least 100ms to unblock
        EXPECT_GE(duration.count(), 0.1);
    });

    // Sleep for 100ms to simulate delay before writing data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.write(42);  // Now reader should unblock

    reader.join();
    
    // Verify that read() actually retrieved the written data
    EXPECT_EQ(result, 42);
    EXPECT_TRUE(readCompleted);  // Ensure read was unblocked
}