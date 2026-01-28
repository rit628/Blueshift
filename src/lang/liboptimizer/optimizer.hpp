#pragma once
#include "EM.hpp"
#include "Serialization.hpp"
#include "bytecode_processor.hpp"
#include "opcodes.hpp"
#include "bls_types.hpp"
#include <fstream>
#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set> 
#include <boost/archive/binary_iarchive.hpp>


namespace BlsLang{

    struct SymbolMetadata{
        std::string metadata; 
        std::set<std::string> deviceDependencies; 
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
            std::unordered_map<int64_t, std::string> taskClientMap; 
            std::unordered_map<int64_t, int64_t> executionMap; 

    }; 

}