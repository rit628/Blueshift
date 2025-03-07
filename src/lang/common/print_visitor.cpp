#include "print_visitor.hpp"

using namespace BlsLang;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

std::any Printer::visit(AstNode::Specifier::Type& ast) {
    os << "AstNode::Specifier::Type {\n";
    os << "  name = " << ast.getName() << "\n";
    os << "  " << "typeArgs = <";
    for (auto&& arg : ast.getTypeArgs()) {
        arg->accept((*this));
    }
    os << " >\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Literal& ast) {
    const auto visitor = overloads {
        [this](int64_t value) { os << "int64_t " << value; },
        [this](double value) { os << "double " << value; },
        [this](bool value) { os << "bool " << value; },
        [this](std::string value) { os << "std::string " << value; }
    };
    os << "AstNode::Expression::Literal {\n";
    os << "  literal = ";
    std::visit(visitor, ast.getLiteral());
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::List& ast) {
    os << "AstNode::Expression::List {\n";
    os << "  elements = [";
    for (auto&& element : ast.getElements()) {
        element->accept(*this);
    }
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Set& ast) {
    os << "AstNode::Expression::Set {\n";
    os << "  elements = [";
    for (auto&& element : ast.getElements()) {
        element->accept(*this);
    }
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Map& ast) {
    os << "AstNode::Expression::Map {\n";
    os << "  elements = [";
    for (auto&& element : ast.getElements()) {
        element.first->accept(*this);
        os << " : ";
        element.second->accept(*this);
    }
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Access& ast) {
    os << "AstNode::Expression::Access {\n";
    os << "  object = " << ast.getObject();
    if (ast.getSubscript().has_value()) {
        os << "  subscript = ";
        ast.getSubscript()->get()->accept(*this);
        os << "\n";
    }
        
    if (ast.getMember().has_value()) {
        os << "  member = " << *ast.getMember() << "\n"; 
    }
    
    os << "}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Function& ast) {
    os << "AstNode::Expression::Function {\n  name = " << ast.getName() << "\n  arguments = [";
    for (auto&& arg : ast.getArguments()) {
        os << " ";
        arg->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Method& ast) {
    os << "AstNode::Expression::Method {\n  object = " << ast.getObject() << "\n  methodName = " << ast.getMethodName() << "\n  arguments = [";
    for (auto&& arg : ast.getArguments()) {
        os << " ";
        arg->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Group& ast) {
    os << "AstNode::Expression::Group {\n  expression = ";
    ast.getExpression()->accept(*this);
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Unary& ast) {
    os << "AstNode::Expression::Unary {\n  op = " << ast.getOp() << "\n  expression = ";
    ast.getExpression()->accept(*this);
    os << "\n  position = " << ((ast.getPosition() == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) ? "prefix" : "postfix");
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Expression::Binary& ast) {
    os << "AstNode::Expression::Binary {\n  op = " << ast.getOp() << "\n  left = ";
    ast.getLeft()->accept(*this);
    os << "\n  right = ";
    ast.getRight()->accept(*this);
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Statement::Expression& ast) {
    os << "AstNode::Statement::Expression {\n  expression = ";
    ast.getExpression()->accept(*this);
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Statement::Declaration& ast) {
    os << "AstNode::Statement::Declaration {\n  name = " << ast.getName();
    os << "\n  type = ";
    ast.getType()->accept(*this);
    os << "\n  value = ";
    if (ast.getValue().has_value()) {
        ast.getValue()->get()->accept(*this);
    }
    else {
        os << "";
    }
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Statement::Continue& ast) {
    os << "AstNode::Statement::Continue {}" << std::endl; 
    return true;
}

std::any Printer::visit(AstNode::Statement::Break& ast) {
    os << "AstNode::Statement::Break {}" << std::endl; 
    return true;
}

std::any Printer::visit(AstNode::Statement::Return& ast) {
    os << "AstNode::Statement::Return {\n  value = ";
    if (ast.getValue().has_value()) {
        ast.getValue()->get()->accept(*this);
    }
    else {
        os << "";
    }
    os << "\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Statement::While& ast) {
    os << "AstNode::Statement::While {\n  type = " << ((ast.getType() == AstNode::Statement::While::LOOP_TYPE::DO) ? "do" : "while");
    os << "\n  condition = ";
    ast.getCondition()->accept(*this);
    os << "\n  block = [";
    for (auto&& statement : ast.getBlock()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Statement::For& ast) {
    os << "AstNode::Statement::For {";
    if (ast.getInitStatement().has_value()) {
        os << "\n  initStatement = ";
        ast.getInitStatement()->get()->accept(*this);
    }
    if (ast.getCondition().has_value()) {
        os << "\n  condition = ";
        ast.getCondition()->get()->accept(*this);
    }
    if (ast.getIncrementExpression().has_value()) {
        os << "\n  incrementExpression = ";
        ast.getIncrementExpression()->get()->accept(*this);
    }
    os << "\n  block = [";
    for (auto&& statement : ast.getBlock()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Statement::If& ast) {
    os << "AstNode::Statement::If {\n  condition = ";
    ast.getCondition()->accept(*this);
    os << "\n  block = [";
    for (auto&& statement : ast.getBlock()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n  elseIfStatements = [";
    for (auto&& elif : ast.getElseIfStatements()) {
        os << " ";
        elif->accept(*this);
    }
    os << " ]\n  elseBlock = [";
    for (auto&& statement : ast.getElseBlock()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Function::Procedure& ast) {
    os << "AstNode::Function::Procedure {\n  name = " << ast.getName();
    os << "\n  returnType = ";
    if (ast.getReturnType().has_value()) {
        ast.getReturnType()->get()->accept(*this);
    }
    else {
        os << "void";
    }
    os << "\n  parameterTypes = [";
    for (auto&& type : ast.getParameterTypes()) {
        type->accept(*this);
        os << " ";
    }
    os << " ]\n  parameters = [";
    for (auto&& parameter : ast.getParameters()) {
        os << " " << parameter;
    }
    os << "\n  statements = [";
    for (auto&& statement : ast.getStatements()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Function::Oblock& ast) {
    os << "AstNode::Function::Oblock {\n  name = " << ast.getName() << "\n}" << std::endl;
    os << "\n  parameterTypes = [";
    for (auto&& type : ast.getParameterTypes()) {
        type->accept(*this);
        os << " ";
    }
    os << " ]\n  parameters = [";
    for (auto&& parameter : ast.getParameters()) {
        os << " " << parameter;
    }
    os << "\n  statements = [";
    for (auto&& statement : ast.getStatements()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Setup& ast) {
    os << "AstNode::Setup {\n  statements = [";
    for (auto&& statement : ast.getStatements()) {
        os << " ";
        statement->accept(*this);
    }
    os << " ]\n}" << std::endl;
    return true;
}

std::any Printer::visit(AstNode::Source& ast) {
    os << "AstNode::Source {\n  procedures = [";
    for (auto&& proc : ast.getProcedures()) {
        os << " ";
        proc->accept(*this);
    }
    os << " ]\n  oblocks = [";
    for (auto&& oblock : ast.getOblocks()) {
        os << " ";
        oblock->accept(*this);
    }
    os << " ]\n  setup = ";
    ast.getSetup()->accept(*this);
    os << "\n}" << std::endl;
    return true;
}
