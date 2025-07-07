#pragma once
#include "ast.hpp"
#include "libtype/bls_types.hpp"
#include "libcompiler/compiler.hpp"
#include "libvirtual_machine/virtual_machine.hpp"
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <filesystem>

namespace BlsLang {

    class E2ETest : public testing::Test {
        public:
            void TEST_E2E_SOURCE(const std::string& fileName) {
                compiler.compileFile(std::filesystem::path(E2E_LANG_SAMPLE_DIR) / fileName, bytecode);
                vm.loadBytecode(bytecode);
            }

            void TEST_E2E_OBLOCK(const std::string& oblockName, std::vector<BlsType>&& input, const std::vector<BlsType>&& expectedOutput, const std::string& expectedStdout) {
                vm.setOblockOffset(compiler.getOblockDescriptorMap().at(oblockName).bytecode_offset);
                
                auto interpreterTransform = compiler.getOblocks().at(oblockName);
                auto vmTransform = std::bind(&VirtualMachine::transform, std::ref(vm), std::placeholders::_1);
                
                auto checkOblockOutput = [&expectedOutput, &expectedStdout](std::function<std::vector<BlsType>(std::vector<BlsType>)> transformFunction, std::vector<BlsType> input) {
                    std::stringstream stdoutCapture;
                    auto oldBuffer = std::cout.rdbuf();
                    std::cout.rdbuf(stdoutCapture.rdbuf());
                    auto output = transformFunction(input);
                    EXPECT_EQ(output.size(), expectedOutput.size());
                    for (int i = 0; i < output.size(); i++) {
                        EXPECT_EQ(output.at(i), expectedOutput.at(i));
                    }
                    EXPECT_EQ(stdoutCapture.str(), expectedStdout);
                    std::cout.rdbuf(oldBuffer);
                };

                checkOblockOutput(interpreterTransform, input);
                checkOblockOutput(vmTransform, input);
            }
        

        private:
            std::unique_ptr<AstNode> ast;
            std::stringstream bytecode;
            Compiler compiler;
            VirtualMachine vm;
    };

}