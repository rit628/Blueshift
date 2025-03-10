#include "interpreter.hpp"
#include "binding_parser.hpp"
#include "bls_types.hpp"
#include "call_stack.hpp"
#include "ast.hpp"
#include "error_types.hpp"
#include "include/Common.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <cstdint>
#include <functional>
#include <memory>
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

    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }

    ast.getSetup()->accept(*this);

    return std::monostate();
}

std::any Interpreter::visit(AstNode::Function::Procedure& ast) {
    auto& functionName = ast.getName();

    auto function = [&ast, this](std::vector<BlsType> args) -> std::any {
        auto& functionName = ast.getName();
        auto& params = ast.getParameters();
        auto& statements = ast.getStatements();
        cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION);
        if (params.size() != args.size()) {
            throw RuntimeError("Invalid number of arguments provided to function call.");
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

std::any Interpreter::visit(AstNode::Function::Oblock& ast) {
    auto& oblockName = ast.getName();

    auto oblock = [&ast, this](std::vector<BlsType> args) -> std::vector<BlsType> {
        auto& oblockName = ast.getName();
        auto& params = ast.getParameters();
        auto& statements = ast.getStatements();
        cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION);
        if (params.size() != args.size()) {
            throw RuntimeError("Invalid number of arguments provided to oblock call.");
        }
        for (int i = 0; i < params.size(); i++) {
            cs.setLocal(params.at(i), args.at(i));
        }

        for (auto&& statement : statements) {
            statement->accept(*this);
        }

        std::vector<BlsType> transformedArgs;
        for (int i = 0; i < params.size(); i++) {
            transformedArgs.push_back(cs.getLocal(params.at(i)));
        }
        cs.popFrame();
        return transformedArgs;
    };
    oblocks.emplace(oblockName, oblock);
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Setup& ast) {
    cs.pushFrame(CallStack<std::string>::Frame::Context::SETUP);
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();
    return true;
}

std::any Interpreter::visit(AstNode::Statement::If& ast) {
    auto checkCondition = [this](AstNode& expr) -> bool {
        auto conditionResult = expr.accept(*this);
        return std::get<bool>(resolve(conditionResult));
    };

    if (checkCondition(*ast.getCondition())) {
        cs.pushFrame(CallStack<std::string>::Frame::Context::CONDITIONAL);
        for (auto&& statement : ast.getBlock()) {
            statement->accept(*this);
        }
        cs.popFrame();
        return BlsType(true);
    }
    else {
        for (auto&& elif : ast.getElseIfStatements()) {
            if (checkCondition(*elif)) {
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
        throw RuntimeError("continue statement outside of loop context.");
    }
    throw ast;
}

std::any Interpreter::visit(AstNode::Statement::Break& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw RuntimeError("break statement outside of loop context.");
    }
    throw ast;
}

std::any Interpreter::visit(AstNode::Statement::Declaration& ast) {
    auto typedObj = ast.getType()->accept(*this);
    auto& name = ast.getName();
    auto& value = ast.getValue();
    if (cs.checkContext(CallStack<std::string>::Frame::Context::SETUP)) {
        auto devtype = std::any_cast<DEVTYPE>(ast.getType()->accept(*this));
        auto binding = value->get()->accept(*this);
        auto& bindStr = std::get<std::string>(resolve(binding));
        auto devDesc = parseDeviceBinding(name, devtype, bindStr);
        deviceDescriptors.emplace(name, devDesc);
    }
    else if (value.has_value()) {
        auto literal = value->get()->accept(*this);
        cs.setLocal(name, resolve(literal));
    }
    else {
        cs.setLocal(name, resolve(typedObj));
    }
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

        case Interpreter::BINARY_OPERATOR::ASSIGN:
            return lhs = rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ASSIGN_ADD:
            return lhs = lhs + rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ASSIGN_SUB:
            return lhs = lhs - rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ASSIGN_MUL:
            return lhs = lhs * rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ASSIGN_DIV:
            return lhs = lhs / rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ASSIGN_MOD:
            return lhs = lhs % rhs;
        break;

        case Interpreter::BINARY_OPERATOR::ASSIGN_EXP:
            return lhs = lhs ^ rhs;
        break;

        default:
            throw RuntimeError("Invalid operator supplied.");
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
            throw RuntimeError("Invalid operator supplied.");
    }
}

std::any Interpreter::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

std::any Interpreter::visit(AstNode::Expression::Method& ast) {
    auto& object = ast.getObject();
    auto& args = ast.getArguments();
    std::vector<BlsType> resolvedArgs;
    for (auto&& arg : args) {
        auto visited = arg->accept(*this);
        resolvedArgs.push_back(resolve(visited));
    }
    auto& methodName = ast.getMethodName();
    auto& operable = std::get<std::shared_ptr<HeapDescriptor>>(cs.getLocal(object));
    if (methodName == "append") {
        dynamic_cast<VectorDescriptor&>(*operable).append(resolvedArgs.at(0));
    }
    else if (methodName == "add") {
        dynamic_cast<MapDescriptor&>(*operable).emplace(resolvedArgs.at(0), resolvedArgs.at(1));
    }
    else if (methodName == "size") {
        return BlsType(operable->getSize());
    }
    else {
        throw RuntimeError("invalid method");
    }
    return std::monostate();
}

std::any Interpreter::visit(AstNode::Expression::Function& ast) {
    auto& name = ast.getName();
    auto& args = ast.getArguments();
    if (cs.checkContext(CallStack<std::string>::Frame::Context::SETUP)) {
        OBlockDesc desc;
        desc.name = name;
        auto& devices = desc.binded_devices;
        for (auto&& arg : args) {
            auto* expr = dynamic_cast<AstNode::Expression::Access*>(arg.get());
            if (expr == nullptr || expr->getMember().has_value() || expr->getSubscript().has_value()) {
                throw RuntimeError("Invalid binding in setup()");
            }
            devices.push_back(deviceDescriptors.at(expr->getObject()));
        }
        oblockDescriptors.push_back(desc);
        return std::monostate();
    }
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
        auto& accessible = std::get<std::shared_ptr<HeapDescriptor>>(cs.getLocal(object));
        auto memberName = BlsType(member.value());
        return std::ref(accessible->access(memberName));
    }
    else if (subscript.has_value()) {
        auto& subscriptable = std::get<std::shared_ptr<HeapDescriptor>>(cs.getLocal(object));
        auto index = subscript->get()->accept(*this);
        return std::ref(subscriptable->access(resolve(index)));
    }
    else {
        return std::ref(cs.getLocal(object));
    }
}

std::any Interpreter::visit(AstNode::Expression::Literal& ast) {
    std::any literal;
    const auto convert = overloads {
        [&literal](int64_t value) {literal = BlsType((int)value);},
        [&literal](double value) {literal = BlsType((float)value);},
        [&literal](auto value) { literal = BlsType(value); }
    };
    std::visit(convert, ast.getLiteral());
    return literal;
}

std::any Interpreter::visit(AstNode::Expression::List& ast) {
    auto list = std::make_shared<VectorDescriptor>(Desctype::ANY);
    auto& elements = ast.getElements();
    for (auto&& element : elements) {
        auto literal = element->accept(*this);
        list->append(resolve(literal));
    }
    return BlsType(list);
}

std::any Interpreter::visit(AstNode::Expression::Set& ast) {
    throw RuntimeError("no support for sets in phase 0");
}

std::any Interpreter::visit(AstNode::Expression::Map& ast) {
    auto map = std::make_shared<MapDescriptor>(Desctype::ANY);
    auto& elements = ast.getElements();
    for (auto&& element : elements) {
        auto key = element.first->accept(*this);
        auto value = element.second->accept(*this);
        map->emplace(resolve(key), resolve(value));
    }
    return BlsType(map);
}

std::any Interpreter::visit(AstNode::Specifier::Type& ast) {
    TYPE primaryType = getTypeEnum(ast.getName());
    const auto& typeArgs = ast.getTypeArgs();
    if (primaryType == TYPE::list_t) {
        if (typeArgs.size() != 1) throw RuntimeError("list may only have a single type argument");
        auto list = std::make_shared<VectorDescriptor>(Desctype::ANY);
        auto arg = typeArgs.at(0)->accept(*this);
        list->append(resolve(arg));
        return BlsType(list);
    }
    else if (primaryType == TYPE::map_t) {
        if (typeArgs.size() != 2) throw RuntimeError("map must have two type arguments");
        auto keyType = getTypeEnum(typeArgs.at(0)->getName());
        auto valType = getTypeEnum(typeArgs.at(1)->getName());
        if (valType == TYPE::void_t) {
            throw RuntimeError("invalid value type for map");
        }
        auto map = std::make_shared<MapDescriptor>(Desctype::ANY);
        auto key = typeArgs.at(0)->accept(*this);
        auto value = typeArgs.at(1)->accept(*this);
        map->emplace(resolve(key), resolve(value));
        return BlsType(map);
    }
    else if (typeArgs.empty()) {
        switch (primaryType) {
            case TYPE::void_t:
                return BlsType(std::monostate());
            break;

            case TYPE::bool_t:
                return BlsType(bool());
            break;

            case TYPE::int_t:
                return BlsType(int());
            break;

            case TYPE::float_t:
                return BlsType(float());
            break;

            case TYPE::string_t:
                return BlsType(std::string());
            break;

            #define DEVTYPE_BEGIN(name) \
            case TYPE::name: \
                return DEVTYPE::name;
            #define ATTRIBUTE(...)
            #define DEVTYPE_END \
            break;
            #include "DEVTYPES.LIST"
            #undef DEVTYPE_BEGIN
            #undef ATTRIBUTE
            #undef DEVTYPE_END

            default:
                throw RuntimeError("container type must include element type arguments");
            break;
        }
    }
    else {
        throw RuntimeError("primitive or devtype cannot include type arguments");
    }
}

BlsType& Interpreter::resolve(std::any& val) {
    if (val.type() == typeid(std::reference_wrapper<BlsType>)) {
        return std::any_cast<std::reference_wrapper<BlsType>>(val);
    }
    else {
        return std::ref(std::any_cast<BlsType&>(val));
    }
}