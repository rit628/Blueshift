#include "BytecodeReader.hpp"
#include "Preprocessor.hpp"
#include "VirtualMachine.hpp"
#include <gtest/gtest.h>

class VMTest : public ::testing::Test
{   
    public: 
        Preprocessor p;  
        
        
}; 

TEST_F(VMTest, BytecodeReader_Test)
{
    std::vector<Instruction> bytecode;
    std::string test_name = "./CodeSamples/test.blu"; 
    bytecode = Make_Bytecode(test_name); 

    EXPECT_EQ(bytecode.size(), 7); 
    
}

TEST_F(VMTest, VMInit){
    std::vector<Instruction> bytecode;
    std::string test_name = "./CodeSamples/test.blu"; 
    bytecode = Make_Bytecode(test_name); 

    VM vm("test", bytecode, 0); 

    EXPECT_EQ(bytecode.size(), 7); 
    
}