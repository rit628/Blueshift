#pragma once
#include "ast.hpp"
#include "bls_types.hpp"
#include "libHD/HeapDescriptors.hpp"
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

            static BlsType createDevtype(std::unordered_map<std::string, BlsType>&& attributes) {
                auto devtype = std::make_shared<MapDescriptor>(Desctype::ANY);
                for (auto&& [attribute, value] : attributes) {
                    auto attr = BlsType(attribute);
                    devtype->emplace(attr, value);
                }
                return devtype;
            }

        private:
            std::unique_ptr<AstNode> ast;
            Compiler compiler;
    };

}