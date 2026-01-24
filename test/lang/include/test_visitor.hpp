#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <gtest/gtest.h>
#include <stack>

namespace BlsLang {
    class Tester : public Visitor {
        public:
            Tester() = default;
            Tester(std::unique_ptr<AstNode> expectedAst) : expectedAst(std::move(expectedAst)) {}
            void addExpectedAst(std::unique_ptr<AstNode> expectedAst) { this->expectedAst = std::move(expectedAst); }
            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
    
            std::unique_ptr<AstNode> expectedAst;
            std::stack<std::unique_ptr<AstNode>> expectedVisits;
    };

    inline BlsObject Tester::visit(AstNode::Source& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedSource = dynamic_cast<AstNode::Source&>(*toCast);
    
        auto& expectedProcedures = expectedSource.getProcedures();
        auto& procedures = ast.getProcedures();
        EXPECT_EQ(expectedProcedures.size(), procedures.size());
        for (size_t i = 0; i < expectedProcedures.size(); i++) {
            expectedVisits.push(std::move(expectedProcedures.at(i)));
            procedures.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedTasks = expectedSource.getTasks();
        auto& tasks = ast.getTasks();
        EXPECT_EQ(expectedTasks.size(), tasks.size());
        for (size_t i = 0; i < expectedTasks.size(); i++) {
            expectedVisits.push(std::move(expectedTasks.at(i)));
            tasks.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedSetup = expectedSource.getSetup();
        auto& setup = ast.getSetup();
        expectedVisits.push(std::move(expectedSetup));
        setup->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Function::Procedure& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedProcedure = dynamic_cast<AstNode::Function::Procedure&>(*toCast);

        EXPECT_EQ(expectedProcedure.getName(), ast.getName());
        EXPECT_EQ(expectedProcedure.getParameters(), ast.getParameters());

        auto& expectedParameterTypes = expectedProcedure.getParameterTypes();
        auto& parameterTypes = ast.getParameterTypes();
        for (size_t i = 0; i < expectedParameterTypes.size(); i++) {
            expectedVisits.push(std::move(expectedParameterTypes.at(i)));
            parameterTypes.at(i)->accept(*this);
            expectedVisits.pop();
        }

        expectedVisits.push(std::move(expectedProcedure.getReturnType()));
        ast.getReturnType()->accept(*this);
        expectedVisits.pop();

        auto& expectedStatements = expectedProcedure.getStatements();
        auto& statements = ast.getStatements();
        EXPECT_EQ(expectedStatements.size(), statements.size());
        for (size_t i = 0; i < expectedStatements.size(); i++) {
            expectedVisits.push(std::move(expectedStatements.at(i)));
            statements.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Function::Task& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedTask = dynamic_cast<AstNode::Function::Task&>(*toCast);
    
        EXPECT_EQ(expectedTask.getName(), ast.getName());
        EXPECT_EQ(expectedTask.getParameters(), ast.getParameters());

        auto& expectedParameterTypes = expectedTask.getParameterTypes();
        auto& parameterTypes = ast.getParameterTypes();
        EXPECT_EQ(expectedParameterTypes.size(), parameterTypes.size());
        for (size_t i = 0; i < expectedParameterTypes.size(); i++) {
            expectedVisits.push(std::move(expectedParameterTypes.at(i)));
            parameterTypes.at(i)->accept(*this);
            expectedVisits.pop();
        }

        auto& expectedConfigOptions= expectedTask.getConfigOptions();
        auto& configOptions= ast.getConfigOptions();
        EXPECT_EQ(expectedConfigOptions.size(), configOptions.size());
        for (size_t i = 0; i < expectedConfigOptions.size(); i++) {
            expectedVisits.push(std::move(expectedConfigOptions.at(i)));
            configOptions.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedStatements = expectedTask.getStatements();
        auto& statements = ast.getStatements();
        EXPECT_EQ(expectedStatements.size(), statements.size());
        for (size_t i = 0; i < expectedStatements.size(); i++) {
            expectedVisits.push(std::move(expectedStatements.at(i)));
            statements.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Setup& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedSetup = dynamic_cast<AstNode::Setup&>(*toCast);
    
        auto& expectedStatements = expectedSetup.getStatements();
        auto& statements = ast.getStatements();
        EXPECT_EQ(expectedStatements.size(), statements.size());
        for (size_t i = 0; i < expectedStatements.size(); i++) {
            expectedVisits.push(std::move(expectedStatements.at(i)));
            statements.at(i)->accept(*this);
            expectedVisits.pop();
        }
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::If& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedIf = dynamic_cast<AstNode::Statement::If&>(*toCast);
    
        auto& expectedCondition = expectedIf.getCondition();
        auto& condition = ast.getCondition();
        expectedVisits.push(std::move(expectedCondition));
        condition->accept(*this);
        expectedVisits.pop();
    
        auto& expectedBlock = expectedIf.getBlock();
        auto& block = ast.getBlock();
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(std::move(expectedBlock.at(i)));
            block.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedElseIf = expectedIf.getElseIfStatements();
        auto& elseIf = ast.getElseIfStatements();
        EXPECT_EQ(expectedElseIf.size(), elseIf.size());
        for (size_t i = 0; i < expectedElseIf.size(); i++) {
            expectedVisits.push(std::move(expectedElseIf.at(i)));
            elseIf.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedElseBlock = expectedIf.getElseBlock();
        auto& elseBlock = ast.getElseBlock();
        EXPECT_EQ(expectedElseBlock.size(), elseBlock.size());
        for (size_t i = 0; i < expectedElseBlock.size(); i++) {
            expectedVisits.push(std::move(expectedElseBlock.at(i)));
            elseBlock.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::For& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedFor = dynamic_cast<AstNode::Statement::For&>(*toCast);
    
        auto& expectedInit = expectedFor.getInitStatement();
        auto& init = ast.getInitStatement();
        EXPECT_EQ(expectedInit.has_value(), init.has_value());
        if (expectedInit.has_value()) {
            expectedVisits.push(std::move(*expectedInit));
            init->get()->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedCondition = expectedFor.getCondition();
        auto& condition = ast.getCondition();
        EXPECT_EQ(expectedCondition.has_value(), condition.has_value());
        if (expectedCondition.has_value()) {
            expectedVisits.push(std::move(*expectedCondition));
            condition->get()->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedIncrement = expectedFor.getIncrementExpression();
        auto& increment = ast.getIncrementExpression();
        EXPECT_EQ(expectedIncrement.has_value(), increment.has_value());
        if (expectedIncrement.has_value()) {
            expectedVisits.push(std::move(*expectedIncrement));
            increment->get()->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedBlock = expectedFor.getBlock();
        auto& block = ast.getBlock();
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(std::move(expectedBlock.at(i)));
            block.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::While& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedWhile = dynamic_cast<AstNode::Statement::While&>(*toCast);

        EXPECT_EQ(expectedWhile.getType(), ast.getType());
    
        auto& expectedCondition = expectedWhile.getCondition();
        auto& condition = ast.getCondition();
        expectedVisits.push(std::move(expectedCondition));
        condition->accept(*this);
        expectedVisits.pop();
    
        auto& expectedBlock = expectedWhile.getBlock();
        auto& block = ast.getBlock();
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(std::move(expectedBlock.at(i)));
            block.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::Return& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedReturn = dynamic_cast<AstNode::Statement::Return&>(*toCast);
    
        auto& expectedValue = expectedReturn.getValue();
        auto& value = ast.getValue();
        EXPECT_EQ(expectedValue.has_value(), value.has_value());
        if (expectedValue.has_value()) {
            expectedVisits.push(std::move(*expectedValue));
            value->get()->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }

    inline BlsObject Tester::visit(AstNode::Statement::Continue&) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        std::ignore = dynamic_cast<AstNode::Statement::Continue&>(*toCast);
        
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::Break&) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        std::ignore = dynamic_cast<AstNode::Statement::Break&>(*toCast);
        
        return true;
    }

    inline BlsObject Tester::visit(AstNode::Statement::Declaration& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedDeclaration = dynamic_cast<AstNode::Statement::Declaration&>(*toCast);
    
        EXPECT_EQ(expectedDeclaration.getName(), ast.getName());

        EXPECT_EQ(expectedDeclaration.getModifiers(), ast.getModifiers());
    
        auto& expectedType = expectedDeclaration.getType();
        auto& type = ast.getType();
        expectedVisits.push(std::move(expectedType));
        type->accept(*this);
        expectedVisits.pop();
    
        auto& expectedValue = expectedDeclaration.getValue();
        auto& value = ast.getValue();
        EXPECT_EQ(expectedValue.has_value(), value.has_value());
        if (expectedValue.has_value()) {
            expectedVisits.push(std::move(*expectedValue));
            value->get()->accept(*this);
            expectedVisits.pop();
        }

        EXPECT_EQ(expectedDeclaration.getLocalIndex(), ast.getLocalIndex());
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::Expression& ast) {
        auto& toCast = (expectedVisits.empty()) ? *expectedAst : *expectedVisits.top();
        auto& expectedExpressionStatement = dynamic_cast<AstNode::Statement::Expression&>(toCast);
    
        auto& expectedExpression = expectedExpressionStatement.getExpression();
        auto& expression = ast.getExpression();
        expectedVisits.push(std::move(expectedExpression));
        expression->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Binary& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedBinary = dynamic_cast<AstNode::Expression::Binary&>(*toCast);
    
        EXPECT_EQ(expectedBinary.getOp(), ast.getOp());
    
        auto& expectedLeft = expectedBinary.getLeft();
        auto& left = ast.getLeft();
        expectedVisits.push(std::move(expectedLeft));
        left->accept(*this);
        expectedVisits.pop();
    
        auto& expectedRight = expectedBinary.getRight();
        auto& right = ast.getRight();
        expectedVisits.push(std::move(expectedRight));
        right->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Unary& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedUnary = dynamic_cast<AstNode::Expression::Unary&>(*toCast);
    
        EXPECT_EQ(expectedUnary.getOp(), ast.getOp());
        EXPECT_EQ(expectedUnary.getPosition(), ast.getPosition());
    
        auto& expectedExpression = expectedUnary.getExpression();
        auto& expression = ast.getExpression();
        expectedVisits.push(std::move(expectedExpression));
        expression->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Group& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedGroup = dynamic_cast<AstNode::Expression::Group&>(*toCast);
    
        auto& expectedExpression = expectedGroup.getExpression();
        auto& expression = ast.getExpression();
        expectedVisits.push(std::move(expectedExpression));
        expression->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Method& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedMethod = dynamic_cast<AstNode::Expression::Method&>(*toCast);
    
        EXPECT_EQ(expectedMethod.getObject(), ast.getObject());
        EXPECT_EQ(expectedMethod.getMethodName(), ast.getMethodName());
        EXPECT_EQ(expectedMethod.getArguments().size(), ast.getArguments().size());
        for (size_t i = 0; i < expectedMethod.getArguments().size(); i++) {
            expectedVisits.push(std::move(expectedMethod.getArguments().at(i)));
            ast.getArguments().at(i)->accept(*this);
            expectedVisits.pop();
        }

        EXPECT_EQ(expectedMethod.getLocalIndex(), ast.getLocalIndex());
        EXPECT_EQ(expectedMethod.getObjectType(), ast.getObjectType());
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Function& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedFunction = dynamic_cast<AstNode::Expression::Function&>(*toCast);
    
        EXPECT_EQ(expectedFunction.getName(), ast.getName());
        EXPECT_EQ(expectedFunction.getArguments().size(), ast.getArguments().size());
        for (size_t i = 0; i < expectedFunction.getArguments().size(); i++) {
            expectedVisits.push(std::move(expectedFunction.getArguments().at(i)));
            ast.getArguments().at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Access& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedAccess = dynamic_cast<AstNode::Expression::Access&>(*toCast);
    
        EXPECT_EQ(expectedAccess.getObject(), ast.getObject());
        EXPECT_EQ(expectedAccess.getSubscript().has_value(), ast.getSubscript().has_value());
        if (expectedAccess.getSubscript().has_value()) {
            expectedVisits.push(std::move(*expectedAccess.getSubscript()));
            ast.getSubscript()->get()->accept(*this);
            expectedVisits.pop();
        }
    
        EXPECT_EQ(expectedAccess.getMember().has_value(), ast.getMember().has_value());
        if (expectedAccess.getMember().has_value()) {
            EXPECT_EQ(expectedAccess.getMember(), ast.getMember());
        }

        EXPECT_EQ(expectedAccess.getLocalIndex(), ast.getLocalIndex());
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Literal& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedLiteral = dynamic_cast<AstNode::Expression::Literal&>(*toCast);
    
        if (std::holds_alternative<int64_t>(expectedLiteral.getLiteral())) {
            EXPECT_EQ(std::get<int64_t>(expectedLiteral.getLiteral()), std::get<int64_t>(ast.getLiteral()));
        }
        else if (std::holds_alternative<double>(expectedLiteral.getLiteral())) {
            EXPECT_EQ(std::get<double>(expectedLiteral.getLiteral()), std::get<double>(ast.getLiteral()));
        }
        else if (std::holds_alternative<bool>(expectedLiteral.getLiteral())) {
            EXPECT_EQ(std::get<bool>(expectedLiteral.getLiteral()), std::get<bool>(ast.getLiteral()));
        }
        else if (std::holds_alternative<std::string>(expectedLiteral.getLiteral())) {
            EXPECT_EQ(std::get<std::string>(expectedLiteral.getLiteral()), std::get<std::string>(ast.getLiteral()));
        }
    
        return true;
    }

    inline BlsObject Tester::visit(AstNode::Expression::List& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedList = dynamic_cast<AstNode::Expression::List&>(*toCast);

        EXPECT_EQ(expectedList.getElements().size(), ast.getElements().size());
        for (size_t i = 0; i < expectedList.getElements().size(); i++) {
            expectedVisits.push(std::move(expectedList.getElements().at(i)));
            ast.getElements().at(i)->accept(*this);
            expectedVisits.pop();
        }
        EXPECT_EQ(expectedList.getLiteral(), expectedList.getLiteral());

        return true;
    }

    inline BlsObject Tester::visit(AstNode::Expression::Set& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedSet = dynamic_cast<AstNode::Expression::Set&>(*toCast);

        EXPECT_EQ(expectedSet.getElements().size(), ast.getElements().size());
        for (size_t i = 0; i < expectedSet.getElements().size(); i++) {
            expectedVisits.push(std::move(expectedSet.getElements().at(i)));
            ast.getElements().at(i)->accept(*this);
            expectedVisits.pop();
        }

        return true;
    }

    inline BlsObject Tester::visit(AstNode::Expression::Map& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedMap = dynamic_cast<AstNode::Expression::Map&>(*toCast);

        EXPECT_EQ(expectedMap.getElements().size(), ast.getElements().size());
        for (size_t i = 0; i < expectedMap.getElements().size(); i++) {
            auto& expectedPair = expectedMap.getElements().at(i);
            auto& pair = ast.getElements().at(i);

            expectedVisits.push(std::move(expectedPair.first));
            pair.first->accept(*this);
            expectedVisits.pop();

            expectedVisits.push(std::move(expectedPair.second));
            pair.second->accept(*this);
            expectedVisits.pop();
        }
        EXPECT_EQ(expectedMap.getLiteral(), ast.getLiteral());

        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Specifier::Type& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedType = dynamic_cast<AstNode::Specifier::Type&>(*toCast);
    
        EXPECT_EQ(expectedType.getName(), ast.getName());
        EXPECT_EQ(expectedType.getTypeArgs().size(), ast.getTypeArgs().size());
        for (size_t i = 0; i < expectedType.getTypeArgs().size(); i++) {
            expectedVisits.push(std::move(expectedType.getTypeArgs().at(i)));
            ast.getTypeArgs().at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }

    inline BlsObject Tester::visit(AstNode::Initializer::Task& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedType = dynamic_cast<AstNode::Initializer::Task&>(*toCast);
    
        EXPECT_EQ(expectedType.getOption(), ast.getOption());
        EXPECT_EQ(expectedType.getArgs().size(), ast.getArgs().size());
        for (size_t i = 0; i < expectedType.getArgs().size(); i++) {
            expectedVisits.push(std::move(expectedType.getArgs().at(i)));
            ast.getArgs().at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
}
