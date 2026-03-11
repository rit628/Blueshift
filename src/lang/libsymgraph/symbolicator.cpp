#include "symbolicator.hpp"
#include "ast.hpp"
#include <variant>

using namespace BlsLang;

BlsObject Symbolicator::visit(AstNode::Expression::Access& ast) {
    symbols.emplace(ast.object+ "%" + std::to_string(static_cast<int>(ast.localIndex)));
    return std::monostate();
}

BlsObject Symbolicator::visit(AstNode::Statement::Declaration& ast) {
    symbols.emplace(ast.name + "%" + std::to_string(static_cast<int>(ast.localIndex)));
    if (ast.value.has_value()) {
        ast.value->get()->accept(*this);
    }
    return std::monostate();
}