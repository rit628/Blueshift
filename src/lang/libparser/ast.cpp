#include "ast.hpp"
#include "visitor.hpp"
#include <memory>
#include <ostream>
#include <variant>

using namespace BlsLang;

#define AST_NODE_ABSTRACT(_)
#define AST_NODE(Node) \
std::any Node::accept(Visitor& v) { return v.visit(*this); }
#include "include/NODE_TYPES.LIST"
#undef AST_NODE
#undef AST_NODE_ABSTRACT

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

// TODO: convert all of these print functions into a single print visitor

void AstNode::Specifier::Type::print(std::ostream& os) const {
    os << "AstNode::Specifier::Type {\n";
    os << "  name = " << name << "\n";
    os << "  " << "typeArgs = <";
    for (auto&& arg : typeArgs) {
        os << " " << *arg;
    }
    os << " >\n}" << std::endl;
}

void AstNode::Expression::Literal::print(std::ostream& os) const {
    const auto visitor = overloads {
        [&os](size_t value) { os << "size_t " << value; },
        [&os](double value) { os << "double " << value; },
        [&os](bool value) { os << "bool " << value; },
        [&os](std::string value) { os << "std::string " << value; }
    };
    os << "AstNode::Expression::Literal {\n";
    os << "  literal = ";
    std::visit(visitor, literal);
    os << "\n}" << std::endl;
}

void AstNode::Expression::List::print(std::ostream& os) const {
    os << "AstNode::Expression::List {\n";
    os << "  type = " << ((type == AstNode::Expression::List::LIST_TYPE::ARRAY) ? "array" : "set") << "\n";
    os << "  elements = [";
    for (auto&& element : elements) {
        os << *element;
    }
    os << "\n}" << std::endl;
}

void AstNode::Expression::Map::print(std::ostream& os) const {
    os << "AstNode::Expression::Map {\n";
    os << "  elements = [";
    for (auto&& element : elements) {
        os << *element.first << " : " << *element.second;
    }
    os << "\n}" << std::endl;
}

void AstNode::Expression::Access::print(std::ostream& os) const {
    os << "AstNode::Expression::Access {\n";
    os << "  object = " << object;
    if (subscript) os << "  subscript = " << **subscript << "\n";
    if (member) os << "  member = " << *member << "\n";
    os << "}" << std::endl;
}

void AstNode::Expression::Function::print(std::ostream& os) const {
    os << "AstNode::Expression::Function {\n  name = " << name << "\n  arguments = [";
    for (auto&& arg : arguments) os << " " << *arg;
    os << " ]\n}" << std::endl;
}

void AstNode::Expression::Method::print(std::ostream& os) const {
    os << "AstNode::Expression::Method {\n  object = " << object << "\n  methodName = " << methodName << "\n  arguments = [";
    for (auto&& arg : arguments) os << " " << *arg;
    os << " ]\n}" << std::endl;
}

void AstNode::Expression::Group::print(std::ostream& os) const {
    os << "AstNode::Expression::Group {\n  expression = " << *expression << "\n}" << std::endl;
}

void AstNode::Expression::Unary::print(std::ostream& os) const {
    os << "AstNode::Expression::Unary {\n  op = " << op << "\n  expression = " << *expression;
    os << "\n  position = " << ((position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) ? "prefix" : "postfix");
    os << "\n}" << std::endl;
}

void AstNode::Expression::Binary::print(std::ostream& os) const {
    os << "AstNode::Expression::Binary {\n  op = " << op << "\n  left = " << *left << "\n  right = " << *right << "\n}" << std::endl;
}

void AstNode::Statement::Expression::print(std::ostream& os) const {
    os << "AstNode::Statement::Expression {\n  expression = " << *expression << "\n}" << std::endl;
}

void AstNode::Statement::Assignment::print(std::ostream& os) const {
    os << "AstNode::Statement::Assignment {\n  recipient = " << *recipient << "\n  value = " << *value << "\n}" << std::endl;
}

void AstNode::Statement::Declaration::print(std::ostream& os) const {
    os << "AstNode::Statement::Declaration {\n  name = " << name;
    os << "\n  type = " << *type;
    os << "\n  value = ";
    if (value) os << **value;
    os << "\n}" << std::endl;
}

void AstNode::Statement::Return::print(std::ostream& os) const {
    os << "AstNode::Statement::Return {\n  value = ";
    (value) ? os << **value : os << "";
    os << "\n}" << std::endl;
}

void AstNode::Statement::While::print(std::ostream& os) const {
    os << "AstNode::Statement::While {\n  condition = " << *condition << "\n  block = [";
    for (auto&& stmt : block) os << " " << *stmt;
    os << " ]\n}" << std::endl;
}

void AstNode::Statement::For::print(std::ostream& os) const {
    if (initStatement) os << "AstNode::Statement::For {\n  initStatement = " << **initStatement;
    if (condition) os << "\n  condition = " << **condition;
    if (incrementExpression) os << "\n  incrementExpression = " << **incrementExpression;
    os << "\n  block = [";
    for (auto&& stmt : block) os << " " << *stmt;
    os << " ]\n}" << std::endl;
}

void AstNode::Statement::If::print(std::ostream& os) const {
    os << "AstNode::Statement::If {\n  condition = " << *condition << "\n  block = [";
    for (auto&& stmt : block) os << " " << *stmt;
    os << " ]\n  elseIfStatements = [";
    for (auto&& elif : elseIfStatements) os << " " << *elif;
    os << " ]\n  elseBlock = [";
    for (auto&& stmt : elseBlock) os << " " << *stmt;
    os << " ]\n}" << std::endl;
}

void AstNode::Function::Procedure::print(std::ostream& os) const {
    os << "AstNode::Function::Procedure {\n  name = " << name;
    os << "\n  returnType = " << **returnType;
    os << "\n  parameterTypes = [";
    for (auto&& t : parameterTypes) {
        os << *t << " ";
    }
    os << " ]\n  parameters = [";
    for (auto&& p : parameters) os << " " << p;
    os << "\n  statements = [";
    for (auto&& stmt : statements) os << " " << *stmt;
    os << " ]\n}" << std::endl;
}

void AstNode::Function::Oblock::print(std::ostream& os) const {
    os << "AstNode::Function::Oblock {\n  name = " << name << "\n}" << std::endl;
    os << "\n  parameterTypes = [";
    for (auto&& t : parameterTypes) {
        os << *t << " ";
    }
    os << " ]\n  parameters = [";
    for (auto&& p : parameters) os << " " << p;
    os << "\n  statements = [";
    for (auto&& stmt : statements) os << " " << *stmt;
    os << " ]\n}" << std::endl;
}

void AstNode::Setup::print(std::ostream& os) const {
    os << "AstNode::Setup {\n  statements = [";
    for (auto&& stmt : statements) os << " " << *stmt;
    os << " ]\n}" << std::endl;
}

void AstNode::Source::print(std::ostream& os) const {
    os << "AstNode::Source {\n  procedures = [";
    for (auto&& proc : procedures) os << " " << *proc;
    os << " ]\n  oblocks = [";
    for (auto&& obl : oblocks) os << " " << *obl;
    os << " ]\n  setup = " << *setup;
    os << "\n}" << std::endl;
}
