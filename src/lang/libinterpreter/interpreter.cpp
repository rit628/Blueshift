#include "interpreter.hpp"
#include "binding_parser.hpp"
#include "libtype/bls_types.hpp"
#include "call_stack.hpp"
#include "ast.hpp"
#include "error_types.hpp"
#include "include/Common.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

using namespace BlsLang;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

BlsObject Interpreter::visit(AstNode::Source& ast) {
    for (auto&& procedure : ast.getProcedures()) {
        procedure->accept(*this);
    }

    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }

    return std::monostate();
}

BlsObject Interpreter::visit(AstNode::Function::Procedure& ast) {
    auto& procedureName = ast.getName();

    auto procedure = [&ast](Interpreter& exec, std::vector<BlsType> args) -> BlsType {
        auto& params = ast.getParameters();
        auto& statements = ast.getStatements();
        exec.cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION);
        if (params.size() != args.size()) {
            throw RuntimeError("Invalid number of arguments provided to procedure call.");
        }
        for (int i = 0; i < params.size(); i++) {
            exec.cs.addLocal(params.at(i), args.at(i));
        }

        try {
            for (auto&& statement : statements) {
                statement->accept(exec);
            }
        } 
        catch (BlsObject& ret) {
            BlsType result = exec.resolve(ret);
            exec.cs.popFrame();
            return result;
        }
        exec.cs.popFrame();
        return std::monostate();
    };
    procedures.emplace(procedureName, procedure);
    return std::monostate();
}

BlsObject Interpreter::visit(AstNode::Function::Oblock& ast) {
    auto& oblockName = ast.getName();

    auto oblock = [&ast](Interpreter& exec, std::vector<BlsType> args) -> std::vector<BlsType> {
        auto& params = ast.getParameters();
        auto& statements = ast.getStatements();
        exec.cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION);
        if (params.size() != args.size()) {
            throw RuntimeError("Invalid number of arguments provided to oblock call.");
        }
        for (int i = 0; i < params.size(); i++) {
            exec.cs.addLocal(params.at(i), args.at(i));
        }

        for (auto&& statement : statements) {
            statement->accept(exec);
        }

        std::vector<BlsType> transformedArgs;
        for (int i = 0; i < params.size(); i++) {
            transformedArgs.push_back(exec.cs.getLocal(params.at(i)));
        }
        exec.cs.popFrame();
        return transformedArgs;
    };
    oblocks.emplace(oblockName, oblock);
    return std::monostate();
}

BlsObject Interpreter::visit(AstNode::Setup& ast) {
    return std::monostate(); // setup does not need to be visited in interpreter
}

BlsObject Interpreter::visit(AstNode::Statement::If& ast) {
    auto checkCondition = [this](AstNode& expr) {
        auto conditionResult = resolve(expr.accept(*this));
        return std::get<bool>(conditionResult);
    };

    auto execBlock = [this](std::vector<std::unique_ptr<AstNode::Statement>>& statements) {
        cs.pushFrame(CallStack<std::string>::Frame::Context::CONDITIONAL);
        for (auto&& statement : statements) {
            statement->accept(*this);
        }
        cs.popFrame();
    };

    if (checkCondition(*ast.getCondition())) {
        execBlock(ast.getBlock());
        return std::monostate();
    }
    else {
        for (auto&& elif : ast.getElseIfStatements()) {
            if (checkCondition(*elif->getCondition())) {
                execBlock(elif->getBlock());
                return std::monostate(); // short circuit if elif condition is satisfied
            }
        }
        
        execBlock(ast.getElseBlock());
        return std::monostate();
    }
}

BlsObject Interpreter::visit(AstNode::Statement::For& ast) {
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
            auto conditionResult = resolve(condition->get()->accept(*this));
            return std::get<bool>(conditionResult);
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

BlsObject Interpreter::visit(AstNode::Statement::While& ast) {
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
        auto conditionResult = resolve(condition->accept(*this));
        return std::get<bool>(conditionResult);
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

BlsObject Interpreter::visit(AstNode::Statement::Return& ast) {
    auto& value = ast.getValue();
    if (value.has_value()) {
        auto literal = value->get()->accept(*this);
        throw literal;
    }
    return std::monostate();
}

BlsObject Interpreter::visit(AstNode::Statement::Continue& ast) {
    throw ast;
}

BlsObject Interpreter::visit(AstNode::Statement::Break& ast) {
    throw ast;
}

BlsObject Interpreter::visit(AstNode::Statement::Declaration& ast) {
    auto typedObj = resolve(ast.getType()->accept(*this));
    auto& name = ast.getName();
    auto& value = ast.getValue();
    if (value.has_value()) {
        auto literal = resolve(value->get()->accept(*this));
        typedObj.assign(literal);
    }
    cs.addLocal(name, typedObj);
    return std::monostate();
}

BlsObject Interpreter::visit(AstNode::Statement::Expression& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Interpreter::visit(AstNode::Expression::Binary& ast) {
    auto leftResult = ast.getLeft()->accept(*this);
    auto& lhs = resolve(leftResult);
    auto rightResult = ast.getRight()->accept(*this);
    auto& rhs = resolve(rightResult);

    auto op = getBinOpEnum(ast.getOp());
    switch (op) {
        case BINARY_OPERATOR::OR:
            return lhs || rhs;
        break;

        case BINARY_OPERATOR::AND:
            return lhs && rhs;
        break;

        case BINARY_OPERATOR::LT:
            return lhs < rhs;
        break;
        
        case BINARY_OPERATOR::LE:
            return lhs <= rhs;
        break;

        case BINARY_OPERATOR::GT:
            return lhs > rhs;
        break;

        case BINARY_OPERATOR::GE:
            return lhs >= rhs;
        break;

        case BINARY_OPERATOR::NE:
            return lhs != rhs;
        break;

        case BINARY_OPERATOR::EQ:
            return lhs == rhs;
        break;

        case BINARY_OPERATOR::ADD:
            return lhs + rhs;
        break;
        
        case BINARY_OPERATOR::SUB:
            return lhs - rhs;
        break;

        case BINARY_OPERATOR::MUL:
            return lhs * rhs;
        break;

        case BINARY_OPERATOR::DIV:
            return lhs / rhs;
        break;

        case BINARY_OPERATOR::MOD:
            return lhs % rhs;
        break;
        
        case BINARY_OPERATOR::EXP:
            return lhs ^ rhs;
        break;

        case BINARY_OPERATOR::ASSIGN:
            return lhs.assign(rhs);
        break;

        case BINARY_OPERATOR::ASSIGN_ADD:
            return lhs.assign(lhs + rhs);
        break;

        case BINARY_OPERATOR::ASSIGN_SUB:
            return lhs.assign(lhs - rhs);
        break;

        case BINARY_OPERATOR::ASSIGN_MUL:
            return lhs.assign(lhs * rhs);
        break;

        case BINARY_OPERATOR::ASSIGN_DIV:
            return lhs.assign(lhs / rhs);
        break;

        case BINARY_OPERATOR::ASSIGN_MOD:
            return lhs.assign(lhs % rhs);
        break;

        case BINARY_OPERATOR::ASSIGN_EXP:
            return lhs.assign(lhs ^ rhs);
        break;

        default:
            return std::monostate(); // just for completeness, never reached at runtime
        break;
    }
}

BlsObject Interpreter::visit(AstNode::Expression::Unary& ast) {
    auto expression = ast.getExpression()->accept(*this);
    auto& object = resolve(expression);
    auto op = getUnOpEnum(ast.getOp());
    auto position = ast.getPosition();
    
    switch (op) {
        case UNARY_OPERATOR::NOT:
            return !object;
        break;

        case UNARY_OPERATOR::NEG:
            return -object;
        break;

        case UNARY_OPERATOR::INC:
            if (position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) {
                object.assign(object + 1);
                return object;
            }
            else {
                auto objCopy = object;
                object.assign(object + 1);
                return objCopy;
            }
        break;

        case UNARY_OPERATOR::DEC:
            if (position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) {
                object.assign(object - 1);
                return object;
            }
            else {
                auto objCopy = object;
                object.assign(object - 1);
                return objCopy;
            }
        break;

        default:
            return std::monostate(); // just for completeness, never reached at runtime
        break;
    }
}

BlsObject Interpreter::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Interpreter::visit(AstNode::Expression::Method& ast) {
    auto& objectName = ast.getObject();
    auto& object = cs.getLocal(objectName);
    auto objType = getType(object);
    auto& args = ast.getArguments();
    auto& methodName = ast.getMethodName();

    if (false) { } // short circuit hack
    #define METHOD_BEGIN(name, objectType, ...) \
    else if (objType == TYPE::objectType##_t && methodName == #name) { \
        using argnum [[ maybe_unused ]] = BlsTrap::Detail::objectType##__##name::ARGNUM; \
        if (args.size() != argnum::COUNT) { \
            throw RuntimeError("Invalid number of arguments provided to " + methodName + "."); \
        } \
        return BlsTrap::executeMTRAP<BlsTrap::MCALLNUM::objectType##__##name>(object, {
        #define ARGUMENT(argName, typeArgIdx, type...) \
            resolve(args.at(argnum::argName)->accept(*this)),
        #define METHOD_END \
        }); \
    }
    #include "libtype/include/LIST_METHODS.LIST"
    #include "libtype/include/MAP_METHODS.LIST"
    #undef METHOD_BEGIN
    #undef ARGUMENT
    #undef METHOD_END
    else {
        throw RuntimeError("Invalid method " + methodName + " for " + getTypeName(objType));
    }
}

BlsObject Interpreter::visit(AstNode::Expression::Function& ast) {
    auto& name = ast.getName();
    auto& args = ast.getArguments();
    std::vector<BlsType> argObjects;
    for (auto&& arg : args) {
        auto result = resolve(arg->accept(*this));
        argObjects.push_back(result);
    }
    auto& f = procedures.at(name);
    return f(*this, argObjects);
}

BlsObject Interpreter::visit(AstNode::Expression::Access& ast) {
    auto& object = ast.getObject();
    auto& member = ast.getMember();
    auto& subscript = ast.getSubscript();
    if (member.has_value()) {
        auto& accessible = std::get<std::shared_ptr<HeapDescriptor>>(cs.getLocal(object));
        auto memberName = BlsType(member.value());
        return std::ref(accessible->access(memberName));
    }
    else if (subscript.has_value()) {
        auto& subscriptable = std::get<std::shared_ptr<HeapDescriptor>>(cs.getLocal(object));
        auto index = resolve(subscript->get()->accept(*this));
        return std::ref(subscriptable->access(index));
    }
    else {
        return std::ref(cs.getLocal(object));
    }
}

BlsObject Interpreter::visit(AstNode::Expression::Literal& ast) {
    BlsType literal;
    const auto convert = overloads {
        [&literal](auto& value) { literal = value; }
    };
    std::visit(convert, ast.getLiteral());
    return literal;
}

BlsObject Interpreter::visit(AstNode::Expression::List& ast) {
    auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    for (auto&& element : elements) {
        auto literal = resolve(element->accept(*this));
        list->append(literal);
    }
    return BlsType(list);
}

BlsObject Interpreter::visit(AstNode::Expression::Set& ast) {
    return std::monostate(); // no support for sets in phase 0
}

BlsObject Interpreter::visit(AstNode::Expression::Map& ast) {
    auto map = std::make_shared<MapDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    for (auto&& element : elements) {
        auto key = resolve(element.first->accept(*this));
        auto value = resolve(element.second->accept(*this));
        map->add(key, value);
    }
    return BlsType(map);
}

BlsObject Interpreter::visit(AstNode::Specifier::Type& ast) {
    // type declarations visited for default construction
    TYPE primaryType = getTypeFromName(ast.getName());
    switch (primaryType) {
        case TYPE::void_t:
            return BlsType(std::monostate());
        break;

        case TYPE::bool_t:
            return BlsType(bool(false));
        break;

        case TYPE::int_t:
            return BlsType(int64_t(0));
        break;

        case TYPE::float_t:
            return BlsType(double(0.0));
        break;

        case TYPE::string_t:
            return BlsType(std::string(""));
        break;

        case TYPE::list_t:
            return std::make_shared<VectorDescriptor>(TYPE::ANY);
        break;

        case TYPE::map_t:
            return std::make_shared<MapDescriptor>(TYPE::ANY);
        break;

        #define DEVTYPE_BEGIN(name, ...) \
        case TYPE::name: { \
            using namespace TypeDef; \
            auto devtype = std::make_shared<MapDescriptor>(TYPE::name, TYPE::string_t, TYPE::ANY); \
            BlsType attr, attrVal;
        #define ATTRIBUTE(name, type) \
            attr = BlsType(#name); \
            attrVal = BlsType(type()); \
            devtype->add(attr, attrVal);
        #define DEVTYPE_END \
            return BlsType(devtype); \
        break; \
        }
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END

        default:
            return std::monostate(); // just for completeness, never reached at runtime
        break;
    }
}

BlsObject Interpreter::visit(AstNode::Initializer::Oblock& ast) {
    return std::monostate(); // Oblock initializers do not need to be visited in interpreter
}