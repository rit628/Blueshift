#pragma once
#include "EM.hpp"
#include "Serialization.hpp"
#include "bytecode_processor.hpp"
#include "call_stack.hpp"
#include "opcodes.hpp"
#include "bls_types.hpp"
#include <fstream>
#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <set> 
#include <boost/archive/binary_iarchive.hpp>


namespace BlsLang{
    
    enum class ORIGIN{
        LITERAL, 
        OPERAND, 
        LOCALS, 
    }; 


    // Refers to a single stack object
    struct PRMetadata{
        std::set<std::string> deviceDeps = {}; 
        std::set<std::string> controllerDeps = {}; 

        TYPE type; 
        ORIGIN og; 
        int index; 
    }; 

    PRMetadata combineDeps(PRMetadata& a, PRMetadata& b){
        PRMetadata pr; 
        
        pr.deviceDeps.insert(a.deviceDeps.begin(),a.deviceDeps.end());
        pr.deviceDeps.insert(b.deviceDeps.begin(), b.deviceDeps.end());

        pr.deviceDeps.insert(a.controllerDeps.begin(),a.controllerDeps.end());
        pr.deviceDeps.insert(b.controllerDeps.begin(), b.controllerDeps.end());

        return pr; 
    }


    // Refers to a information within a single stack view
    class MetadataFrame{
        public: 
            MetadataFrame(std::span<PRMetadata> &args); 
            void addLocal(PRMetadata &pm, int index); 
            void pushOperand(PRMetadata &pr); 
            PRMetadata popOperand(); 
            PRMetadata& getLocal(int index); 
            void setLocal(int index, PRMetadata& newPos); 


            int returnAddress; 
            std::vector<PRMetadata> locals; 
            std::stack<PRMetadata>  pr_stk; 

    }; 

    class MetadataStack{
        public: 
            std::stack<MetadataFrame> metadataStack; 
            void pushFrame(int ret, std::span<PRMetadata> args); 
            int popFrame(); 
            MetadataFrame& getCurrentFrame(); 
        private: 
    }; 


    class Optimizer : public BytecodeProcessor{
        public: 
            void optimize(); 
        
        // Override bytecode specific functions. 
        protected:
            #define OPCODE_BEGIN(code) \
            void code(
            #define ARGUMENT(arg, type) \
            type arg,
            #define OPCODE_END(...) \
            int = 0) override;
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END


        private:    
            // Literal List:
            std::vector<PRMetadata> literalList; 
            MetadataStack metadataStack; 
            

            std::unordered_map<int64_t, std::string> taskClientMap; 
            std::unordered_map<int64_t, int64_t> executionMap; 



            
    }; 

}