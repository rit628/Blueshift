#include "ast.hpp"
#include "binding_parser.hpp"
#include "error_types.hpp"
#include "include/Common.hpp"
#include "libtypes/typedefs.hpp"
#include "analyzer.hpp"
#include "libtypes/bls_types.hpp"
#include "call_stack.hpp"
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include <boost/range/iterator_range.hpp>
#include <boost/range/combine.hpp>

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
    auto returnType = resolve(ast.getReturnType()->get()->accept(*this));
    
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
    oblocks.emplace(oblockName, FunctionSignature(oblockName, std::monostate(), parameterTypes));
    oblockDescriptors.emplace(oblockName, OBlockDesc()); // create empty descriptor here to ensure all oblocks are accounted for even if some are not bound

    cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION, oblockName);
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

BlsObject Analyzer::visit(AstNode::Setup& ast) {
    for (auto&& statement : ast.getStatements()) {
        if (auto* deviceBinding = dynamic_cast<AstNode::Statement::Declaration*>(statement.get())) {
            auto& name = deviceBinding->getName();
            auto devtypeObj = resolve(deviceBinding->getType()->accept(*this));
            auto devtype = static_cast<DEVTYPE>(getType(devtypeObj));

            auto& value = deviceBinding->getValue();
            if (!value.has_value()) {
                throw SemanticError("DEVTYPE binding cannot be empty");
            }

            auto binding = resolve(value->get()->accept(*this));
            if (!std::holds_alternative<std::string>(binding)) {
                throw SemanticError("DEVTYPE binding must be a string literal");
            }
            auto& bindStr = std::get<std::string>(binding);
            auto devDesc = parseDeviceBinding(name, devtype, bindStr);
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
            OBlockDesc desc;
            desc.name = name;
            auto& devices = desc.binded_devices;
            for (auto&& arg : args) {
                auto* expr = dynamic_cast<AstNode::Expression::Access*>(arg.get());
                if (expr == nullptr || expr->getMember().has_value() || expr->getSubscript().has_value()) {
                    throw SemanticError("Invalid oblock binding in setup()");
                }
                try {
                    devices.push_back(deviceDescriptors.at(expr->getObject()));
                }
                catch (std::out_of_range) {
                    throw RuntimeError(expr->getObject() + " does not refer to a device binding");
                }
            }
            oblockDescriptors.at(name) = std::move(desc);
        }
        else {
            throw SemanticError("Statement within setup() must be an oblock binding expression or device binding declaration");
        }
    }
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
        cs.addLocal(name, literal);
    }
    else {
        cs.addLocal(name, typedObj);
    }
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
            return lhs = rhs;
        break;

        case BINARY_OPERATOR::ASSIGN_ADD:
            return lhs = lhs + rhs;
        break;

        case BINARY_OPERATOR::ASSIGN_SUB:
            return lhs = lhs - rhs;
        break;

        case BINARY_OPERATOR::ASSIGN_MUL:
            return lhs = lhs * rhs;
        break;

        case BINARY_OPERATOR::ASSIGN_DIV:
            return lhs = lhs / rhs;
        break;

        case BINARY_OPERATOR::ASSIGN_MOD:
            return lhs = lhs % rhs;
        break;

        case BINARY_OPERATOR::ASSIGN_EXP:
            return lhs = lhs ^ rhs;
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

    if ((op >= UNARY_OPERATOR::INC) && std::holds_alternative<BlsType>(expression)) {
        throw SemanticError("Assignments to temporary not permitted");
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
                object = object + 1;
                return object;
            }
            else {
                auto objCopy = object;
                object = object + 1;
                return objCopy;
            }
        break;

        case UNARY_OPERATOR::DEC:
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
    if (objType < TYPE::PRIMITIVE_COUNT || objType > TYPE::CONTAINER_COUNT) {
        throw SemanticError("Methods may only be applied on container type objects.");
    }
    std::vector<BlsType> resolvedArgs;
    for (auto&& arg : methodArgs) {
        auto visited = resolve(arg->accept(*this));
        resolvedArgs.push_back(visited);
    }
    auto& operable = std::get<std::shared_ptr<HeapDescriptor>>(object);
    // temporary solution for method type checking; not scalable with arbitrary number of methods
    if (methodName == "append" && operable->getType() != TYPE::list_t) {
        throw SemanticError("append may only be used on list type objects");
    }
    else if (methodName == "add" && operable->getType() != TYPE::map_t) {
        throw SemanticError("add may only be used on map type objects");
    }
    else if (methodName == "size") {
        return BlsType(operable->getSize());
    }
    else {
        throw SemanticError("invalid method");
    }
    return std::monostate();
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
        return std::ref(accessible->access(memberName));
    }
    else if (subscript.has_value()) {
        auto objType = getType(object);
        if (objType < TYPE::PRIMITIVE_COUNT || objType > TYPE::CONTAINER_COUNT) {
            throw SemanticError("Subscript access only possible on container type objects");
        }
        auto& subscriptable = std::get<std::shared_ptr<HeapDescriptor>>(object);
        auto index = resolve(subscript->get()->accept(*this));
        return std::ref(subscriptable->access(index));
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
    if (!literalPool.contains(literal)) {
        literalPool.emplace(literal, literalCount++);
    }
    return literal;
}

BlsObject Analyzer::visit(AstNode::Expression::List& ast) {
    auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
    auto listLiteral = std::make_shared<VectorDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    BlsType initIdx = 0;
    auto addElement = [&, this](auto& element) {
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
            listLiteral->append(null);
        }
        return literal;
    };

    if (elements.size() > 0) {
        addElement(elements.at(0));
        list->setCont(getType(list->access(initIdx)));
        list->getSampleElement() = list->access(initIdx);
        for (auto&& element : boost::make_iterator_range(elements.begin() + 1, elements.end())) {
            auto value = addElement(element);
            if (!typeCompatible(list->access(initIdx), value)) {
                throw SemanticError("List literal declaration must be of homogeneous type.");
            }
        }
    }

    if (!literalPool.contains(listLiteral)) {
        literalPool.emplace(listLiteral, literalCount++);
    }

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
        if (keyType > TYPE::PRIMITIVE_COUNT) {
            throw SemanticError("Invalid key type for map");
        }
        auto value = resolve(element.second->accept(*this));
        map->emplace(key, value);
        auto& keyExpressionType = typeid(*element.first);
        if (keyExpressionType == typeid(AstNode::Expression::Literal)) { // key can be added at compile time
            auto& valExpressionType = typeid(*element.second);
            if (valExpressionType == typeid(AstNode::Expression::Literal)
             || valExpressionType == typeid(AstNode::Expression::List)
             || valExpressionType == typeid(AstNode::Expression::Map)) { // value can be added at compile time
                mapLiteral->emplace(key, value);
            }
            else {
                BlsType null = std::monostate();
                mapLiteral->emplace(key, null);
            }
        }
        return std::pair(key, value);
    };

    if (elements.size() > 0) {
        std::tie(initKey, initVal) = addElement(elements.at(0));
        map->setCont(getType(initVal));
        map->getSampleElement() = initVal;
        for (auto&& element : boost::make_iterator_range(elements.begin() + 1, elements.end())) {
            auto&& [key, value] = addElement(element);
            if (!typeCompatible(initKey, key)
             || !typeCompatible(initVal,value)) {
                throw SemanticError("Map literal declaration must be of homogeneous type.");
            }
        }
    }

    if (!literalPool.contains(mapLiteral)) {
        literalPool.emplace(mapLiteral, literalCount++);
    }

    ast.getLiteral() = mapLiteral;

    return map;
}

BlsObject Analyzer::visit(AstNode::Specifier::Type& ast) {
    TYPE primaryType = getTypeFromName(ast.getName());
    const auto& typeArgs = ast.getTypeArgs();
    if (primaryType == TYPE::list_t) {
        if (typeArgs.size() != 1) throw SemanticError("List may only have a single type argument.");
        auto argType = getTypeFromName(typeArgs.at(0)->getName());
        auto list = std::make_shared<VectorDescriptor>(argType);
        auto arg = resolve(typeArgs.at(0)->accept(*this));
        list->append(arg);
        list->getSampleElement() = arg;
        return list;
    }
    else if (primaryType == TYPE::map_t) {
        if (typeArgs.size() != 2) throw SemanticError("Map must have two type arguments.");
        auto keyType = getTypeFromName(typeArgs.at(0)->getName());
        if (keyType > TYPE::PRIMITIVE_COUNT || keyType == TYPE::void_t) {
            throw SemanticError("Invalid key type for map.");
        }
        auto valType = getTypeFromName(typeArgs.at(1)->getName());
        if (valType == TYPE::void_t) {
            throw SemanticError("Invalid value type for map.");
        }
        auto map = std::make_shared<MapDescriptor>(TYPE::map_t, keyType, valType);
        auto key = resolve(typeArgs.at(0)->accept(*this));
        auto value = resolve(typeArgs.at(1)->accept(*this));
        map->emplace(key, value);
        map->getSampleElement() = value;
        return BlsType(map);
    }
    else if (typeArgs.empty()) {
        switch (primaryType) {
            // explicit default constructors for declaration
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

            #define DEVTYPE_BEGIN(name) \
            case TYPE::name: { \
                using namespace TypeDef; \
                auto devtype = std::make_shared<MapDescriptor>(TYPE::name, TYPE::string_t, TYPE::ANY); \
                BlsType attr, attrVal;
            #define ATTRIBUTE(name, type) \
                attr = BlsType(#name); \
                attrVal = BlsType(type()); \
                devtype->emplace(attr, attrVal);
            #define DEVTYPE_END \
                return BlsType(devtype); \
            break; \
            }
            #include "DEVTYPES.LIST"
            #undef DEVTYPE_BEGIN
            #undef ATTRIBUTE
            #undef DEVTYPE_END

            case TYPE::ANY:
            case TYPE::NONE:
            case TYPE::COUNT:
                throw SemanticError("Invalid type: " + ast.getName());
            break;

            default:
                throw SemanticError("Container type must include element type arguments.");
            break;
        }
    }
    else {
        throw SemanticError("Primitive or devtype cannot include type arguments.");
    }
}