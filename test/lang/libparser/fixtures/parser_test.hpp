#pragma once
#include "visitor.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include <stack>

namespace BlsLang {
    class ParserTest : public testing::Test {
        public:
            class TestVisitor : public Visitor {
                public:
                    TestVisitor() = default;
                    TestVisitor(std::unique_ptr<AstNode> expectedAst) : expectedAst(std::move(expectedAst)) {}
                    void addExpectedAst(std::unique_ptr<AstNode> expectedAst) { expectedAst = std::move(expectedAst); }
                    #define AST_NODE_ABSTRACT(_)
                    #define AST_NODE(Node) \
                    std::any visit(Node& ast) override;
                    #include "include/NODE_TYPES.LIST"
                    #undef AST_NODE_ABSTRACT
                    #undef AST_NODE

                    std::unique_ptr<AstNode> expectedAst;
                    std::stack<AstNode*> expectedVisits;
            };


            void TEST_PARSE_SOURCE(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseSource();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_FUNCTION(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseFunction();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_STATEMENT(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseStatement();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_EXPRESSION(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseExpression();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_SPECIFIER(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseSpecifier();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void checkAst(std::unique_ptr<AstNode> ast, std::unique_ptr<AstNode> expectedAst) {
                ASSERT_NE(ast, nullptr);
                ASSERT_NE(expectedAst, nullptr);
                tv.addExpectedAst(std::move(expectedAst));
                ast->accept(tv);
            }

        private:
            Parser parser;
            TestVisitor tv;
    };

    inline std::any ParserTest::TestVisitor::visit(AstNode::Source& ast) {
        bool result = true;
        auto expectedSource = dynamic_cast<AstNode::Source*>(expectedAst.get());
        EXPECT_NE(expectedSource, nullptr);

        auto& expectedProcedures = expectedSource->getProcedures();
        auto& procedures = ast.getProcedures();
        EXPECT_EQ(expectedProcedures.size(), procedures.size());
        for (size_t i = 0; i < expectedProcedures.size(); i++) {
            expectedVisits.push(expectedProcedures.at(i).get());
            EXPECT_TRUE(procedures.at(i)->accept(*this));
            expectedVisits.pop();
        }

        auto& expectedOblocks = expectedSource->getOblocks();
        auto& oblocks = ast.getOblocks();
        EXPECT_EQ(expectedOblocks.size(), oblocks.size());
        for (size_t i = 0; i < expectedOblocks.size(); i++) {
            expectedVisits.push(expectedOblocks.at(i).get());
            EXPECT_TRUE(oblocks.at(i)->accept(*this));
            expectedVisits.pop();
        }
        
        auto& expectedSetup = expectedSource->getSetup();
        auto& setup = ast.getSetup();
        EXPECT_EQ(expectedSetup, setup);
        expectedVisits.push(expectedSetup.get());
        EXPECT_TRUE(setup->accept(*this));
        expectedVisits.pop();

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Function::Procedure& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedProcedure = dynamic_cast<AstNode::Function::Procedure*>(toCast);
        EXPECT_NE(expectedProcedure, nullptr);
        EXPECT_EQ(expectedProcedure->getName(), ast.getName());
        EXPECT_EQ(expectedProcedure->getParameters(), ast.getParameters());

        expectedVisits.push(expectedProcedure->getReturnType()->get());
        EXPECT_TRUE(ast.getReturnType()->get()->accept(*this));
        expectedVisits.pop();
        for (auto& statement : ast.getStatements()) {
            expectedVisits.push(statement.get());
            EXPECT_TRUE(statement->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Function::Oblock& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedProcedure = dynamic_cast<AstNode::Function::Oblock*>(toCast);
        EXPECT_NE(expectedProcedure, nullptr);

        EXPECT_EQ(expectedProcedure->getName(), ast.getName());
        EXPECT_EQ(expectedProcedure->getParameters(), ast.getParameters());

        for (auto& statement : ast.getStatements()) {
            expectedVisits.push(statement.get());
            EXPECT_TRUE(statement->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Setup& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedSetup = dynamic_cast<AstNode::Setup*>(toCast);
        EXPECT_NE(expectedSetup, nullptr);

        auto& expectedStatements = expectedSetup->getStatements();
        auto& statements = ast.getStatements();
        EXPECT_EQ(expectedStatements.size(), statements.size());
        for (size_t i = 0; i < expectedStatements.size(); i++) {
            expectedVisits.push(expectedStatements.at(i).get());
            EXPECT_TRUE(statements.at(i)->accept(*this));
            expectedVisits.pop();
        }
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::If& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedIf = dynamic_cast<AstNode::Statement::If*>(toCast);
        EXPECT_NE(expectedIf, nullptr);

        auto& expectedCondition = expectedIf->getCondition();
        auto& condition = ast.getCondition();
        expectedVisits.push(expectedCondition.get());
        EXPECT_TRUE(condition->accept(*this));
        expectedVisits.pop();

        auto& expectedBlock = expectedIf->getBlock();
        auto& block = ast.getBlock();
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(expectedBlock.at(i).get());
            EXPECT_TRUE(block.at(i)->accept(*this));
            expectedVisits.pop();
        }

        auto& expectedElseIf = expectedIf->getElseIfStatements();
        auto& elseIf = ast.getElseIfStatements();
        EXPECT_EQ(expectedElseIf, elseIf);
        for (size_t i = 0; i < expectedElseIf.size(); i++) {
            expectedVisits.push(expectedElseIf.at(i).get());
            EXPECT_TRUE(elseIf.at(i)->accept(*this));
            expectedVisits.pop();
        }

        auto& expectedElseBlock = expectedIf->getElseBlock();
        auto& elseBlock = ast.getElseBlock();
        EXPECT_EQ(expectedElseBlock.size(), elseBlock.size());
        for (size_t i = 0; i < expectedElseBlock.size(); i++) {
            expectedVisits.push(expectedElseBlock.at(i).get());
            EXPECT_TRUE(elseBlock.at(i)->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::For& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedFor = dynamic_cast<AstNode::Statement::For*>(toCast);
        EXPECT_NE(expectedFor, nullptr);

        auto& expectedInit = expectedFor->getInitStatement();
        auto& init = ast.getInitStatement();
        EXPECT_EQ(expectedInit.has_value(), init.has_value());
        if (expectedInit.has_value()) {
            expectedVisits.push(expectedInit->get());
            EXPECT_TRUE(init->get()->accept(*this));
            expectedVisits.pop();
        }

        auto& expectedCondition = expectedFor->getCondition();
        auto& condition = ast.getCondition();
        EXPECT_EQ(expectedCondition.has_value(), condition.has_value());
        if (expectedCondition.has_value()) {
            expectedVisits.push(expectedCondition->get());
            EXPECT_TRUE(condition->get()->accept(*this));
            expectedVisits.pop();
        }

        auto& expectedIncrement = expectedFor->getIncrementExpression();
        auto& increment = ast.getIncrementExpression();
        EXPECT_EQ(expectedIncrement.has_value(), increment.has_value());
        if (expectedIncrement.has_value()) {
            expectedVisits.push(expectedIncrement->get());
            EXPECT_TRUE(increment->get()->accept(*this));
            expectedVisits.pop();
        }

        auto& expectedBlock = expectedFor->getBlock();
        auto& block = ast.getBlock();
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(expectedBlock.at(i).get());
            EXPECT_TRUE(block.at(i)->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::While& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedWhile = dynamic_cast<AstNode::Statement::While*>(toCast);
        EXPECT_NE(expectedWhile, nullptr);

        auto& expectedCondition = expectedWhile->getCondition();
        auto& condition = ast.getCondition();
        expectedVisits.push(expectedCondition.get());
        EXPECT_TRUE(condition->accept(*this));
        expectedVisits.pop();

        auto& expectedBlock = expectedWhile->getBlock();
        auto& block = ast.getBlock();
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(expectedBlock.at(i).get());
            EXPECT_TRUE(block.at(i)->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::Return& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedReturn = dynamic_cast<AstNode::Statement::Return*>(toCast);
        EXPECT_NE(expectedReturn, nullptr);

        auto& expectedValue = expectedReturn->getValue();
        auto& value = ast.getValue();
        EXPECT_EQ(expectedValue.has_value(), value.has_value());
        if (expectedValue.has_value()) {
            expectedVisits.push(expectedValue->get());
            EXPECT_TRUE(value->get()->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::Declaration& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedDeclaration = dynamic_cast<AstNode::Statement::Declaration*>(toCast);
        EXPECT_NE(expectedDeclaration, nullptr);

        EXPECT_EQ(expectedDeclaration->getName(), ast.getName());

        auto& expectedType = expectedDeclaration->getType();
        auto& type = ast.getType();
        expectedVisits.push(expectedType.get());
        EXPECT_TRUE(type->accept(*this));
        expectedVisits.pop();

        auto& expectedValue = expectedDeclaration->getValue();
        auto& value = ast.getValue();
        EXPECT_EQ(expectedValue.has_value(), value.has_value());
        if (expectedValue.has_value()) {
            expectedVisits.push(expectedValue->get());
            EXPECT_TRUE(value->get()->accept(*this));
            expectedVisits.pop();
        }

        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::Assignment& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedAssignment = dynamic_cast<AstNode::Statement::Assignment*>(toCast);
        EXPECT_NE(expectedAssignment, nullptr);
    
        auto& expectedRecipient = expectedAssignment->getRecipient();
        auto& recipient = ast.getRecipient();
        expectedVisits.push(expectedRecipient.get());
        EXPECT_TRUE(recipient->accept(*this));
        expectedVisits.pop();
    
        auto& expectedValue = expectedAssignment->getValue();
        auto& value = ast.getValue();
        expectedVisits.push(expectedValue.get());
        EXPECT_TRUE(value->accept(*this));
        expectedVisits.pop();
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Statement::Expression& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedExpressionStatement = dynamic_cast<AstNode::Statement::Expression*>(toCast);
        EXPECT_NE(expectedExpressionStatement, nullptr);
    
        auto& expectedExpression = expectedExpressionStatement->getExpression();
        auto& expression = ast.getExpression();
        expectedVisits.push(expectedExpression.get());
        EXPECT_TRUE(expression->accept(*this));
        expectedVisits.pop();
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Binary& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedBinary = dynamic_cast<AstNode::Expression::Binary*>(toCast);
        EXPECT_NE(expectedBinary, nullptr);
    
        EXPECT_EQ(expectedBinary->getOp(), ast.getOp());
    
        auto& expectedLeft = expectedBinary->getLeft();
        auto& left = ast.getLeft();
        expectedVisits.push(expectedLeft.get());
        EXPECT_TRUE(left->accept(*this));
        expectedVisits.pop();
    
        auto& expectedRight = expectedBinary->getRight();
        auto& right = ast.getRight();
        expectedVisits.push(expectedRight.get());
        EXPECT_TRUE(right->accept(*this));
        expectedVisits.pop();
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Unary& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedUnary = dynamic_cast<AstNode::Expression::Unary*>(toCast);
        EXPECT_NE(expectedUnary, nullptr);
    
        EXPECT_EQ(expectedUnary->getOp(), ast.getOp());
        EXPECT_EQ(expectedUnary->getPrefix(), ast.getPrefix());
    
        auto& expectedExpression = expectedUnary->getExpression();
        auto& expression = ast.getExpression();
        expectedVisits.push(expectedExpression.get());
        EXPECT_TRUE(expression->accept(*this));
        expectedVisits.pop();
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Group& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedGroup = dynamic_cast<AstNode::Expression::Group*>(toCast);
        EXPECT_NE(expectedGroup, nullptr);
    
        auto& expectedExpression = expectedGroup->getExpression();
        auto& expression = ast.getExpression();
        expectedVisits.push(expectedExpression.get());
        EXPECT_TRUE(expression->accept(*this));
        expectedVisits.pop();
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Method& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedMethod = dynamic_cast<AstNode::Expression::Method*>(toCast);
        EXPECT_NE(expectedMethod, nullptr);
    
        EXPECT_EQ(expectedMethod->getObject(), ast.getObject());
        EXPECT_EQ(expectedMethod->getMethodName(), ast.getMethodName());
        EXPECT_EQ(expectedMethod->getArguments().size(), ast.getArguments().size());
        for (size_t i = 0; i < expectedMethod->getArguments().size(); i++) {
            expectedVisits.push(expectedMethod->getArguments().at(i).get());
            EXPECT_TRUE(ast.getArguments().at(i)->accept(*this));
            expectedVisits.pop();
        }
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Function& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedFunction = dynamic_cast<AstNode::Expression::Function*>(toCast);
        EXPECT_NE(expectedFunction, nullptr);
    
        EXPECT_EQ(expectedFunction->getName(), ast.getName());
        EXPECT_EQ(expectedFunction->getArguments().size(), ast.getArguments().size());
        for (size_t i = 0; i < expectedFunction->getArguments().size(); i++) {
            expectedVisits.push(expectedFunction->getArguments().at(i).get());
            EXPECT_TRUE(ast.getArguments().at(i)->accept(*this));
            expectedVisits.pop();
        }
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Access& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedAccess = dynamic_cast<AstNode::Expression::Access*>(toCast);
        EXPECT_NE(expectedAccess, nullptr);
    
        EXPECT_EQ(expectedAccess->getObject(), ast.getObject());
        EXPECT_EQ(expectedAccess->getSubscript().has_value(), ast.getSubscript().has_value());
        if (expectedAccess->getSubscript().has_value()) {
            expectedVisits.push(expectedAccess->getSubscript()->get());
            EXPECT_TRUE(ast.getSubscript()->get()->accept(*this));
            expectedVisits.pop();
        }
    
        EXPECT_EQ(expectedAccess->getMember().has_value(), ast.getMember().has_value());
        if (expectedAccess->getMember().has_value()) {
            EXPECT_EQ(expectedAccess->getMember(), ast.getMember());
        }
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Expression::Literal& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedLiteral = dynamic_cast<AstNode::Expression::Literal*>(toCast);
        EXPECT_NE(expectedLiteral, nullptr);
    
        // Check if the literal values match (size_t, double, bool, std::string)
        if (std::holds_alternative<size_t>(expectedLiteral->getLiteral())) {
            EXPECT_EQ(std::get<size_t>(expectedLiteral->getLiteral()), std::get<size_t>(ast.getLiteral()));
        } else if (std::holds_alternative<double>(expectedLiteral->getLiteral())) {
            EXPECT_EQ(std::get<double>(expectedLiteral->getLiteral()), std::get<double>(ast.getLiteral()));
        } else if (std::holds_alternative<bool>(expectedLiteral->getLiteral())) {
            EXPECT_EQ(std::get<bool>(expectedLiteral->getLiteral()), std::get<bool>(ast.getLiteral()));
        } else if (std::holds_alternative<std::string>(expectedLiteral->getLiteral())) {
            EXPECT_EQ(std::get<std::string>(expectedLiteral->getLiteral()), std::get<std::string>(ast.getLiteral()));
        }
    
        return true;
    }

    inline std::any ParserTest::TestVisitor::visit(AstNode::Specifier::Type& ast) {
        auto toCast = (expectedVisits.empty()) ? expectedAst.get() : expectedVisits.top();
        auto expectedType = dynamic_cast<AstNode::Specifier::Type*>(toCast);
        EXPECT_NE(expectedType, nullptr);
    
        EXPECT_EQ(expectedType->getName(), ast.getName());
        EXPECT_EQ(expectedType->getTypeArgs().size(), ast.getTypeArgs().size());
        for (size_t i = 0; i < expectedType->getTypeArgs().size(); i++) {
            expectedVisits.push(expectedType->getTypeArgs().at(i).get());
            EXPECT_TRUE(ast.getTypeArgs().at(i)->accept(*this));
            expectedVisits.pop();
        }
    
        return true;
    }
    
}