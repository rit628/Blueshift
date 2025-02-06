#include "TSQ.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>


class TSQTest : public ::testing::Test 
{
protected:
    TSQ<int> tsq;
    TSQ<string> tsq_string;
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
        this_thread::sleep_for(chrono::milliseconds(100));
        sharedQueue.write(99);
    });

    thread reader([&sharedQueue]() 
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
        auto start = chrono::high_resolution_clock::now();
        result = queue.read();  // This should block until write() is called
        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double> duration = end - start;
        readCompleted = true;

        // Verify that read() took at least 100ms to unblock
        EXPECT_GE(duration.count(), 0.1);
    });

    // Sleep for 100ms to simulate delay before writing data
    this_thread::sleep_for(chrono::milliseconds(100));
    queue.write(42);  // Now reader should unblock

    reader.join();
    
    // Verify that read() actually retrieved the written data
    EXPECT_EQ(result, 42);
    EXPECT_TRUE(readCompleted);  // Ensure read was unblocked
}

TEST_F(TSQTest, clearQueue_Test_String) 
{
    tsq_string.write("Colin");
    tsq_string.write("Rohan");
    tsq_string.clearQueue();
    EXPECT_TRUE(tsq_string.isEmpty());
    EXPECT_EQ(tsq_string.getSize(), 0);
}

TEST_F(TSQTest, getSize_Test_String)
{
    tsq_string.write("Colin");
    tsq_string.write("Rohan");
    tsq_string.write("Thomas");
    EXPECT_EQ(tsq_string.getSize(), 3);
}

TEST_F(TSQTest, Write_Test_String) 
{
    tsq_string.write("Colin");
    EXPECT_FALSE(tsq_string.isEmpty());
    EXPECT_EQ(tsq_string.getSize(), 1);
}

TEST_F(TSQTest, Read_Test_String) 
{
    tsq_string.write("Colin");
    EXPECT_EQ(tsq_string.read(), "Colin");
    EXPECT_EQ(tsq_string.getSize(), 0);
}

TEST_F(TSQTest, MultiThreaded_WriteAndRead_String) 
{
    TSQ<string> sharedQueue;
    
    std::thread writer([&sharedQueue]() 
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        sharedQueue.write("Colin");
    });

    thread reader([&sharedQueue]() 
    {
        string value = sharedQueue.read();
        EXPECT_EQ(value, "Colin");
    });

    writer.join();
    reader.join();
}

TEST_F(TSQTest, Read_Blocks_Until_Data_Available_String) 
{
    TSQ<string> queue;
    string result = " ";
    bool readCompleted = false;

    std::thread reader([&]() 
    {
        auto start = chrono::high_resolution_clock::now();
        result = queue.read();  // This should block until write() is called
        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double> duration = end - start;
        readCompleted = true;

        // Verify that read() took at least 100ms to unblock
        EXPECT_GE(duration.count(), 0.1);
    });

    // Sleep for 100ms to simulate delay before writing data
    this_thread::sleep_for(chrono::milliseconds(100));
    queue.write("Colin");  // Now reader should unblock

    reader.join();
    
    // Verify that read() actually retrieved the written data
    EXPECT_EQ(result, "Colin");
    EXPECT_TRUE(readCompleted);  // Ensure read was unblocked
}