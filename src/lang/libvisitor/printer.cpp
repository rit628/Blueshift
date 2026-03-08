#include "printer.hpp"
#include "ast.hpp"

using namespace BlsLang;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

BlsObject Printer::visit(AstNode::Initializer::Task& ast) {
    os << "AstNode::Initializer::Task {\n";
    os << "  option = " << ast.option << "\n";
    os << "  " << "arguments = <";
    for (auto&& arg : ast.args) {
        arg->accept((*this));
    }
    os << " >\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Specifier::Type& ast) {
    os << "AstNode::Specifier::Type {\n";
    os << "  name = " << ast.name << "\n";
    os << "  " << "typeArgs = <";
    for (auto&& arg : ast.typeArgs) {
        arg->accept((*this));
    }
    os << " >\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Literal& ast) {
    const auto visitor = overloads {
        [this](int64_t value) { os << "int64_t " << value; },
        [this](double value) { os << "double " << value; },
        [this](bool value) { os << "bool " << value; },
        [this](std::string value) { os << "std::string " << value; }
    };
    os << "AstNode::Expression::Literal {\n";
    os << "  literal = ";
    std::visit(visitor, ast.literal);
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::List& ast) {
    os << "AstNode::Expression::List {\n";
    os << "  elements = [";
    for (auto&& element : ast.elements) {
        element->accept(*this);
    }
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Set& ast) {
    os << "AstNode::Expression::Set {\n";
    os << "  elements = [";
    for (auto&& element : ast.elements) {
        element->accept(*this);
    }
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Map& ast) {
    os << "AstNode::Expression::Map {\n";
    os << "  elements = [";
    for (auto&& element : ast.elements) {
        element.first->accept(*this);
        os << " : ";
        element.second->accept(*this);
    }
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Access& ast) {
    os << "AstNode::Expression::Access {\n";
    os << "  object = " << ast.object;
    if (ast.subscript.has_value()) {
        os << "  subscript = ";
        ast.subscript->get()->accept(*this);
        os << "\n";
    }
        
    if (ast.member.has_value()) {
        os << "  member = " << *ast.member << "\n"; 
    }
    
    os << "}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Function& ast) {
    os << "AstNode::Expression::Function {\n  name = " << ast.name << "\n  arguments = [";
    for (auto&& arg : ast.arguments) {
        os << " ";
        arg->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Method& ast) {
    os << "AstNode::Expression::Method {\n  object = " << ast.object << "\n  methodName = " << ast.methodName << "\n  arguments = [";
    for (auto&& arg : ast.arguments) {
        os << " ";
        arg->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Group& ast) {
    os << "AstNode::Expression::Group {\n  expression = ";
    ast.expression->accept(*this);
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Unary& ast) {
    os << "AstNode::Expression::Unary {\n  op = " << ast.op << "\n  expression = ";
    ast.expression->accept(*this);
    os << "\n  position = " << ((ast.position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) ? "prefix" : "postfix");
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Expression::Binary& ast) {
    os << "AstNode::Expression::Binary {\n  op = " << ast.op << "\n  left = ";
    ast.left->accept(*this);
    os << "\n  right = ";
    ast.right->accept(*this);
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Statement::Expression& ast) {
    os << "AstNode::Statement::Expression {\n  expression = ";
    ast.expression->accept(*this);
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Statement::Declaration& ast) {
    os << "AstNode::Statement::Declaration {\n  name = " << ast.name;
    os << "\n  type = ";
    ast.type->accept(*this);
    os << "\n  value = ";
    if (ast.value.has_value()) {
        ast.value->get()->accept(*this);
    }
    else {
        os << "";
    }
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Statement::Continue&) {
    os << "AstNode::Statement::Continue {}" << std::endl; 
    return true;
}

BlsObject Printer::visit(AstNode::Statement::Break&) {
    os << "AstNode::Statement::Break {}" << std::endl; 
    return true;
}

BlsObject Printer::visit(AstNode::Statement::Return& ast) {
    os << "AstNode::Statement::Return {\n  value = ";
    if (ast.value.has_value()) {
        ast.value->get()->accept(*this);
    }
    else {
        os << "";
    }
    os << "\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Statement::While& ast) {
    os << "AstNode::Statement::While {\n  type = " << ((ast.type == AstNode::Statement::While::LOOP_TYPE::DO) ? "do" : "while");
    os << "\n  condition = ";
    ast.condition->accept(*this);
    os << "\n  block = [";
    for (auto&& statement : ast.block) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Statement::For& ast) {
    os << "AstNode::Statement::For {";
    if (ast.initStatement.has_value()) {
        os << "\n  initStatement = ";
        ast.initStatement->get()->accept(*this);
    }
    if (ast.condition.has_value()) {
        os << "\n  condition = ";
        ast.condition->get()->accept(*this);
    }
    if (ast.incrementExpression.has_value()) {
        os << "\n  incrementExpression = ";
        ast.incrementExpression->get()->accept(*this);
    }
    os << "\n  block = [";
    for (auto&& statement : ast.block) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Statement::If& ast) {
    os << "AstNode::Statement::If {\n  condition = ";
    ast.condition->accept(*this);
    os << "\n  block = [";
    for (auto&& statement : ast.block) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n  elseIfStatements = [";
    for (auto&& elif : ast.elseIfStatements) {
        os << " ";
        elif->accept(*this);
    }
    os << " ]\n  elseBlock = [";
    for (auto&& statement : ast.elseBlock) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Function::Procedure& ast) {
    os << "AstNode::Function::Procedure {\n  name = " << ast.name;
    os << "\n  returnType = ";
    ast.returnType->accept(*this);
    os << "\n  parameterTypes = [";
    for (auto&& type : ast.parameterTypes) {
        type->accept(*this);
        os << " ";
    }
    os << " ]\n  parameters = [";
    for (auto&& parameter : ast.parameters) {
        os << " " << parameter;
    }
    os << "\n  statements = [";
    for (auto&& statement : ast.statements) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Function::Task& ast) {
    os << "AstNode::Function::Task {\n  name = " << ast.name << "\n}" << std::endl;
    os << "\n  configOptions = ";
    for (auto&& option : ast.configOptions) {
        option->accept(*this);
    }
    os << "\n  parameterTypes = [";
    for (auto&& type : ast.parameterTypes) {
        type->accept(*this);
        os << " ";
    }
    os << " ]\n  parameters = [";
    for (auto&& parameter : ast.parameters) {
        os << " " << parameter;
    }
    os << "\n  statements = [";
    for (auto&& statement : ast.statements) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Setup& ast) {
    os << "AstNode::Setup {\n  statements = [";
    for (auto&& statement : ast.statements) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

BlsObject Printer::visit(AstNode::Source& ast) {
    os << "AstNode::Source {\n  procedures = [";
    for (auto&& proc : ast.procedures) {
        os << " ";
        proc->accept(*this);
    }
    os << " ]\n  tasks = [";
    for (auto&& task : ast.tasks) {
        os << " ";
        task->accept(*this);
    }
    os << " ]\n  setup = ";
    ast.setup->accept(*this);
    os << "\n}" << std::endl;
    return true;
}
