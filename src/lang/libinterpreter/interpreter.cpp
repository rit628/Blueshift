#include "interpreter.hpp"
#include "bls_types.hpp"
#include "call_stack.hpp"
#include "ast.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace BlsLang;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

std::any Interpreter::visit(AstNode::Source& ast) {
    for (auto&& procedure : ast.getProcedures()) {
        procedure->accept(*this);
    }

    // leave oblocks and setup for later
    const auto visitor = overloads {
        [](std::monostate value) { std::cout << "retVal = void" << std::endl; },
        [](bool value) { std::cout << "retVal = " << ((value) ? "true" : "false") << std::endl; },
        [](auto value) { std::cout << "retVal = " << value << std::endl; }
    };
    for (auto&& i : ast.getSetup()->getStatements()) {
        auto result = i->accept(*this);
        std::visit(visitor, resolve(result));
    }
}

std::any Interpreter::visit(AstNode::Function::Procedure& ast) {
    auto& functionName = ast.getName();

    auto function = [&ast, this](std::vector<BlsType> args) -> std::any {
        auto& functionName = ast.getName();
        auto& params = ast.getParameters();
        auto& statements = ast.getStatements();
        cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION);
        if (params.size() != args.size()) {
            throw std::runtime_error("Invalid number of arguments provided to function call.");
        }
        for (int i = 0; i < params.size(); i++) {
            cs.setLocal(params.at(i), args.at(i));
        }

        try {
            for (auto&& statement : statements) {
                statement->accept(*this);
            }
        } 
        catch (std::any& ret) {
            BlsType result = resolve(ret);
            cs.popFrame();
            return result;
        }
        cs.popFrame();
        return std::monostate();
    };
    functions.emplace(functionName, function);
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Function::Oblock& ast) {}

std::any Interpreter::visit(AstNode::Setup& ast) {}

std::any Interpreter::visit(AstNode::Statement::If& ast) {
    auto conditionResult = ast.getCondition()->accept(*this);
    if (std::get<bool>(resolve(conditionResult))) {
        cs.pushFrame(CallStack<std::string>::Frame::Context::CONDITIONAL);
        for (auto&& statement : ast.getBlock()) {
            statement->accept(*this);
        }
        cs.popFrame();
        return BlsType(true);
    }
    else {
        for (auto&& elif : ast.getElseIfStatements()) {
            auto elifResult = elif->accept(*this);
            if (std::get<bool>(resolve(elifResult))) {
                return BlsType(true); // short circuit if elif condition is satisfied
            }
        }
        
        cs.pushFrame(CallStack<std::string>::Frame::Context::CONDITIONAL);
        for (auto&& statement : ast.getElseBlock()) {
            statement->accept(*this);
        }
        cs.popFrame();
        return BlsType(false);
    }
}

std::any Interpreter::visit(AstNode::Statement::For& ast) {
    auto& initStatement = ast.getInitStatement();
    auto& condition = ast.getCondition();
    auto& incrementExpression = ast.getIncrementExpression();
    auto& statements = ast.getBlock();

    cs.pushFrame(CallStack<std::string>::Frame::Context::LOOP);
    std::function<void()> initExec = []() {};
    std::function<bool()> conditionExec = []() { return true; };
    std::function<void()> incrementExec = []() {};

    if (initStatement.has_value()) {
        initExec = [&initStatement, this]() { initStatement->get()->accept(*this); };
    }
    if (condition.has_value()) {
        conditionExec = [&condition, this]() {
            auto conditionResult = condition->get()->accept(*this);
            return std::get<bool>(resolve(conditionResult));
        };
    }
    if (incrementExpression.has_value()) {
        incrementExec = [&incrementExpression, this]() { incrementExpression->get()->accept(*this); };
    }

    for (initExec(); conditionExec(); incrementExec()) {
        try {
            for (auto&& statement : statements) {
                statement->accept(*this);
            }
        }
        catch (AstNode::Statement::Continue& c) {
            continue;
        }
        catch (AstNode::Statement::Break& b) {
            break;
        }
    }
    
    cs.popFrame();
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Statement::While& ast) {
    auto& type = ast.getType();
    auto& condition = ast.getCondition();
    auto& statements = ast.getBlock();

    cs.pushFrame(CallStack<std::string>::Frame::Context::LOOP);

    if (type == AstNode::Statement::While::LOOP_TYPE::DO) {
        try {
            for (auto&& statement : statements) {
                statement->accept(*this);
            }
        }
        catch (AstNode::Statement::Continue& c) {}
        catch (AstNode::Statement::Break& b) {
            cs.popFrame();
            return std::monostate();
        }
    }

    auto conditionExec = [&condition, this]() {
        auto conditionResult = condition->accept(*this);
        return std::get<bool>(resolve(conditionResult));
    };
    while (conditionExec()) {
        try {
            for (auto&& statement : statements) {
                statement->accept(*this);
            }
        }
        catch (AstNode::Statement::Continue& c) {
            continue;
        }
        catch (AstNode::Statement::Break& b) {
            break;
        }
    }

    cs.popFrame();
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Statement::Return& ast) {
    auto& value = ast.getValue();
    if (value.has_value()) {
        auto literal = value->get()->accept(*this);
        throw literal;
    }
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Statement::Continue& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw std::runtime_error("continue statement outside of loop context.");
    }
    throw ast;
}

std::any Interpreter::visit(AstNode::Statement::Break& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw std::runtime_error("break statement outside of loop context.");
    }
    throw ast;
}

std::any Interpreter::visit(AstNode::Statement::Declaration& ast) {
    auto& name = ast.getName();
    auto& value = ast.getValue();
    if (value.has_value()) {
        auto literal = value->get()->accept(*this);
        cs.setLocal(name, resolve(literal));
    }
    else {
        cs.setLocal(name, std::monostate());
    }
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Statement::Assignment& ast) {
    auto recipient = ast.getRecipient()->accept(*this);
    auto value = ast.getValue()->accept(*this);
    resolve(recipient) = resolve(value);
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Statement::Expression& ast) {
    return ast.getExpression()->accept(*this);
}

std::any Interpreter::visit(AstNode::Expression::Binary& ast) {
    auto lRes = ast.getLeft()->accept(*this);
    auto rRes = ast.getRight()->accept(*this);
    auto& lhs = resolve(lRes);
    auto& rhs = resolve(rRes);

    auto op = getBinOpEnum(ast.getOp());
    switch (op) {
        case Interpreter::BINARY_OPERATOR::OR:
            return lhs || rhs;
        break;

        case Interpreter::BINARY_OPERATOR::AND:
            return lhs && rhs;
        break;

        case Interpreter::BINARY_OPERATOR::LT:
            return lhs < rhs;
        break;
        
        case Interpreter::BINARY_OPERATOR::LE:
            return lhs <= rhs;
        break;

        case Interpreter::BINARY_OPERATOR::GT:
            return lhs > rhs;
        break;

        case Interpreter::BINARY_OPERATOR::GE:
            return lhs >= rhs;
        break;

        case Interpreter::BINARY_OPERATOR::NE:
            return lhs != rhs;
        break;

        case Interpreter::BINARY_OPERATOR::EQ:
            return lhs == rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ADD:
            return lhs + rhs;
        break;
        
        case Interpreter::BINARY_OPERATOR::SUB:
            return lhs - rhs;
        break;

        case Interpreter::BINARY_OPERATOR::MUL:
            return lhs * rhs;
        break;

        case Interpreter::BINARY_OPERATOR::DIV:
            return lhs / rhs;
        break;

        case Interpreter::BINARY_OPERATOR::MOD:
            return lhs % rhs;
        break;
        
        case Interpreter::BINARY_OPERATOR::EXP:
            return lhs ^ rhs;
        break;

        default:
            throw std::runtime_error("Invalid operator supplied.");
    }
}

std::any Interpreter::visit(AstNode::Expression::Unary& ast) {
    auto expression = ast.getExpression()->accept(*this);
    auto& object = resolve(expression);
    auto op = getUnOpEnum(ast.getOp());
    auto position = ast.getPosition();
    
    switch (op) {
        case Interpreter::UNARY_OPERATOR::NOT:
            return !object;
        break;

        case Interpreter::UNARY_OPERATOR::NEG:
            return -object;
        break;

        case Interpreter::UNARY_OPERATOR::INC:
            if (position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) {
                object = object + 1;
                return object;
            }
            else {
                auto objCopy = object;
                object = object + 1;
                return objCopy;
            }
        break;

        case Interpreter::UNARY_OPERATOR::DEC:
            if (position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) {
                object = object - 1;
                return object;
            }
            else {
                auto objCopy = object;
                object = object - 1;
                return objCopy;
            }
        break;

        default:
            throw std::runtime_error("Invalid operator supplied.");
    }
}

std::any Interpreter::visit(AstNode::Expression::Group& ast) {}

std::any Interpreter::visit(AstNode::Expression::Method& ast) {}

std::any Interpreter::visit(AstNode::Expression::Function& ast) {
    auto& name = ast.getName();
    auto& args = ast.getArguments();
    std::vector<BlsType> argObjects;
    for (auto&& arg : args) {
        auto result = arg->accept(*this);
        argObjects.push_back(resolve(result));
    }
    auto& f = functions.at(name);
    return f(argObjects);
}

std::any Interpreter::visit(AstNode::Expression::Access& ast) {
    auto& object = ast.getObject();
    auto& member = ast.getMember();
    auto& subscript = ast.getSubscript();
    if (member.has_value()) {
        throw std::runtime_error("Member access not yet implemented.");
    }
    else if (subscript.has_value()) {
        throw std::runtime_error("Subscript access not yet implemented.");
    }
    else {
        return std::ref(cs.getLocal(object));
    }
}

std::any Interpreter::visit(AstNode::Expression::Literal& ast) {
    std::any literal;
    const auto convert = overloads {
        [&literal](size_t value) {literal = BlsType((int64_t)value);},
        [&literal](auto value) { literal = BlsType(value); }
    };
    std::visit(convert, ast.getLiteral());
    return literal;
}

std::any Interpreter::visit(AstNode::Expression::List& ast) {}

std::any Interpreter::visit(AstNode::Expression::Set& ast) {}

std::any Interpreter::visit(AstNode::Expression::Map& ast) {}

std::any Interpreter::visit(AstNode::Specifier::Type& ast) {}

BlsType& Interpreter::resolve(std::any& val) {
    if (val.type() == typeid(std::reference_wrapper<BlsType>)) {
        return std::any_cast<std::reference_wrapper<BlsType>>(val);
    }
    else {
        return std::ref(std::any_cast<BlsType&>(val));
    }
}