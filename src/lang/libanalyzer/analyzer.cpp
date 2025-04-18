#include "error_types.hpp"
#include "libtypes/typedefs.hpp"
#include "analyzer.hpp"
#include "libtypes/bls_types.hpp"
#include "call_stack.hpp"
#include <any>
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

std::any Analyzer::visit(AstNode::Source& ast) {
    for (auto&& procedure : ast.getProcedures()) {
        procedure->accept(*this);
    }

    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }

    // ast.getSetup()->accept(*this);

    return std::monostate();
}

std::any Analyzer::visit(AstNode::Function::Procedure& ast) {
    auto& procedureName = ast.getName();
    auto returnType = ast.getReturnType()->get()->accept(*this);
    
    std::vector<BlsType> parameterTypes;
    for (auto&& type : ast.getParameterTypes()) {
        auto typeObject = type->accept(*this);
        parameterTypes.push_back(resolve(typeObject));
    }
    procedures.emplace(procedureName, FunctionSignature(procedureName, resolve(returnType), parameterTypes));

    cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION, procedureName);
    auto params = ast.getParameters();
    for (int i = 0; i < params.size(); i++) {
        cs.setLocal(params.at(i), parameterTypes.at(i));
    }
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();

    return std::monostate();
}

std::any Analyzer::visit(AstNode::Function::Oblock& ast) {
    auto& oblockName = ast.getName();
    
    std::vector<BlsType> parameterTypes;
    for (auto&& type : ast.getParameterTypes()) {
        auto typeObject = type->accept(*this);
        parameterTypes.push_back(resolve(typeObject));
    }
    oblocks.emplace(oblockName, FunctionSignature(oblockName, std::monostate(), parameterTypes));

    cs.pushFrame(CallStack<std::string>::Frame::Context::FUNCTION, oblockName);
    auto params = ast.getParameters();
    for (int i = 0; i < params.size(); i++) {
        cs.setLocal(params.at(i), parameterTypes.at(i));
    }
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();

    return std::monostate();
}

std::any Analyzer::visit(AstNode::Setup& ast) {
    cs.pushFrame(CallStack<std::string>::Frame::Context::SETUP, "__setup__");
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();
    return true;
}

std::any Analyzer::visit(AstNode::Statement::If& ast) {
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

    auto conditionResult = condition->accept(*this);
    if (getType(resolve(conditionResult)) != TYPE::bool_t) {
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

std::any Analyzer::visit(AstNode::Statement::For& ast) {
    auto& initStatement = ast.getInitStatement();
    auto& condition = ast.getCondition();
    auto& incrementExpression = ast.getIncrementExpression();
    auto& statements = ast.getBlock();

    cs.pushFrame(CallStack<std::string>::Frame::Context::LOOP);
    if (initStatement.has_value()) {
        initStatement->get()->accept(*this);
    }
    if (condition.has_value()) {
        auto conditionResult = condition->get()->accept(*this);
        if (getType(resolve(conditionResult)) != TYPE::bool_t) {
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

std::any Analyzer::visit(AstNode::Statement::While& ast) {
    auto& condition = ast.getCondition();
    auto& statements = ast.getBlock();

    cs.pushFrame(CallStack<std::string>::Frame::Context::LOOP);
    auto conditionResult = condition->accept(*this);
    if (getType(resolve(conditionResult)) != TYPE::bool_t) {
        throw SemanticError("Condition must resolve to a boolean value");
    }
    // TODO: add checks for unreachable code
    for (auto&& statement : statements) {
        statement->accept(*this);
    }
    cs.popFrame();
    return std::monostate();
}

std::any Analyzer::visit(AstNode::Statement::Return& ast) {
    auto& functionName = cs.getFrameName();
    if (functionName == "__setup__") {
        throw SemanticError("No return statements allowed in setup()");
    }
    auto parentSignature = (procedures.contains(functionName)) ? procedures.at(functionName) : oblocks.at(functionName);
    auto expectedReturnType = getType(parentSignature.returnType);
    auto& value = ast.getValue();
    if (value.has_value()) {
        if (expectedReturnType == TYPE::void_t) {
            throw SemanticError("void function should not return a value");
        }
        auto visited = value->get()->accept(*this);
        if (!typeCompatible(parentSignature.returnType, resolve(visited))) {
            throw SemanticError("Invalid return type for " + functionName);
        }
    }
    else if (expectedReturnType != TYPE::void_t) {
        throw SemanticError("non void function should return a value");
    }
    return std::monostate();
}

std::any Analyzer::visit(AstNode::Statement::Continue& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw SemanticError("continue statement outside of loop context.");
    }
    return std::monostate();
}

std::any Analyzer::visit(AstNode::Statement::Break& ast) {
    if (!cs.checkContext(CallStack<std::string>::Frame::Context::LOOP)) {
        throw SemanticError("break statement outside of loop context.");
    }
    return std::monostate();
}

std::any Analyzer::visit(AstNode::Statement::Declaration& ast) {
    auto typedObj = ast.getType()->accept(*this);
    auto& name = ast.getName();
    auto& value = ast.getValue();
    if (value.has_value()) {
        auto literal = value->get()->accept(*this);
        if (!typeCompatible(resolve(typedObj), resolve(literal))) {
            throw SemanticError("Invalid initialization for declaration");
        }
        cs.setLocal(name, resolve(literal));
    }
    else {
        cs.setLocal(name, resolve(typedObj));
    }
    return std::monostate();
}

std::any Analyzer::visit(AstNode::Statement::Expression& ast) {
    return ast.getExpression()->accept(*this);
}

std::any Analyzer::visit(AstNode::Expression::Binary& ast) {
    auto lRes = ast.getLeft()->accept(*this);
    auto rRes = ast.getRight()->accept(*this);
    auto& lhs = resolve(lRes);
    auto& rhs = resolve(rRes);

    if (!typeCompatible(lhs, rhs)) {
        throw SemanticError("Invalid operands for binary expression");
    }

    auto op = getBinOpEnum(ast.getOp());
    // operator type checking done by overload specialization
    switch (op) {
        case Analyzer::BINARY_OPERATOR::OR:
            return BlsType(lhs || rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::AND:
            return BlsType(lhs && rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::LT:
            return BlsType(lhs < rhs); // convert back to BlsType from bool conversion
        break;
        
        case Analyzer::BINARY_OPERATOR::LE:
            return BlsType(lhs <= rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::GT:
            return BlsType(lhs > rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::GE:
            return BlsType(lhs >= rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::NE:
            return BlsType(lhs != rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::EQ:
            return BlsType(lhs == rhs); // convert back to BlsType from bool conversion
        break;

        case Analyzer::BINARY_OPERATOR::ADD:
            return lhs + rhs;
        break;
        
        case Analyzer::BINARY_OPERATOR::SUB:
            return lhs - rhs;
        break;

        case Analyzer::BINARY_OPERATOR::MUL:
            return lhs * rhs;
        break;

        case Analyzer::BINARY_OPERATOR::DIV:
            return lhs / rhs;
        break;

        case Analyzer::BINARY_OPERATOR::MOD:
            return lhs % rhs;
        break;
        
        case Analyzer::BINARY_OPERATOR::EXP:
            return lhs ^ rhs;
        break;

        // TODO: verify that lhs is not a temporary expression (needs dep graph)
        case Analyzer::BINARY_OPERATOR::ASSIGN:
            return lhs = rhs;
        break;

        case Analyzer::BINARY_OPERATOR::ASSIGN_ADD:
            return lhs = lhs + rhs;
        break;

        case Analyzer::BINARY_OPERATOR::ASSIGN_SUB:
            return lhs = lhs - rhs;
        break;

        case Analyzer::BINARY_OPERATOR::ASSIGN_MUL:
            return lhs = lhs * rhs;
        break;

        case Analyzer::BINARY_OPERATOR::ASSIGN_DIV:
            return lhs = lhs / rhs;
        break;

        case Analyzer::BINARY_OPERATOR::ASSIGN_MOD:
            return lhs = lhs % rhs;
        break;

        case Analyzer::BINARY_OPERATOR::ASSIGN_EXP:
            return lhs = lhs ^ rhs;
        break;

        default:
            throw SemanticError("Invalid operator supplied.");
        break;
    }
}

std::any Analyzer::visit(AstNode::Expression::Unary& ast) {
    auto expression = ast.getExpression()->accept(*this);
    auto& object = resolve(expression);
    auto op = getUnOpEnum(ast.getOp());
    auto position = ast.getPosition();
    
    // operator type checking done by overload specialization
    switch (op) {
        case Analyzer::UNARY_OPERATOR::NOT:
            return BlsType(!object); // convert back to BlsType from bool conversion
        break;

        case Analyzer::UNARY_OPERATOR::NEG:
            return -object;
        break;

        case Analyzer::UNARY_OPERATOR::INC:
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

        case Analyzer::UNARY_OPERATOR::DEC:
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

std::any Analyzer::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

std::any Analyzer::visit(AstNode::Expression::Method& ast) {
    auto& object = cs.getLocal(ast.getObject());
    auto& methodName = ast.getMethodName();
    auto& methodArgs = ast.getArguments();
    auto objType = getType(object);
    if (objType < TYPE::PRIMITIVE_COUNT || objType > TYPE::CONTAINER_COUNT) {
        throw SemanticError("Methods may only be applied on container type objects.");
    }
    std::vector<BlsType> resolvedArgs;
    for (auto&& arg : methodArgs) {
        auto visited = arg->accept(*this);
        resolvedArgs.push_back(resolve(visited));
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

std::any Analyzer::visit(AstNode::Expression::Function& ast) {
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
            auto result = arg->accept(*this);
            if (!typeCompatible(argType, resolve(result))) {
                throw SemanticError("Invalid argument provided to " + name);
            }
        }
    }
    return signature.returnType;
}

std::any Analyzer::visit(AstNode::Expression::Access& ast) {
    auto& object = cs.getLocal(ast.getObject());
    auto& member = ast.getMember();
    auto& subscript = ast.getSubscript();
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
        auto index = subscript->get()->accept(*this);
        return std::ref(subscriptable->access(resolve(index)));
    }
    else {
        return std::ref(object);
    }
}

std::any Analyzer::visit(AstNode::Expression::Literal& ast) {
    std::any literal;
    const auto convert = overloads {
        [&literal](auto value) { literal = BlsType(value); }
    };
    std::visit(convert, ast.getLiteral());
    literals.emplace(resolve(literal));
    return literal;
}

std::any Analyzer::visit(AstNode::Expression::List& ast) {
    auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    BlsType initIdx = 0;
    auto addElement = [&, this](auto& element) {
        auto literal = element->accept(*this);
        auto& resolved = resolve(literal);
        list->append(resolved);
        return resolved;
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
    
    literals.emplace(BlsType(list));

    return BlsType(list);
}

std::any Analyzer::visit(AstNode::Expression::Set& ast) {
    throw SemanticError("no support for sets in phase 0");
}

std::any Analyzer::visit(AstNode::Expression::Map& ast) {
    auto map = std::make_shared<MapDescriptor>(TYPE::ANY);
    auto& elements = ast.getElements();
    BlsType initKey, initVal;
    auto addElement = [&, this](auto& element) {
        auto key = element.first->accept(*this);
        auto keyType = getType(resolve(key));
        if (keyType > TYPE::PRIMITIVE_COUNT) {
            throw SemanticError("Invalid key type for map");
        }
        auto value = element.second->accept(*this);
        auto& resolvedKey = resolve(key), resolvedVal = resolve(value);
        map->emplace(resolvedKey, resolvedVal);
        return std::pair(resolvedKey, resolvedVal);
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

    literals.emplace(BlsType(map));

    return BlsType(map);
}

std::any Analyzer::visit(AstNode::Specifier::Type& ast) {
    TYPE primaryType = getTypeFromName(ast.getName());
    const auto& typeArgs = ast.getTypeArgs();
    if (primaryType == TYPE::list_t) {
        if (typeArgs.size() != 1) throw SemanticError("List may only have a single type argument.");
        auto argType = getTypeFromName(typeArgs.at(0)->getName());
        auto list = std::make_shared<VectorDescriptor>(argType);
        auto arg = typeArgs.at(0)->accept(*this);
        list->append(resolve(arg));
        list->getSampleElement() = resolve(arg);
        return BlsType(list);
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
        auto key = typeArgs.at(0)->accept(*this);
        auto value = typeArgs.at(1)->accept(*this);
        map->emplace(resolve(key), resolve(value));
        map->getSampleElement() = resolve(value);
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

            default:
                throw SemanticError("Container type must include element type arguments.");
            break;
        }
    }
    else {
        throw SemanticError("Primitive or devtype cannot include type arguments.");
    }
}

BlsType& Analyzer::resolve(std::any& val) {
    if (val.type() == typeid(std::reference_wrapper<BlsType>)) {
        return std::any_cast<std::reference_wrapper<BlsType>>(val);
    }
    else {
        return std::ref(std::any_cast<BlsType&>(val));
    }
}