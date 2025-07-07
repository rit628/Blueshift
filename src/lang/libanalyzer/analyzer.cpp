#include "ast.hpp"
#include "binding_parser.hpp"
#include "error_types.hpp"
#include "include/Common.hpp"
#include "include/reserved_tokens.hpp"
#include "libtype/typedefs.hpp"
#include "analyzer.hpp"
#include "libtype/bls_types.hpp"
#include "call_stack.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>
#include <boost/range/iterator_range.hpp>
#include <boost/range/combine.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace BlsLang;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

BlsObject Analyzer::visit(AstNode::Source& ast) {
    for (auto&& procedure : ast.getProcedures()) {
        procedure->accept(*this);
    }

    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }

    ast.getSetup()->accept(*this);

    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Function::Procedure& ast) {
    auto& procedureName = ast.getName();
    auto returnType = resolve(ast.getReturnType()->accept(*this));
    
    // add default constructed literal to pool for non-returning control paths
    // will only be necessary for void once control path checking is implemented
    switch (getType(returnType)) {
        case TYPE::void_t:
            addToPool(std::monostate());
        break;

        case TYPE::bool_t:
            addToPool(false);
        break;

        case TYPE::int_t:
            addToPool(0);
        break;

        case TYPE::float_t:
            addToPool(0.0);
        break;

        case TYPE::string_t:
            addToPool("");
        break;

        case TYPE::list_t:
            addToPool(std::make_shared<VectorDescriptor>(TYPE::ANY));
        break;

        case TYPE::map_t:
            addToPool(std::make_shared<MapDescriptor>(TYPE::ANY));
        break;

        default:
        break;
    }
    
    std::vector<BlsType> parameterTypes;
    for (auto&& type : ast.getParameterTypes()) {
        auto typeObject = resolve(type->accept(*this));
        parameterTypes.push_back(typeObject);
    }
    procedures.emplace(procedureName, FunctionSignature(procedureName, returnType, parameterTypes));

    cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION, procedureName);
    auto params = ast.getParameters();
    for (int i = 0; i < params.size(); i++) {
        cs.addLocal(params.at(i), parameterTypes.at(i));
    }
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();

    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Function::Oblock& ast) {
    auto& oblockName = ast.getName();
    
    std::vector<BlsType> parameterTypes;
    for (auto&& type : ast.getParameterTypes()) {
        auto typeObject = resolve(type->accept(*this));
        parameterTypes.push_back(typeObject);
    }
    
    cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION, oblockName);
    auto params = ast.getParameters();
    std::unordered_map<std::string, uint8_t> parameterIndices;
    for (int i = 0; i < params.size(); i++) {
        parameterIndices.emplace(params.at(i), i);
        cs.addLocal(params.at(i), parameterTypes.at(i));
    }
    oblocks.emplace(oblockName, FunctionSignature(oblockName, std::monostate(), parameterTypes, parameterIndices));
    
    OBlockDesc desc;
    desc.name = oblockName;
    desc.binded_devices.resize(params.size());
    oblockDescriptors.emplace(oblockName, std::move(desc));
    for (auto&& option : ast.getConfigOptions()) {
        option->accept(*this);
    }
    
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();

    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Setup& ast) {
    auto completedLiteralPool = std::move(literalPool); // save initial literal pool as setup values arent necessary at runtime
    std::unordered_set<std::string> boundOblocks;

    for (auto&& statement : ast.getStatements()) {
        if (auto* deviceBinding = dynamic_cast<AstNode::Statement::Declaration*>(statement.get())) {
            auto& name = deviceBinding->getName();
            auto devtypeObj = resolve(deviceBinding->getType()->accept(*this));
            auto type = getType(devtypeObj);
            auto& value = deviceBinding->getValue();
            DeviceDescriptor devDesc;

            if (type > TYPE::CONTAINERS_END) {  // use binding parser for devtypes
                if (!value.has_value()) {
                    throw SemanticError("DEVTYPE binding cannot be empty");
                }
    
                auto binding = resolve(value->get()->accept(*this));
                if (!std::holds_alternative<std::string>(binding)) {
                    throw SemanticError("DEVTYPE binding must be a string literal");
                }
                auto& bindStr = std::get<std::string>(binding);
                devDesc = parseDeviceBinding(bindStr);
                devDesc.isVtype = deviceBinding->getModifiers().contains(RESERVED_VIRTUAL);
            }
            else {
                if (!deviceBinding->getModifiers().contains(RESERVED_VIRTUAL)) {
                    throw SemanticError("Non DEVTYPE devices must be declared as virtual.");
                }
                if (value.has_value()) {
                    devtypeObj = resolve(value->get()->accept(*this));
                }
                devDesc.controller = "MASTER";
                devDesc.isVtype = true;
            }
            
            devDesc.device_name = name;
            devDesc.type = type;
            devDesc.initialValue = devtypeObj;
            deviceDescriptors.emplace(name, devDesc);
        }
        else if (auto* statementExpression = dynamic_cast<AstNode::Statement::Expression*>(statement.get())) {
            auto* oblockBinding = dynamic_cast<AstNode::Expression::Function*>(statementExpression->getExpression().get());
            if (!oblockBinding) {
                throw SemanticError("Statement within setup() must be an oblock binding expression or device binding declaration");
            }
            auto& name = oblockBinding->getName();
            auto& args = oblockBinding->getArguments();
            if (!oblocks.contains(name)) {
                throw SemanticError(name + " does not refer to an oblock");
            }
            auto argc = oblocks.at(name).parameterTypes.size();
            if (argc != args.size()) {
                throw SemanticError("Invalid number of arguments supplied to " + name + " (received " + std::to_string(args.size()) + " expected " + std::to_string(argc) + " )");
            }
            auto& desc = oblockDescriptors.at(name);
            desc.name = name;
            auto& devices = desc.binded_devices;
            for (size_t i = 0; i < args.size(); i++) {
                auto* expr = dynamic_cast<AstNode::Expression::Access*>(args.at(i).get());
                if (expr == nullptr || expr->getMember().has_value() || expr->getSubscript().has_value()) {
                    throw SemanticError("Invalid oblock binding in setup()");
                }
                try {
                    auto& declaredDev = deviceDescriptors.at(expr->getObject());
                    auto& dev = devices.at(i);
                    dev.device_name = declaredDev.device_name;
                    dev.initialValue = declaredDev.initialValue;
                    dev.type = declaredDev.type;
                    dev.controller = declaredDev.controller;
                    dev.port_maps = declaredDev.port_maps;
                    dev.isVtype = declaredDev.isVtype;
                }
                catch (std::out_of_range) {
                    throw SemanticError(expr->getObject() + " does not refer to a device binding");
                }
            }
            // update trigger rules to point to device names rather than alias indices
            for (auto&& trigger : desc.triggers) {
                for (auto&& parameter : trigger.rule) {
                    parameter = desc.binded_devices.at(std::stoi(parameter)).device_name;
                }
            }
            boundOblocks.emplace(name);
        }
        else {
            throw SemanticError("Statement within setup() must be an oblock binding expression or device binding declaration");
        }
    }

    std::erase_if(oblockDescriptors, [&boundOblocks](const auto& element) {
        return !boundOblocks.contains(element.first);
    }); // get rid of unbound oblocks
    literalPool = std::move(completedLiteralPool);
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::If& ast) {
    auto& condition = ast.getCondition();
    auto& statements = ast.getBlock();
    auto& elifStatements = ast.getElseIfStatements();
    auto& elseBlock = ast.getElseBlock();

    auto execBlock = [this](std::vector<std::unique_ptr<AstNode::Statement>>& statements) {
        cs.pushFrame(CallStack<std::string>::Frame::Context::CONDITIONAL);
        for (auto&& statement : statements) {
            statement->accept(*this);
        }
        cs.popFrame();
    };

    auto conditionResult = resolve(condition->accept(*this));
    if (getType(conditionResult) != TYPE::bool_t) {
        throw SemanticError("Condition must resolve to a boolean value");
    }
    // TODO: add checks for unreachable code
    execBlock(statements);
    for (auto&& elif : elifStatements) {
        elif->accept(*this);
    }
    execBlock(elseBlock);
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::For& ast) {
    auto& initStatement = ast.getInitStatement();
    auto& condition = ast.getCondition();
    auto& incrementExpression = ast.getIncrementExpression();
    auto& statements = ast.getBlock();

    cs.pushFrame(CallStack<std::string>::Frame::Context::LOOP);
    if (initStatement.has_value()) {
        initStatement->get()->accept(*this);
    }
    if (condition.has_value()) {
        auto conditionResult = resolve(condition->get()->accept(*this));
        if (getType(conditionResult) != TYPE::bool_t) {
            throw SemanticError("Condition must resolve to a boolean value");
        }
    }
    if (incrementExpression.has_value()) {
        incrementExpression->get()->accept(*this);
    }
    // TODO: add checks for unreachable code
    for (auto&& statement : statements) {
        statement->accept(*this);
    }
    cs.popFrame();
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::While& ast) {
    auto& condition = ast.getCondition();
    auto& statements = ast.getBlock();

    cs.pushFrame(CallStack<std::string>::Frame::Context::LOOP);
    auto conditionResult = resolve(condition->accept(*this));
    if (getType(conditionResult) != TYPE::bool_t) {
        throw SemanticError("Condition must resolve to a boolean value");
    }
    // TODO: add checks for unreachable code
    for (auto&& statement : statements) {
        statement->accept(*this);
    }
    cs.popFrame();
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::Return& ast) {
    auto& functionName = cs.getFrameName();
    auto parentSignature = (procedures.contains(functionName)) ? procedures.at(functionName) : oblocks.at(functionName);
    auto expectedReturnType = getType(parentSignature.returnType);
    auto& value = ast.getValue();
    if (value.has_value()) {
        if (expectedReturnType == TYPE::void_t) {
            throw SemanticError("void function should not return a value");
        }
        auto returnType = resolve(value->get()->accept(*this));
        if (!typeCompatible(parentSignature.returnType, returnType)) {
            throw SemanticError("Invalid return type for " + functionName);
        }
    }
    else if (expectedReturnType != TYPE::void_t) {
        throw SemanticError("non void function should return a value");
    }
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::Continue& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw SemanticError("continue statement outside of loop context.");
    }
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::Break& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw SemanticError("break statement outside of loop context.");
    }
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::Declaration& ast) {
    auto typedObj = resolve(ast.getType()->accept(*this));
    auto& name = ast.getName();
    auto& value = ast.getValue();
    if (value.has_value()) {
        auto literal = resolve(value->get()->accept(*this));
        if (!typeCompatible(typedObj, literal)) {
            throw SemanticError("Invalid initialization for declaration");
        }
        typedObj.assign(literal);
    }
    cs.addLocal(name, typedObj);
    ast.getLocalIndex() = cs.getLocalIndex(name);
    return std::monostate();
}

BlsObject Analyzer::visit(AstNode::Statement::Expression& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Analyzer::visit(AstNode::Expression::Binary& ast) {
    auto leftResult = ast.getLeft()->accept(*this);
    auto rightResult = ast.getRight()->accept(*this);
    auto& lhs = resolve(leftResult);
    auto& rhs = resolve(rightResult);

    if (!typeCompatible(lhs, rhs)) {
        throw SemanticError("Invalid operands for binary expression");
    }

    auto op = getBinOpEnum(ast.getOp());
    if ((op >= BINARY_OPERATOR::ASSIGN) && std::holds_alternative<BlsType>(leftResult)) {
        throw SemanticError("Assignments to temporary not permitted");
    }
    // operator type checking done by overload specialization
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
            throw SemanticError("Invalid operator supplied.");
        break;
    }
}

BlsObject Analyzer::visit(AstNode::Expression::Unary& ast) {
    auto expression = ast.getExpression()->accept(*this);
    auto& object = resolve(expression);
    auto op = getUnOpEnum(ast.getOp());
    auto position = ast.getPosition();

    if (op >= UNARY_OPERATOR::INC) {
        addToPool(1); // literal 1 needs to be pushed for increment/decrement
        if (std::holds_alternative<BlsType>(expression)) {
            throw SemanticError("Assignments to temporary not permitted");
        }
    }
    
    // operator type checking done by overload specialization
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
            throw SemanticError("Invalid operator supplied.");
        break;
    }
}

BlsObject Analyzer::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Analyzer::visit(AstNode::Expression::Method& ast) {
    auto& objectName = ast.getObject();
    auto& object = cs.getLocal(objectName);
    auto& methodName = ast.getMethodName();
    auto& methodArgs = ast.getArguments();
    auto objType = getType(object);
    ast.getLocalIndex() = cs.getLocalIndex(objectName);
    if (objType < TYPE::PRIMITIVES_END || objType > TYPE::CONTAINERS_END) {
        throw SemanticError("Methods may only be applied on container type objects.");
    }
    auto& operable = std::get<std::shared_ptr<HeapDescriptor>>(object);

    static auto deduceType = []<int typeArgIdx, typename T>(std::shared_ptr<HeapDescriptor>& operable) -> BlsType {
        using namespace TypeDef;
        if constexpr (TypeDef::BlueshiftType<T>) {
            return createBlsType(converted_t<T>());
        }
        else {
            return operable->getSampleElement().at(typeArgIdx);
        }
    };

    if (false) { } // short circuit hack
    #define METHOD_BEGIN(name, objectType, typeArgIdx, returnType...) \
    else if (objType == TYPE::objectType##_t && methodName == #name) { \
        using namespace TypeDef; \
        using argnum [[ maybe_unused ]] = BlsTrap::Detail::objectType##__##name::ARGNUM; \
        BlsType result = deduceType.operator()<typeArgIdx, returnType>(operable);
        #define ARGUMENT(argName, typeArgIdx, type...) \
        if (methodArgs.size() != argnum::COUNT) { \
            throw SemanticError("Invalid number of arguments provided to " + methodName + "."); \
        } \
        auto argName = resolve(methodArgs.at(argnum::argName)->accept(*this)); \
        BlsType expected_##argName = deduceType.operator()<typeArgIdx, type>(operable); \
        if (!typeCompatible(argName, expected_##argName)) { \
            throw SemanticError("Invalid type for argument " #argName "."); \
        }
        #define METHOD_END \
        ast.getObjectType() = objType; \
        return result; \
    }
    #include "libtype/include/LIST_METHODS.LIST"
    #include "libtype/include/MAP_METHODS.LIST"
    #undef METHOD_BEGIN
    #undef ARGUMENT
    #undef METHOD_END
    else {
        throw SemanticError("Invalid method " + methodName + " for " + getTypeName(objType));
    }
}

BlsObject Analyzer::visit(AstNode::Expression::Function& ast) {
    auto& name = ast.getName();
    auto& args = ast.getArguments();
    if (!procedures.contains(name)) {
        throw SemanticError("Procedure " + name + " is not defined.");
    }
    auto& signature = procedures.at(name);

    if (signature.variadic) {
        for (auto&& arg : args) {
            arg->accept(*this);
        }
    }
    else {
        if (signature.parameterTypes.size() != args.size()) {
            throw SemanticError("Invalid number of arguments passed into " + name);
        }
        for (auto&& [argType, arg] : boost::combine(signature.parameterTypes, args)) {
            auto result = resolve(arg->accept(*this));
            if (!typeCompatible(argType, result)) {
                throw SemanticError("Invalid argument provided to " + name);
            }
        }
    }
    return signature.returnType;
}

BlsObject Analyzer::visit(AstNode::Expression::Access& ast) {
    auto& objectName = ast.getObject();
    auto& object = cs.getLocal(objectName);
    auto& member = ast.getMember();
    auto& subscript = ast.getSubscript();
    ast.getLocalIndex() = cs.getLocalIndex(objectName);
    if (member.has_value()) {
        switch (getType(object)) {
            #define DEVTYPE_BEGIN(name) \
            case TYPE::name: \
                if (false) { } // trick for short circuiting
            #define ATTRIBUTE(name, ...) \
                else if (member.value() == #name) { }
            #define DEVTYPE_END \
            else { \
                throw SemanticError("Invalid attribute for devtype"); \
            } \
            break;
            #include "DEVTYPES.LIST"
            #undef DEVTYPE_BEGIN
            #undef ATTRIBUTE
            #undef DEVTYPE_END
            default:
                throw SemanticError("Attribute access only possible on devtypes");
            break;
        }
        auto& accessible = std::get<std::shared_ptr<HeapDescriptor>>(object);
        auto memberName = BlsType(member.value());
        addToPool(memberName); // member accesses need string literal representation of member at runtime
        return std::ref(accessible->access(memberName));
    }
    else if (subscript.has_value()) {
        auto objType = getType(object);
        if (objType < TYPE::PRIMITIVES_END || objType > TYPE::CONTAINERS_END) {
            throw SemanticError("Subscript access only possible on container type objects");
        }
        auto& subscriptable = std::get<std::shared_ptr<HeapDescriptor>>(object);
        auto index = resolve(subscript->get()->accept(*this));
        return std::ref(subscriptable->getSampleElement().at(0));
    }
    else {
        return std::ref(object);
    }
}

BlsObject Analyzer::visit(AstNode::Expression::Literal& ast) {
    BlsType literal;
    const auto convert = overloads {
        [&literal](auto& value) { literal = value; }
    };
    std::visit(convert, ast.getLiteral());
    addToPool(literal);
    return literal;
}

BlsObject Analyzer::visit(AstNode::Expression::List& ast) {
    auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
    auto listLiteral = std::make_shared<VectorDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    BlsType initIdx = 0;
    auto addElement = [&, this](size_t index, auto& element) {
        auto literal = resolve(element->accept(*this));
        list->append(literal);
        auto& valExpressionType = typeid(*element);
        if (valExpressionType == typeid(AstNode::Expression::Literal)
            || valExpressionType == typeid(AstNode::Expression::List)
            || valExpressionType == typeid(AstNode::Expression::Map)) { // element can be added at compile time
                listLiteral->append(literal);
        }
        else {
            BlsType null = std::monostate();
            addToPool(int64_t(index)); // literal integer representing index needs to be available at runtime for population
            listLiteral->append(null);
        }
        return literal;
    };

    if (elements.size() > 0) {
        addElement(0, elements.at(0));
        list->setCont(getType(list->access(initIdx)));
        list->getSampleElement().assign({list->access(initIdx)});
        for (size_t i = 1; i < elements.size(); i++) {
            auto value = addElement(i, elements.at(i));
            if (!typeCompatible(list->access(initIdx), value)) {
                throw SemanticError("List literal declaration must be of homogeneous type.");
            }
        }
    }

    addToPool(listLiteral);
    ast.getLiteral() = listLiteral;
    return list;
}

BlsObject Analyzer::visit(AstNode::Expression::Set& ast) {
    throw SemanticError("no support for sets in phase 0");
}

BlsObject Analyzer::visit(AstNode::Expression::Map& ast) {
    auto map = std::make_shared<MapDescriptor>(TYPE::ANY);
    auto mapLiteral = std::make_shared<MapDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    BlsType initKey, initVal;
    auto addElement = [&, this](auto& element) {
        auto key = resolve(element.first->accept(*this));
        auto keyType = getType(key);
        if (keyType > TYPE::PRIMITIVES_END) {
            throw SemanticError("Invalid key type for map");
        }
        auto value = resolve(element.second->accept(*this));
        map->add(key, value);
        auto& keyExpressionType = typeid(*element.first);
        if (keyExpressionType == typeid(AstNode::Expression::Literal)) { // key can be added at compile time
            auto& valExpressionType = typeid(*element.second);
            if (valExpressionType == typeid(AstNode::Expression::Literal)
             || valExpressionType == typeid(AstNode::Expression::List)
             || valExpressionType == typeid(AstNode::Expression::Map)) { // value can be added at compile time
                mapLiteral->add(key, value);
            }
            else {
                mapLiteral->add(key, std::monostate());
            }
        }
        return std::pair(key, value);
    };

    if (elements.size() > 0) {
        std::tie(initKey, initVal) = addElement(elements.at(0));
        map->setCont(getType(initVal));
        map->getSampleElement().assign({initKey, initVal});
        for (auto&& element : boost::make_iterator_range(elements.begin() + 1, elements.end())) {
            auto&& [key, value] = addElement(element);
            if (!typeCompatible(initKey, key)
             || !typeCompatible(initVal,value)) {
                throw SemanticError("Map literal declaration must be of homogeneous type.");
            }
        }
    }

    addToPool(mapLiteral);
    ast.getLiteral() = mapLiteral;

    return map;
}

BlsObject Analyzer::visit(AstNode::Specifier::Type& ast) {
    TYPE primaryType = getTypeFromName(ast.getName());
    const auto& typeArgs = ast.getTypeArgs();
    switch (primaryType) {
        // explicit default constructors for declaration
        case TYPE::void_t:
            if (!typeArgs.empty()) throw SemanticError("Primitives cannot include type arguments.");
            return BlsType(std::monostate());
        break;

        case TYPE::bool_t:
            if (!typeArgs.empty()) throw SemanticError("Primitives cannot include type arguments.");
            return BlsType(bool(false));
        break;

        case TYPE::int_t:
            if (!typeArgs.empty()) throw SemanticError("Primitives cannot include type arguments.");
            return BlsType(int64_t(0));
        break;

        case TYPE::float_t:
            if (!typeArgs.empty()) throw SemanticError("Primitives cannot include type arguments.");
            return BlsType(double(0.0));
        break;

        case TYPE::string_t:
            if (!typeArgs.empty()) throw SemanticError("Primitives cannot include type arguments.");
            return BlsType(std::string(""));
        break;

        case TYPE::list_t: {
            if (typeArgs.size() != 1) throw SemanticError("List may only have a single type argument.");
            auto argType = getTypeFromName(typeArgs.at(0)->getName());
            auto list = std::make_shared<VectorDescriptor>(argType);
            auto arg = resolve(typeArgs.at(0)->accept(*this));
            list->getSampleElement().assign({arg});
            return list;
        }
        break;

        case TYPE::map_t: {
            if (typeArgs.size() != 2) throw SemanticError("Map must have two type arguments.");
            auto keyType = getTypeFromName(typeArgs.at(0)->getName());
            if (keyType > TYPE::PRIMITIVES_END || keyType == TYPE::void_t) {
                throw SemanticError("Invalid key type for map.");
            }
            auto valType = getTypeFromName(typeArgs.at(1)->getName());
            if (valType == TYPE::void_t) {
                throw SemanticError("Invalid value type for map.");
            }
            auto map = std::make_shared<MapDescriptor>(TYPE::map_t, keyType, valType);
            auto key = resolve(typeArgs.at(0)->accept(*this));
            auto value = resolve(typeArgs.at(1)->accept(*this));
            map->getSampleElement().assign({key, value});
            return BlsType(map);
        }
        break;

        #define DEVTYPE_BEGIN(name) \
        case TYPE::name: { \
            using namespace TypeDef; \
            if (!typeArgs.empty()) throw SemanticError("Devtypes cannot include type arguments."); \
            auto devtype = std::make_shared<MapDescriptor>(TYPE::name, TYPE::string_t, TYPE::ANY); \
            BlsType attr, attrVal;
        #define ATTRIBUTE(name, type) \
            attr = BlsType(#name); \
            attrVal = BlsType(type()); \
            devtype->add(attr, attrVal);
        #define DEVTYPE_END \
            return BlsType(devtype); \
        } \
        break;
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END

        default:
            throw SemanticError("Invalid type: " + ast.getName());
        break;
    }
}

BlsObject Analyzer::visit(AstNode::Initializer::Oblock& ast) {
    auto& oblockName = cs.getFrameName();
    auto& desc = oblockDescriptors.at(oblockName);
    auto& signature = oblocks.at(oblockName);
    auto& boundDevices = desc.binded_devices;
    auto& parameterIndices = signature.parameterIndices;
    auto& option = ast.getOption();
    auto& args = ast.getArgs();
    if (option == "triggerOn") {
        if (args.empty()) {
            throw SemanticError("At least one argument must be supplied to triggerOn.");
        }

        auto updateRule = [this, &parameterIndices](std::unique_ptr<AstNode::Expression>& arg, std::vector<std::string>& rule) {
            if (auto* resolvedArg = dynamic_cast<AstNode::Expression::Access*>(arg.get())) {
                auto& param = resolvedArg->getObject();
                cs.getLocal(param); // check that param exists
                // push back index of param to convert to device name in setup() visitor
                rule.push_back(std::to_string(parameterIndices.at(param)));
            }
            else {
                throw SemanticError("Trigger rules must be oblock parameters or list of oblock parameters.");
            }
        };

        auto createRule = [&updateRule](std::unique_ptr<AstNode::Expression>& ruleExpr, std::vector<std::string>& rule) {
            if (auto* resolvedList = dynamic_cast<AstNode::Expression::List*>(ruleExpr.get())) {
                for (auto&& element : resolvedList->getElements()) {
                    updateRule(element, rule); // create conjunction rule
                }
            }
            else {
                updateRule(ruleExpr, rule); // create singleton rule
            }
        };

        for (auto&& arg : args) {
            std::vector<std::string> rule;
            std::optional<std::string> id = std::nullopt;
            uint8_t priority = 1;
            if (auto* resolvedMap = dynamic_cast<AstNode::Expression::Map*>(arg.get())) { // verbose syntax
                for (auto&& [attrExpr, valExpr] : resolvedMap->getElements()) {
                    auto* attrVal = dynamic_cast<AstNode::Expression::Literal*>(attrExpr.get());
                    if (!attrVal || !std::holds_alternative<std::string>(attrVal->getLiteral())) {
                        throw SemanticError("Trigger attributes must be given as strings.");
                    }
                    auto attribute = std::get<std::string>(attrVal->getLiteral());
                    boost::algorithm::to_lower(attribute);
                    if (attribute == "id") {
                        auto* idVal = dynamic_cast<AstNode::Expression::Literal*>(valExpr.get());
                        if (!idVal || !std::holds_alternative<std::string>(idVal->getLiteral())) {
                            throw SemanticError("id attribute must be a string.");
                        }
                        id = std::get<std::string>(idVal->getLiteral());
                    }
                    else if (attribute == "priority") {
                        auto* priorityVal = dynamic_cast<AstNode::Expression::Literal*>(valExpr.get());
                        if (!priorityVal || !std::holds_alternative<int64_t>(priorityVal->getLiteral())) {
                            throw SemanticError("priority attribute must be an integer.");
                        }
                        priority = std::get<int64_t>(priorityVal->getLiteral());
                    }
                    else if (attribute == "rule") {
                        createRule(valExpr, rule);
                    }
                    else {
                        // get original string (no lowercasing) for error clarity
                        throw SemanticError("Invalid attribute " + std::get<std::string>(attrVal->getLiteral()) + " for trigger declaration.");
                    }
                }
                if (rule.empty()) {
                    throw SemanticError("Trigger declaration must provide a rule attribute.");
                }
            }
            else { // shortcut syntax
                createRule(arg, rule);
            }
            desc.triggers.push_back(TriggerData{std::move(rule), std::move(id), std::move(priority)});
        }
    }
    else if (option == "constPollOn") {
        if (args.size() != 1) {
            throw SemanticError("Exactly one argument must be supplied to constPoll.");
        }
        auto* pollMap = dynamic_cast<AstNode::Expression::Map*>(args.at(0).get());
        if (!pollMap) {
            throw SemanticError("constPoll must be supplied a mapping of oblock parameters to polling rates.");
        }
        for (auto&& [key, value] : pollMap->getElements()) {
            auto* paramExpr = dynamic_cast<AstNode::Expression::Access*>(key.get());
            auto* rateExpr = dynamic_cast<AstNode::Expression::Literal*>(value.get());
            if (!paramExpr) {
                throw SemanticError("Mapping keys must be oblock parameters.");
            }
            if (!rateExpr
             || std::holds_alternative<std::string>(rateExpr->getLiteral())
             || std::holds_alternative<bool>(rateExpr->getLiteral())) {
                throw SemanticError("Polling rate values must be integers or floats.");
            }
            auto& param = paramExpr->getObject();
            auto rate = resolve(rateExpr->accept(*this));
            literalPool.erase(rate);
            
            auto& dev = boundDevices.at(parameterIndices.at(param));
            dev.polling_period = int64_t(rate);
            dev.isConst = true;
        }
    }
    else if (option == "dropReadOn") {
        if (args.empty()) {
            throw SemanticError("dropReadOn takes at least one argument.");
        }
        for (auto&& arg : args) {
            if (auto* parameter = dynamic_cast<AstNode::Expression::Access*>(arg.get())) {
                auto& alias = parameter->getObject();
                cs.getLocal(alias); // check that param exists
                boundDevices.at(parameterIndices.at(alias)).dropRead = true;
            }
        }
    }
    else if (option == "dropWriteOn") {
        if (args.empty()) {
            throw SemanticError("dropWriteOn takes at least one argument.");
        }
        for (auto&& arg : args) {
            if (auto* parameter = dynamic_cast<AstNode::Expression::Access*>(arg.get())) {
                auto& alias = parameter->getObject();
                cs.getLocal(alias); // check that param exists
                boundDevices.at(parameterIndices.at(alias)).dropWrite = true;
            }
        }
    }
    else {
        throw SemanticError("Invalid oblock configuration option: " + option + ".");
    }
    return std::monostate();
} 