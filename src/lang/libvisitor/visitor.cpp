#include "visitor.hpp"
#include <variant>

using namespace BlsLang;

BlsObject Visitor::visit(AstNode::Initializer::Task& ast) {
    for (auto&& arg : ast.getArgs()) {
        arg->accept((*this));
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Specifier::Type& ast) {
    for (auto&& arg : ast.getTypeArgs()) {
        arg->accept((*this));
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Literal&) {
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::List& ast) {
    for (auto&& element : ast.getElements()) {
        element->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Set& ast) {
    for (auto&& element : ast.getElements()) {
        element->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Map& ast) {
    for (auto&& element : ast.getElements()) {
        element.first->accept(*this);
        element.second->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Access& ast) {
    if (ast.getSubscript().has_value()) {
        return ast.getSubscript()->get()->accept(*this);
    }    
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Function& ast) {
    for (auto&& arg : ast.getArguments()) {
        arg->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Method& ast) {
    for (auto&& arg : ast.getArguments()) {
        arg->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Visitor::visit(AstNode::Expression::Unary& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Visitor::visit(AstNode::Expression::Binary& ast) {
    ast.getLeft()->accept(*this);
    ast.getRight()->accept(*this);
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::Expression& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Visitor::visit(AstNode::Statement::Declaration& ast) {
    ast.getType()->accept(*this);
    if (ast.getValue().has_value()) {
        ast.getValue()->get()->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::Continue&) {
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::Break&) {
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::Return& ast) {
    if (ast.getValue().has_value()) {
        return ast.getValue()->get()->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::While& ast) {
    ast.getCondition()->accept(*this);
    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::For& ast) {
    if (ast.getInitStatement().has_value()) {
        ast.getInitStatement()->get()->accept(*this);
    }
    if (ast.getCondition().has_value()) {
        ast.getCondition()->get()->accept(*this);
    }
    if (ast.getIncrementExpression().has_value()) {
        ast.getIncrementExpression()->get()->accept(*this);
    }
    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::If& ast) {
    ast.getCondition()->accept(*this);
    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    for (auto&& elif : ast.getElseIfStatements()) {
        elif->accept(*this);
    }
    for (auto&& statement : ast.getElseBlock()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Function::Procedure& ast) {
    ast.getReturnType()->accept(*this);
    for (auto&& type : ast.getParameterTypes()) {
        type->accept(*this);
    }
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Function::Task& ast) {
    for (auto&& option : ast.getConfigOptions()) {
        option->accept(*this);
    }
    for (auto&& type : ast.getParameterTypes()) {
        type->accept(*this);
    }
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Setup& ast) {
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Source& ast) {
    for (auto&& proc : ast.getProcedures()) {
        proc->accept(*this);
    }
    for (auto&& task : ast.getTasks()) {
        task->accept(*this);
    }
    ast.getSetup()->accept(*this);
    return std::monostate();
}
