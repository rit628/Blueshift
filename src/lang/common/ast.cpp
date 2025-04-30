#include "ast.hpp"
#include "visitor.hpp"
#include "print_visitor.hpp"
#include <memory>
#include <ostream>
#include <variant>

using namespace BlsLang;

#define AST_NODE_ABSTRACT(...)
#define AST_NODE(Node) \
BlsObject Node::accept(Visitor& v) { return v.visit(*this); }
#include "include/NODE_TYPES.LIST"
#undef AST_NODE
#undef AST_NODE_ABSTRACT

std::ostream& BlsLang::operator<<(std::ostream& os, const AstNode& node) {
    Printer printer(os);
    const_cast<AstNode&>(node).accept(printer);
    return os;
}
