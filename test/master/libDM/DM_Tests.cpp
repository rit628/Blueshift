#include "DynamicMessage.hpp"
#include <gtest/gtest.h>

class DMTest : public ::testing::Test
{
    protected: 
        DynamicMessage dm;
}; 

TEST_F(DMTest, CreateField_Test)
{
    int test = 10; 
    dm.createField("test", test); 
    int i;
    dm.unpack("test", i);
    EXPECT_EQ(i, 10); 
}