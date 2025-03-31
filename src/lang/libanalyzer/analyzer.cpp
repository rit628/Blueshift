#include "libtypes/typedefs.hpp"
#include "analyzer.hpp"
#include "libtypes/bls_types.hpp"
#include "call_stack.hpp"
#include <any>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

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

    ast.getSetup()->accept(*this);

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
    cs.pushFrame(CallStack<std::string>::Frame::Context::SETUP, "_setup");
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    cs.popFrame();
    return true;
}

std::any Analyzer::visit(AstNode::Statement::If& ast) {

}

std::any Analyzer::visit(AstNode::Statement::For& ast) {

}

std::any Analyzer::visit(AstNode::Statement::While& ast) {

}

std::any Analyzer::visit(AstNode::Statement::Return& ast) {
    auto& functionName = cs.getFrameName();
    if (functionName == "_setup") {
        throw std::runtime_error("No return statements allowed in setup()");
    }
    auto parentSignature = (procedures.contains(functionName)) ? procedures.at(functionName) : oblocks.at(functionName);
    auto expectedReturnType = getTypeEnum(parentSignature.returnType);
    auto& value = ast.getValue();
    if (value.has_value()) {
        if (expectedReturnType == TYPE::void_t) {
            throw std::runtime_error("void function should not return a value");
        }
        auto visited = value->get()->accept(*this);
        if (!typeCompatible(parentSignature.returnType, resolve(visited))) {
            throw std::runtime_error("invalid return type");
        }
    }
    else if (expectedReturnType != TYPE::void_t) {
        throw std::runtime_error("non void function should return a value");
    }
    return std::monostate();
}

std::any Analyzer::visit(AstNode::Statement::Continue& ast) {

}

std::any Analyzer::visit(AstNode::Statement::Break& ast) {

}

std::any Analyzer::visit(AstNode::Statement::Declaration& ast) {

}

std::any Analyzer::visit(AstNode::Statement::Expression& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Binary& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Unary& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Group& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Method& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Function& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Access& ast) {

}

std::any Analyzer::visit(AstNode::Expression::Literal& ast) {
    std::any literal;
    const auto convert = overloads {
        [&literal](size_t value) {literal = BlsType((int64_t)value);},
        [&literal](auto value) { literal = BlsType(value); }
    };
    std::visit(convert, ast.getLiteral());
    return literal;
}

std::any Analyzer::visit(AstNode::Expression::List& ast) {
    
}

std::any Analyzer::visit(AstNode::Expression::Set& ast) {
    throw std::runtime_error("no support for sets in phase 0");
}

std::any Analyzer::visit(AstNode::Expression::Map& ast) {

}

std::any Analyzer::visit(AstNode::Specifier::Type& ast) {
    TYPE primaryType = getTypeEnum(ast.getName());
    const auto& typeArgs = ast.getTypeArgs();
    if (primaryType == TYPE::list_t) {
        if (typeArgs.size() != 1) throw std::runtime_error("list may only have a single type argument");
        auto argType = getTypeEnum(typeArgs.at(0)->getName());
        auto list = std::make_shared<VectorDescriptor>(argType);
        auto arg = typeArgs.at(0)->accept(*this);
        list->append(resolve(arg));
        return BlsType(list);
    }
    else if (primaryType == TYPE::map_t) {
        if (typeArgs.size() != 2) throw std::runtime_error("map must have two type arguments");
        auto keyType = getTypeEnum(typeArgs.at(0)->getName());
        if (keyType > TYPE::PRIMITIVE_COUNT || keyType == TYPE::void_t) {
            throw std::runtime_error("invalid key type for map");
        }
        auto valType = getTypeEnum(typeArgs.at(1)->getName());
        if (valType == TYPE::void_t) {
            throw std::runtime_error("invalid value type for map");
        }
        auto map = std::make_shared<MapDescriptor>(TYPE::map_t, keyType, valType);
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
                return BlsType(int64_t());
            break;

            case TYPE::float_t:
                return BlsType(double());
            break;

            case TYPE::string_t:
                return BlsType(std::string());
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
                throw std::runtime_error("container type must include element type arguments");
            break;
        }
    }
    else {
        throw std::runtime_error("primitive or devtype cannot include type arguments");
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