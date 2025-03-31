#pragma once
#include "ast.hpp"
#include "libtypes/bls_types.hpp"
#include "libcompiler/compiler.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>

namespace BlsLang {

    class E2ETest : public testing::Test {
        public:
            void TEST_E2E_SOURCE(const std::string& fileName) {
                compiler.compileFile(fileName);
            }

            void TEST_E2E_OBLOCK(const std::string& oblockName, std::vector<BlsType>&& input, const std::vector<BlsType>&& expectedOutput, const std::string& expectedStdout) {
                auto oblock = compiler.getOblocks().at(oblockName);
                std::stringstream stdoutCapture;
                auto oldBuffer = std::cout.rdbuf();
	            std::cout.rdbuf(stdoutCapture.rdbuf());
                auto output = oblock(input);
                EXPECT_EQ(output.size(), expectedOutput.size());
                for (int i = 0; i < output.size(); i++) {
                    EXPECT_EQ(output.at(i), expectedOutput.at(i));
                }
                EXPECT_EQ(stdoutCapture.str(), expectedStdout);
                std::cout.rdbuf(oldBuffer);
            }

            template<typename T>
            static BlsType createDevtype(T states) {
                auto devtype = std::make_shared<MapDescriptor>(TYPE::ANY);
                using namespace TypeDef;
                #define DEVTYPE_BEGIN(name) \
                if constexpr (std::same_as<T, name>) { 
                #define ATTRIBUTE(name, ...) \
                    BlsType name##_key = #name; \
                    BlsType name##_val = states.name; \
                    devtype->emplace(name##_key, name##_val);
                #define DEVTYPE_END \
                }
                #include "DEVTYPES.LIST"
                #undef DEVTYPE_BEGIN
                #undef ATTRIBUTE
                #undef DEVTYPE_END
                return devtype;
            }

        private:
            std::unique_ptr<AstNode> ast;
            Compiler compiler;
    };

}