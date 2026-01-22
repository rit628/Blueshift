#pragma once
#include "analyzer.hpp"
#include "ast.hpp"
#include "call_stack.hpp"
#include "Serialization.hpp"
#include "test_visitor.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <boost/range/combine.hpp>

namespace BlsLang {
    class AnalyzerTest : public testing::Test {
        public:
            struct Metadata {
                Metadata() { };
                std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
                std::unordered_map<std::string, TaskDescriptor> taskDescriptors;
                std::unordered_map<BlsType, uint8_t> literalPool;
            };

            void TEST_ANALYZE(std::unique_ptr<AstNode>& ast, std::unique_ptr<AstNode>& decoratedAst = defaultAst, Metadata metadata = Metadata(), CallStack<std::string> cs = CallStack<std::string>(CallStack<std::string>::Frame::Context::FUNCTION)) {
                analyzer.cs = cs;
                ast->accept(analyzer);
                static auto compareDeviceDescriptors = [](const DeviceDescriptor& desc, const DeviceDescriptor& expectedDesc) {
                    EXPECT_EQ(desc.device_name, expectedDesc.device_name);
                    EXPECT_EQ(desc.type, expectedDesc.type);
                    EXPECT_EQ(desc.controller, expectedDesc.controller);
                    EXPECT_EQ(desc.port_maps, expectedDesc.port_maps);
                    EXPECT_EQ(desc.initialValue, expectedDesc.initialValue);
                    EXPECT_EQ(desc.isVtype, expectedDesc.isVtype);
                    EXPECT_EQ(desc.readPolicy, expectedDesc.readPolicy);
                    EXPECT_EQ(desc.overwritePolicy, expectedDesc.overwritePolicy);
                    EXPECT_EQ(desc.isYield, expectedDesc.isYield);
                    EXPECT_EQ(desc.polling_period, expectedDesc.polling_period);
                    EXPECT_EQ(desc.isConst, expectedDesc.isConst);
                    EXPECT_EQ(desc.deviceKind, expectedDesc.deviceKind);
                };

                static auto compareTaskDescriptors = [](const TaskDescriptor& desc, const TaskDescriptor& expectedDesc) {
                    EXPECT_EQ(desc.name, expectedDesc.name);
                    for (auto&& [devDesc, expectedDevDesc] : boost::combine(desc.binded_devices, expectedDesc.binded_devices)) {
                        compareDeviceDescriptors(devDesc, expectedDevDesc);
                    }
                    EXPECT_EQ(desc.bytecode_offset, expectedDesc.bytecode_offset);
                    for (auto&& [devDesc, expectedDevDesc] : boost::combine(desc.inDevices, expectedDesc.inDevices)) {
                        compareDeviceDescriptors(devDesc, expectedDevDesc);
                    }
                    for (auto&& [devDesc, expectedDevDesc] : boost::combine(desc.outDevices, expectedDesc.outDevices)) {
                        compareDeviceDescriptors(devDesc, expectedDevDesc);
                    }
                    EXPECT_EQ(desc.hostController, expectedDesc.hostController);
                    EXPECT_EQ(desc.triggers, expectedDesc.triggers);
                };

                auto& deviceDescriptors = analyzer.deviceDescriptors;
                auto& expectedDeviceDescriptors = metadata.deviceDescriptors;
                for (auto&& [expectedName, expectedDesc] : expectedDeviceDescriptors) {
                    ASSERT_TRUE(deviceDescriptors.contains(expectedName));
                    compareDeviceDescriptors(deviceDescriptors.at(expectedName), expectedDesc);
                }

                auto& taskDescriptors = analyzer.taskDescriptors;
                auto& expectedTaskDescriptors = metadata.taskDescriptors;
                for (auto&& [expectedName, expectedDesc] : expectedTaskDescriptors) {
                    ASSERT_TRUE(taskDescriptors.contains(expectedName));
                    compareTaskDescriptors(taskDescriptors.at(expectedName), expectedDesc);
                }

                ASSERT_EQ(analyzer.literalPool.size(), metadata.literalPool.size());
                EXPECT_EQ(analyzer.literalPool, metadata.literalPool);
                if (decoratedAst != defaultAst) {
                    ASSERT_NE(ast, nullptr);
                    ASSERT_NE(decoratedAst, nullptr);
                    Tester tester(std::move(decoratedAst));
                    ast->accept(tester);
                }
            }

        private:
            Analyzer analyzer;
            static std::unique_ptr<AstNode> defaultAst;
    };

    inline std::unique_ptr<AstNode> AnalyzerTest::defaultAst = nullptr;
}