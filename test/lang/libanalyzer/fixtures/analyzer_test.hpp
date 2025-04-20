#pragma once
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "analyzer.hpp"
#include "ast.hpp"
#include "call_stack.hpp"

namespace BlsLang {
    class AnalyzerTest : public testing::Test {
        public:
            struct Metadata {
                Metadata() { };
                std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
                std::vector<OBlockDesc> oblockDescriptors;
                std::unordered_map<BlsType, uint8_t> literalPool = {
                    {std::monostate(), 0},  // for void return values
                    {1, 1}  // for increment & decrement expressions
                };
                uint8_t literalCount = literalPool.size();
            };

            void TEST_ANALYZE(std::unique_ptr<AstNode>& ast, Metadata&& metadata = Metadata(), CallStack<std::string>&& cs = CallStack<std::string>()) {
                analyzer.cs = cs;
                ast->accept(analyzer);
                EXPECT_EQ(analyzer.deviceDescriptors, metadata.deviceDescriptors);
                EXPECT_EQ(analyzer.oblockDescriptors, metadata.oblockDescriptors);
                EXPECT_EQ(analyzer.literalPool, metadata.literalPool);
            }

        private:
            Analyzer analyzer;
    };
}