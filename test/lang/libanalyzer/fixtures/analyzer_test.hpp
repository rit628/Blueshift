#pragma once
#include "analyzer.hpp"
#include "ast.hpp"
#include "call_stack.hpp"
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
                std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
                std::unordered_map<BlsType, uint8_t> literalPool;
            };

            void TEST_ANALYZE(std::unique_ptr<AstNode>& ast, std::unique_ptr<AstNode>& decoratedAst = defaultAst, Metadata metadata = Metadata(), CallStack<std::string> cs = CallStack<std::string>(CallStack<std::string>::Frame::Context::FUNCTION)) {
                analyzer.cs = cs;
                ast->accept(analyzer);
                EXPECT_EQ(analyzer.deviceDescriptors, metadata.deviceDescriptors);
                auto& oblockDescriptors = analyzer.oblockDescriptors;
                auto& expectedOblockDescriptors = metadata.oblockDescriptors;
                for (auto&& [data, expectedData] : boost::combine(oblockDescriptors, expectedOblockDescriptors)) {
                    EXPECT_EQ(data.first, expectedData.first);
                    auto& desc = data.second;
                    auto& expectedDesc = expectedData.second;
                    EXPECT_EQ(desc.name, expectedDesc.name);
                    EXPECT_EQ(desc.binded_devices, expectedDesc.binded_devices);
                    EXPECT_EQ(desc.bytecode_offset, expectedDesc.bytecode_offset);
                    EXPECT_EQ(desc.dropRead, expectedDesc.dropRead);
                    EXPECT_EQ(desc.dropWrite, expectedDesc.dropWrite);
                    EXPECT_EQ(desc.triggerRules, expectedDesc.triggerRules);
                    EXPECT_EQ(desc.synchronize_states, expectedDesc.synchronize_states);
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