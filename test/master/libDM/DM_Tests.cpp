#include "DynamicMessage.hpp"
#include <gtest/gtest.h>

class DMTest : public ::testing::Test
{   
    public: 
        DynamicMessage dm;      
}; 

TEST_F(DMTest, startsWith_Test)
{
    std::string testString = "Hello World"; 
    std::string searchString = "Hello"; 
    EXPECT_TRUE(DynamicMessage::startsWith(searchString, testString)); 
}

TEST_F(DMTest, intStorage)
{
    int j = 10; 
    dm.createField("int_val", j); 

    int k = 0; 
    dm.unpack("int_val", k); 

    EXPECT_EQ(k, j);

}