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
    
        auto& expectedProcedures = expectedSource.procedures;
        auto& procedures = ast.procedures;
        EXPECT_EQ(expectedProcedures.size(), procedures.size());
        for (size_t i = 0; i < expectedProcedures.size(); i++) {
            expectedVisits.push(std::move(expectedProcedures.at(i)));
            procedures.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedTasks = expectedSource.tasks;
        auto& tasks = ast.tasks;
        EXPECT_EQ(expectedTasks.size(), tasks.size());
        for (size_t i = 0; i < expectedTasks.size(); i++) {
            expectedVisits.push(std::move(expectedTasks.at(i)));
            tasks.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedSetup = expectedSource.setup;
        auto& setup = ast.setup;
        expectedVisits.push(std::move(expectedSetup));
        setup->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Function::Procedure& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedProcedure = dynamic_cast<AstNode::Function::Procedure&>(*toCast);

        EXPECT_EQ(expectedProcedure.name, ast.name);
        EXPECT_EQ(expectedProcedure.parameters, ast.parameters);

        auto& expectedParameterTypes = expectedProcedure.parameterTypes;
        auto& parameterTypes = ast.parameterTypes;
        for (size_t i = 0; i < expectedParameterTypes.size(); i++) {
            expectedVisits.push(std::move(expectedParameterTypes.at(i)));
            parameterTypes.at(i)->accept(*this);
            expectedVisits.pop();
        }

        expectedVisits.push(std::move(expectedProcedure.returnType));
        ast.returnType->accept(*this);
        expectedVisits.pop();

        auto& expectedStatements = expectedProcedure.statements;
        auto& statements = ast.statements;
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
    
        EXPECT_EQ(expectedTask.name, ast.name);
        EXPECT_EQ(expectedTask.parameters, ast.parameters);

        auto& expectedParameterTypes = expectedTask.parameterTypes;
        auto& parameterTypes = ast.parameterTypes;
        EXPECT_EQ(expectedParameterTypes.size(), parameterTypes.size());
        for (size_t i = 0; i < expectedParameterTypes.size(); i++) {
            expectedVisits.push(std::move(expectedParameterTypes.at(i)));
            parameterTypes.at(i)->accept(*this);
            expectedVisits.pop();
        }

        auto& expectedConfigOptions= expectedTask.configOptions;
        auto& configOptions= ast.configOptions;
        EXPECT_EQ(expectedConfigOptions.size(), configOptions.size());
        for (size_t i = 0; i < expectedConfigOptions.size(); i++) {
            expectedVisits.push(std::move(expectedConfigOptions.at(i)));
            configOptions.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedStatements = expectedTask.statements;
        auto& statements = ast.statements;
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
    
        auto& expectedStatements = expectedSetup.statements;
        auto& statements = ast.statements;
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
    
        auto& expectedCondition = expectedIf.condition;
        auto& condition = ast.condition;
        expectedVisits.push(std::move(expectedCondition));
        condition->accept(*this);
        expectedVisits.pop();
    
        auto& expectedBlock = expectedIf.block;
        auto& block = ast.block;
        EXPECT_EQ(expectedBlock.size(), block.size());
        for (size_t i = 0; i < expectedBlock.size(); i++) {
            expectedVisits.push(std::move(expectedBlock.at(i)));
            block.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedElseIf = expectedIf.elseIfStatements;
        auto& elseIf = ast.elseIfStatements;
        EXPECT_EQ(expectedElseIf.size(), elseIf.size());
        for (size_t i = 0; i < expectedElseIf.size(); i++) {
            expectedVisits.push(std::move(expectedElseIf.at(i)));
            elseIf.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedElseBlock = expectedIf.elseBlock;
        auto& elseBlock = ast.elseBlock;
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
    
        auto& expectedInit = expectedFor.initStatement;
        auto& init = ast.initStatement;
        EXPECT_EQ(expectedInit.has_value(), init.has_value());
        if (expectedInit.has_value()) {
            expectedVisits.push(std::move(*expectedInit));
            init->get()->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedCondition = expectedFor.condition;
        auto& condition = ast.condition;
        EXPECT_EQ(expectedCondition.has_value(), condition.has_value());
        if (expectedCondition.has_value()) {
            expectedVisits.push(std::move(*expectedCondition));
            condition->get()->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedIncrement = expectedFor.incrementExpression;
        auto& increment = ast.incrementExpression;
        EXPECT_EQ(expectedIncrement.has_value(), increment.has_value());
        if (expectedIncrement.has_value()) {
            expectedVisits.push(std::move(*expectedIncrement));
            increment->get()->accept(*this);
            expectedVisits.pop();
        }
    
        auto& expectedBlock = expectedFor.block;
        auto& block = ast.block;
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

        EXPECT_EQ(expectedWhile.type, ast.type);
    
        auto& expectedCondition = expectedWhile.condition;
        auto& condition = ast.condition;
        expectedVisits.push(std::move(expectedCondition));
        condition->accept(*this);
        expectedVisits.pop();
    
        auto& expectedBlock = expectedWhile.block;
        auto& block = ast.block;
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
    
        auto& expectedValue = expectedReturn.value;
        auto& value = ast.value;
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
    
        EXPECT_EQ(expectedDeclaration.name, ast.name);

        EXPECT_EQ(expectedDeclaration.modifiers, ast.modifiers);
    
        auto& expectedType = expectedDeclaration.type;
        auto& type = ast.type;
        expectedVisits.push(std::move(expectedType));
        type->accept(*this);
        expectedVisits.pop();
    
        auto& expectedValue = expectedDeclaration.value;
        auto& value = ast.value;
        EXPECT_EQ(expectedValue.has_value(), value.has_value());
        if (expectedValue.has_value()) {
            expectedVisits.push(std::move(*expectedValue));
            value->get()->accept(*this);
            expectedVisits.pop();
        }

        EXPECT_EQ(expectedDeclaration.localIndex, ast.localIndex);
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Statement::Expression& ast) {
        auto& toCast = (expectedVisits.empty()) ? *expectedAst : *expectedVisits.top();
        auto& expectedExpressionStatement = dynamic_cast<AstNode::Statement::Expression&>(toCast);
    
        auto& expectedExpression = expectedExpressionStatement.expression;
        auto& expression = ast.expression;
        expectedVisits.push(std::move(expectedExpression));
        expression->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Binary& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedBinary = dynamic_cast<AstNode::Expression::Binary&>(*toCast);
    
        EXPECT_EQ(expectedBinary.op, ast.op);
    
        auto& expectedLeft = expectedBinary.left;
        auto& left = ast.left;
        expectedVisits.push(std::move(expectedLeft));
        left->accept(*this);
        expectedVisits.pop();
    
        auto& expectedRight = expectedBinary.right;
        auto& right = ast.right;
        expectedVisits.push(std::move(expectedRight));
        right->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Unary& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedUnary = dynamic_cast<AstNode::Expression::Unary&>(*toCast);
    
        EXPECT_EQ(expectedUnary.op, ast.op);
        EXPECT_EQ(expectedUnary.position, ast.position);
    
        auto& expectedExpression = expectedUnary.expression;
        auto& expression = ast.expression;
        expectedVisits.push(std::move(expectedExpression));
        expression->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Group& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedGroup = dynamic_cast<AstNode::Expression::Group&>(*toCast);
    
        auto& expectedExpression = expectedGroup.expression;
        auto& expression = ast.expression;
        expectedVisits.push(std::move(expectedExpression));
        expression->accept(*this);
        expectedVisits.pop();
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Method& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedMethod = dynamic_cast<AstNode::Expression::Method&>(*toCast);
    
        EXPECT_EQ(expectedMethod.object, ast.object);
        EXPECT_EQ(expectedMethod.methodName, ast.methodName);
        EXPECT_EQ(expectedMethod.arguments.size(), ast.arguments.size());
        for (size_t i = 0; i < expectedMethod.arguments.size(); i++) {
            expectedVisits.push(std::move(expectedMethod.arguments.at(i)));
            ast.arguments.at(i)->accept(*this);
            expectedVisits.pop();
        }

        EXPECT_EQ(expectedMethod.localIndex, ast.localIndex);
        EXPECT_EQ(expectedMethod.objectType, ast.objectType);
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Function& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedFunction = dynamic_cast<AstNode::Expression::Function&>(*toCast);
    
        EXPECT_EQ(expectedFunction.name, ast.name);
        EXPECT_EQ(expectedFunction.arguments.size(), ast.arguments.size());
        for (size_t i = 0; i < expectedFunction.arguments.size(); i++) {
            expectedVisits.push(std::move(expectedFunction.arguments.at(i)));
            ast.arguments.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Access& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedAccess = dynamic_cast<AstNode::Expression::Access&>(*toCast);
    
        EXPECT_EQ(expectedAccess.object, ast.object);
        EXPECT_EQ(expectedAccess.subscript.has_value(), ast.subscript.has_value());
        if (expectedAccess.subscript.has_value()) {
            expectedVisits.push(std::move(*expectedAccess.subscript));
            ast.subscript->get()->accept(*this);
            expectedVisits.pop();
        }
    
        EXPECT_EQ(expectedAccess.member.has_value(), ast.member.has_value());
        if (expectedAccess.member.has_value()) {
            EXPECT_EQ(expectedAccess.member, ast.member);
        }

        EXPECT_EQ(expectedAccess.localIndex, ast.localIndex);
    
        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Expression::Literal& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedLiteral = dynamic_cast<AstNode::Expression::Literal&>(*toCast);
    
        if (std::holds_alternative<int64_t>(expectedLiteral.literal)) {
            EXPECT_EQ(std::get<int64_t>(expectedLiteral.literal), std::get<int64_t>(ast.literal));
        }
        else if (std::holds_alternative<double>(expectedLiteral.literal)) {
            EXPECT_EQ(std::get<double>(expectedLiteral.literal), std::get<double>(ast.literal));
        }
        else if (std::holds_alternative<bool>(expectedLiteral.literal)) {
            EXPECT_EQ(std::get<bool>(expectedLiteral.literal), std::get<bool>(ast.literal));
        }
        else if (std::holds_alternative<std::string>(expectedLiteral.literal)) {
            EXPECT_EQ(std::get<std::string>(expectedLiteral.literal), std::get<std::string>(ast.literal));
        }
    
        return true;
    }

    inline BlsObject Tester::visit(AstNode::Expression::List& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedList = dynamic_cast<AstNode::Expression::List&>(*toCast);

        EXPECT_EQ(expectedList.elements.size(), ast.elements.size());
        for (size_t i = 0; i < expectedList.elements.size(); i++) {
            expectedVisits.push(std::move(expectedList.elements.at(i)));
            ast.elements.at(i)->accept(*this);
            expectedVisits.pop();
        }
        EXPECT_EQ(expectedList.literal, expectedList.literal);

        return true;
    }

    inline BlsObject Tester::visit(AstNode::Expression::Set& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedSet = dynamic_cast<AstNode::Expression::Set&>(*toCast);

        EXPECT_EQ(expectedSet.elements.size(), ast.elements.size());
        for (size_t i = 0; i < expectedSet.elements.size(); i++) {
            expectedVisits.push(std::move(expectedSet.elements.at(i)));
            ast.elements.at(i)->accept(*this);
            expectedVisits.pop();
        }

        return true;
    }

    inline BlsObject Tester::visit(AstNode::Expression::Map& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedMap = dynamic_cast<AstNode::Expression::Map&>(*toCast);

        EXPECT_EQ(expectedMap.elements.size(), ast.elements.size());
        for (size_t i = 0; i < expectedMap.elements.size(); i++) {
            auto& expectedPair = expectedMap.elements.at(i);
            auto& pair = ast.elements.at(i);

            expectedVisits.push(std::move(expectedPair.first));
            pair.first->accept(*this);
            expectedVisits.pop();

            expectedVisits.push(std::move(expectedPair.second));
            pair.second->accept(*this);
            expectedVisits.pop();
        }
        EXPECT_EQ(expectedMap.literal, ast.literal);

        return true;
    }
    
    inline BlsObject Tester::visit(AstNode::Specifier::Type& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedType = dynamic_cast<AstNode::Specifier::Type&>(*toCast);
    
        EXPECT_EQ(expectedType.name, ast.name);
        EXPECT_EQ(expectedType.typeArgs.size(), ast.typeArgs.size());
        for (size_t i = 0; i < expectedType.typeArgs.size(); i++) {
            expectedVisits.push(std::move(expectedType.typeArgs.at(i)));
            ast.typeArgs.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }

    inline BlsObject Tester::visit(AstNode::Initializer::Task& ast) {
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top();
        auto& expectedType = dynamic_cast<AstNode::Initializer::Task&>(*toCast);
    
        EXPECT_EQ(expectedType.option, ast.option);
        EXPECT_EQ(expectedType.args.size(), ast.args.size());
        for (size_t i = 0; i < expectedType.args.size(); i++) {
            expectedVisits.push(std::move(expectedType.args.at(i)));
            ast.args.at(i)->accept(*this);
            expectedVisits.pop();
        }
    
        return true;
    }
}
