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
#include <variant>
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

            void TEST_E2E_TASK(const std::string& taskName, std::vector<BlsType>&& input, const std::vector<BlsType>&& expectedOutput, const std::string& expectedStdout) {
                vm.setTaskOffset(compiler.getTaskDescriptorMap().at(taskName).bytecode_offset);
                
                auto interpreterTransform = compiler.getTasks().at(taskName);
                auto vmTransform = std::bind(&VirtualMachine::transform, std::ref(vm), std::placeholders::_1);
                
                auto checkTaskOutput = [&expectedOutput, &expectedStdout](std::function<std::vector<BlsType>(std::vector<BlsType>)> transformFunction, std::vector<BlsType> input) {
                    std::vector<BlsType> clonedInput;
                    for (auto&& arg : input) {
                        if (std::holds_alternative<std::shared_ptr<HeapDescriptor>>(arg)) {
                            clonedInput.push_back(std::get<std::shared_ptr<HeapDescriptor>>(arg)->clone());
                        }
                        else {
                            clonedInput.push_back(arg);
                        }
                    }
                    std::stringstream stdoutCapture;
                    auto oldBuffer = std::cout.rdbuf();
                    std::cout.rdbuf(stdoutCapture.rdbuf());
                    auto output = transformFunction(clonedInput);
                    EXPECT_EQ(output.size(), expectedOutput.size());
                    for (int i = 0; i < output.size(); i++) {
                        EXPECT_EQ(output.at(i), expectedOutput.at(i));
                    }
                    EXPECT_EQ(stdoutCapture.str(), expectedStdout);
                    std::cout.rdbuf(oldBuffer);
                };

                // checkTaskOutput(interpreterTransform, input);
                checkTaskOutput(vmTransform, input);
            }
        

        private:
            std::unique_ptr<AstNode> ast;
            std::stringstream bytecode;
            Compiler compiler;
            VirtualMachine vm;
    };

}