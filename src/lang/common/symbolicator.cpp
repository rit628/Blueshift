#include "symbolicator.hpp"
#include "ast.hpp"
#include <variant>

using namespace BlsLang;

BlsObject Symbolicator::visit(AstNode::Specifier::Type& ast) {
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Literal& ast) {
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::List& ast) {
    for (auto&& element : ast.getElements()) {
        element->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Set& ast) {
    for (auto&& element : ast.getElements()) {
        element->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Map& ast) {
    for (auto&& element : ast.getElements()) {
        element.first->accept(*this);
        element.second->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Access& ast) {
    symbols.emplace(ast.getObject()+ "%" + std::to_string(static_cast<int>(ast.getLocalIndex())));
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Function& ast) {
    for (auto&& arg : ast.getArguments()) {
        arg->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Method& ast) {
    for (auto&& arg : ast.getArguments()) {
        arg->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Group& ast) {
    ast.getExpression()->accept(*this);
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Unary& ast) {
    ast.getExpression()->accept(*this);
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Expression::Binary& ast) {
    ast.getLeft()->accept(*this);
    ast.getRight()->accept(*this);
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::Expression& ast) {
    ast.getExpression()->accept(*this);
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::Declaration& ast) {
    symbols.emplace(ast.getName() + "%" + std::to_string(static_cast<int>(ast.getLocalIndex())));
    if (ast.getValue().has_value()) {
        ast.getValue()->get()->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::Continue& ast) {
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::Break& ast) {
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::Return& ast) {
    if (ast.getValue().has_value()) {
        ast.getValue()->get()->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::While& ast) {
    ast.getCondition()->accept(*this);
    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::For& ast) {
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

BlsObject Symbolicator::visit(AstNode::Statement::If& ast) {
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

BlsObject Symbolicator::visit(AstNode::Function::Procedure& ast) {
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Function::Oblock& ast) {
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Setup& ast) {
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Source& ast) {
    for (auto&& proc : ast.getProcedures()) {
        proc->accept(*this);
    }
    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }
    ast.getSetup()->accept(*this);
    return std::monostate();
}
