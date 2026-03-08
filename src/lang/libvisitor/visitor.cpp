#include "visitor.hpp"
#include <variant>

using namespace BlsLang;

BlsObject Visitor::visit(AstNode::Initializer::Task& ast) {
    for (auto&& arg : ast.args) {
        arg->accept((*this));
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Specifier::Type& ast) {
    for (auto&& arg : ast.typeArgs) {
        arg->accept((*this));
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Literal&) {
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::List& ast) {
    for (auto&& element : ast.elements) {
        element->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Set& ast) {
    for (auto&& element : ast.elements) {
        element->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Map& ast) {
    for (auto&& element : ast.elements) {
        element.first->accept(*this);
        element.second->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Access& ast) {
    if (ast.subscript.has_value()) {
        return ast.subscript->get()->accept(*this);
    }    
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Function& ast) {
    for (auto&& arg : ast.arguments) {
        arg->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Method& ast) {
    for (auto&& arg : ast.arguments) {
        arg->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Expression::Group& ast) {
    return ast.expression->accept(*this);
}

BlsObject Visitor::visit(AstNode::Expression::Unary& ast) {
    return ast.expression->accept(*this);
}

BlsObject Visitor::visit(AstNode::Expression::Binary& ast) {
    ast.left->accept(*this);
    ast.right->accept(*this);
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::Expression& ast) {
    return ast.expression->accept(*this);
}

BlsObject Visitor::visit(AstNode::Statement::Declaration& ast) {
    ast.type->accept(*this);
    if (ast.value.has_value()) {
        ast.value->get()->accept(*this);
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
    if (ast.value.has_value()) {
        return ast.value->get()->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::While& ast) {
    ast.condition->accept(*this);
    for (auto&& statement : ast.block) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::For& ast) {
    if (ast.initStatement.has_value()) {
        ast.initStatement->get()->accept(*this);
    }
    if (ast.condition.has_value()) {
        ast.condition->get()->accept(*this);
    }
    if (ast.incrementExpression.has_value()) {
        ast.incrementExpression->get()->accept(*this);
    }
    for (auto&& statement : ast.block) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Statement::If& ast) {
    ast.condition->accept(*this);
    for (auto&& statement : ast.block) {
        statement->accept(*this);
    }
    for (auto&& elif : ast.elseIfStatements) {
        elif->accept(*this);
    }
    for (auto&& statement : ast.elseBlock) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Function::Procedure& ast) {
    ast.returnType->accept(*this);
    for (auto&& type : ast.parameterTypes) {
        type->accept(*this);
    }
    for (auto&& statement : ast.statements) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Function::Task& ast) {
    for (auto&& option : ast.configOptions) {
        option->accept(*this);
    }
    for (auto&& type : ast.parameterTypes) {
        type->accept(*this);
    }
    for (auto&& statement : ast.statements) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Setup& ast) {
    for (auto&& statement : ast.statements) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject Visitor::visit(AstNode::Source& ast) {
    for (auto&& proc : ast.procedures) {
        proc->accept(*this);
    }
    for (auto&& task : ast.tasks) {
        task->accept(*this);
    }
    ast.setup->accept(*this);
    return std::monostate();
}
