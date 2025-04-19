#include "generator.hpp"
#include "ast.hpp"
#include "libbytecode/include/opcodes.hpp"
#include "libtypes/bls_types.hpp"
#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <boost/archive/binary_oarchive.hpp>
#include <unordered_set>

using namespace BlsLang;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

std::any Generator::visit(AstNode::Source& ast) {
    for (auto&& procedure : ast.getProcedures()) {
        procedure->accept(*this);
    }

    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }

    ast.getSetup()->accept(*this);

    return 0;
}

std::any Generator::visit(AstNode::Function::Procedure& ast) {
    uint16_t address = instructions.size();
    uint8_t argc = ast.getParameters().size();
    auto callInstruction = INSTRUCTION::CALL{ {OPCODE::CALL}, address, argc};
    procedureMap.emplace(ast.getName(), callInstruction);
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return 0;
}

std::any Generator::visit(AstNode::Function::Oblock& ast) {
    uint16_t address = instructions.size();
    oblockDescriptors.at(ast.getName()).bytecode_offset = address;
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return 0;
}

std::any Generator::visit(AstNode::Setup& ast) {
    boost::archive::binary_oarchive oa(outputStream, boost::archive::archive_flags::no_header);
    
    // write header
    uint16_t descSize = oblockDescriptors.size();
    outputStream.write(reinterpret_cast<const char *>(&descSize), sizeof(descSize));
    for (auto&& desc : oblockDescriptors) {
        oa << desc;
    }

    // write literal pool
    uint16_t poolSize = literalPool.size();
    outputStream.write(reinterpret_cast<const char *>(&poolSize), sizeof(poolSize));
    for (auto&& literal : literalPool) {
        oa << literal;
    }

    for (auto&& instruction : instructions) {
        switch (instruction->opcode) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: { \
                auto& resolvedInstruction = reinterpret_cast<INSTRUCTION::code&>(*instruction);
            #define ARGUMENT(arg, type) \
                type& arg = resolvedInstruction.arg; \
                outputStream.write(reinterpret_cast<const char *>(&arg), sizeof(type));
            #define OPCODE_END(code, args...) \
                break; \
            } 
            #include "libbytecode/include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
            default:
            break;
        }
    }

    return 0;
}

std::any Generator::visit(AstNode::Statement::If& ast) {
    ast.getCondition()->accept(*this);
    auto branchPtr = createBRANCH(0);
    auto& branchInstruction = *branchPtr;
    instructions.push_back(std::move(branchPtr));
    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);   
    }
    branchInstruction.address = instructions.size();
    for (auto&& elif : ast.getElseIfStatements()) {
        elif->accept(*this);
    }
    for (auto&& statement : ast.getElseBlock()) {
        statement->accept(*this);
    }
    return 0;
}

std::any Generator::visit(AstNode::Statement::For& ast) {
    auto& initStatement = ast.getInitStatement();
    if (initStatement.has_value()) {
        initStatement->get()->accept(*this);
    }
    uint16_t loopStart = instructions.size();

    auto& condition = ast.getCondition();
    if (condition.has_value()) {
        condition->get()->accept(*this);
    }
    auto loopBranchPtr = createBRANCH(0);
    auto& loopBranchInstruction = *loopBranchPtr;
    instructions.push_back(std::move(loopBranchPtr));

    auto& incrementExpression = ast.getIncrementExpression();
    if (incrementExpression.has_value()) {
        incrementExpression->get()->accept(*this);
    }

    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    instructions.push_back(createJMP(loopStart));
    loopBranchInstruction.address = instructions.size();
    
    return 0;
}

std::any Generator::visit(AstNode::Statement::While& ast) {
    std::optional<std::reference_wrapper<INSTRUCTION::JMP>> doJMPInstruction;
    if (ast.getType() == AstNode::Statement::While::LOOP_TYPE::DO) {
        auto jmpPtr = createJMP(0);
        doJMPInstruction = *jmpPtr;
        instructions.push_back(std::move(jmpPtr));
    }
    uint16_t loopStart = instructions.size();

    ast.getCondition()->accept(*this);
    auto loopBranchPtr = createBRANCH(0);
    auto& loopBranchInstruction = *loopBranchPtr;
    instructions.push_back(std::move(loopBranchPtr));
    if (doJMPInstruction.has_value()) { // skip branch condition with do JMP
        doJMPInstruction->get().address = instructions.size();
    }

    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    instructions.push_back(createJMP(loopStart));
    loopBranchInstruction.address = instructions.size();

    return 0;
}

std::any Generator::visit(AstNode::Statement::Return& ast) {
    auto& returnExpression = ast.getValue();
    if (returnExpression.has_value()) {
        returnExpression->get()->accept(*this);
    }
    else {  // push void value
        instructions.push_back(createPUSH(0));
    }
    instructions.push_back(createRETURN());
    return 0;
}

std::any Generator::visit(AstNode::Statement::Continue& ast) {

}

std::any Generator::visit(AstNode::Statement::Break& ast) {

}

std::any Generator::visit(AstNode::Statement::Declaration& ast) {

}

std::any Generator::visit(AstNode::Statement::Expression& ast) {

}

std::any Generator::visit(AstNode::Expression::Binary& ast) {

}

std::any Generator::visit(AstNode::Expression::Unary& ast) {

}

std::any Generator::visit(AstNode::Expression::Group& ast) {

}

std::any Generator::visit(AstNode::Expression::Method& ast) {

}

std::any Generator::visit(AstNode::Expression::Function& ast) {

}

std::any Generator::visit(AstNode::Expression::Access& ast) {

}

std::any Generator::visit(AstNode::Expression::Literal& ast) {

}

std::any Generator::visit(AstNode::Expression::List& ast) {

}

std::any Generator::visit(AstNode::Expression::Set& ast) {
    throw std::runtime_error("no support for sets in phase 0");
}

std::any Generator::visit(AstNode::Expression::Map& ast) {

}

std::any Generator::visit(AstNode::Specifier::Type& ast) {

}

#define OPCODE_BEGIN(code) \
std::unique_ptr<INSTRUCTION::code> Generator::create##code(
#define ARGUMENT(arg, type) \
type arg,
#define OPCODE_END(code, args...) \
int) { \
    return std::make_unique<INSTRUCTION::code>(INSTRUCTION::code{{OPCODE::code}, args}); \
}
#include "libbytecode/include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END