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

    // Substask Metadata:
    struct SubtaskMetadata{
        // Controller (the id should be the offset of the task): 
        int id = 0; 
        std::string controller; 
        std::set<std::string> ctlDeps; 
        std::vector<size_t> code; 
        
        // Data: 
        std::vector<int> dependents; 
        std::unordered_map<size_t, size_t> literalOffset; 
        std::unordered_map<size_t, size_t> localOffset; 
        

    }; 


    // Refers to a single stack object
    struct TaskMetadata{

    
        std::pair<size_t, size_t> codeOffsets; 
        std::set<std::string> controllerDeps = {}; 
        std::string ctl = "UNKNOWN"; 
        int Task_ID = 0; 
        
        // Contains instruction, literal and local dependencies 
        std::set<size_t> instructionDeps; 
        
        // Type metadata
        TYPE type = static_cast<TYPE>(0); 
        ORIGIN og = static_cast<ORIGIN>(0); 
        int index = 0; 

        // Determines if the write is final: 
        bool complete = false; 
        bool junction = false; 

        // Omar
        std::vector<TaskMetadata> taskDeps; 

    }; 

    // Refers to a information within a single stack view
    class MetadataFrame{
        public: 
            MetadataFrame(std::span<TaskMetadata> &args); 
            void addLocal(TaskMetadata &pm, int index); 
            void pushOperand(TaskMetadata &Task); 
            TaskMetadata popOperand(); 
            TaskMetadata& topOperand(); 
            TaskMetadata& getLocal(int index); 
            void setLocal(int index, TaskMetadata& newPos); 


            int returnAddress; 
            std::vector<TaskMetadata> locals; 
            std::stack<TaskMetadata>  task_stk; 
            std::string hostingCtl; 

    }; 

    class MetadataStack{
        public: 
            std::stack<MetadataFrame> metadataStack; 
            void pushFrame(int ret, std::span<TaskMetadata> args); 
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
        
            // Loaded Subtask
            std::set<std::string> loadCtlDeps; 

            // Literal List:
            std::vector<TaskMetadata> literalList; 
            MetadataStack metadataStack; 
        
            // Contains the map from controller to subtask 
            std::vector<SubtaskMetadata> subtaskInfo; 
 
            TaskMetadata combineDeps(std::vector<TaskMetadata>&& taskSet); 
            void divideTree(int index); 
            void divideTreeH(TaskMetadata &tm); 

          
            
            



            
    }; 

}