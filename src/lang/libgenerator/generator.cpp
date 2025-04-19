#include "generator.hpp"
#include "ast.hpp"
#include "libbytecode/bytecode_processor.hpp"
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

BlsObject Generator::visit(AstNode::Source& ast) {
    for (auto&& procedure : ast.getProcedures()) {
        procedure->accept(*this);
    }

    for (auto&& oblock : ast.getOblocks()) {
        oblock->accept(*this);
    }

    ast.getSetup()->accept(*this);

    return 0;
}

BlsObject Generator::visit(AstNode::Function::Procedure& ast) {
    uint16_t address = instructions.size();
    procedureAddresses.emplace(ast.getName(), address);
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return 0;
}

BlsObject Generator::visit(AstNode::Function::Oblock& ast) {
    uint16_t address = instructions.size();
    oblockDescriptors.at(ast.getName()).bytecode_offset = address;
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    instructions.push_back(createEMIT(static_cast<uint8_t>(BytecodeProcessor::SIGNAL::SIGSTOP)));
    return 0;
}

BlsObject Generator::visit(AstNode::Setup& ast) {
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

BlsObject Generator::visit(AstNode::Statement::If& ast) {
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

BlsObject Generator::visit(AstNode::Statement::For& ast) {
    auto& initStatement = ast.getInitStatement();
    if (initStatement.has_value()) {
        initStatement->get()->accept(*this);
    }
    uint16_t loopStart = instructions.size();
    loopIndices.push(loopStart); // for use in continue JMP instructions

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
    uint16_t endAddress = instructions.size();
    loopBranchInstruction.address = endAddress;

    loopIndices.pop(); // in scope continue JMP instructions emitted, index no longer necessary
    for (size_t i = 0; i < breakIndices.size(); i++) {  // set break JMP indices
        auto& breakInstruction = instructions.at(breakIndices.top());
        static_cast<INSTRUCTION::JMP&>(*breakInstruction).address = endAddress;
        breakIndices.pop();
    }
    
    return 0;
}

BlsObject Generator::visit(AstNode::Statement::While& ast) {
    std::optional<std::reference_wrapper<INSTRUCTION::JMP>> doJMPInstruction;
    if (ast.getType() == AstNode::Statement::While::LOOP_TYPE::DO) {
        auto jmpPtr = createJMP(0);
        doJMPInstruction = *jmpPtr;
        instructions.push_back(std::move(jmpPtr));
    }
    uint16_t loopStart = instructions.size();
    loopIndices.push(loopStart); // for use in continue JMP instructions

    ast.getCondition()->accept(*this);
    auto loopBranchPtr = createBRANCH(0);
    auto& loopBranchInstruction = *loopBranchPtr;
    instructions.push_back(std::move(loopBranchPtr));
    if (doJMPInstruction.has_value()) { // skip branch condition with do statement JMP
        doJMPInstruction->get().address = instructions.size();
    }

    for (auto&& statement : ast.getBlock()) {
        statement->accept(*this);
    }
    instructions.push_back(createJMP(loopStart));
    uint16_t endAddress = instructions.size();
    loopBranchInstruction.address = endAddress;

    loopIndices.pop(); // in scope continue JMP instructions emitted, index no longer necessary
    for (size_t i = 0; i < breakIndices.size(); i++) {  // set break JMP indices
        auto& breakInstruction = instructions.at(breakIndices.top());
        static_cast<INSTRUCTION::JMP&>(*breakInstruction).address = endAddress;
        breakIndices.pop();
    }

    return 0;
}

BlsObject Generator::visit(AstNode::Statement::Return& ast) {
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

BlsObject Generator::visit(AstNode::Statement::Continue& ast) {
    uint16_t parentLoopIndex = loopIndices.top();
    instructions.push_back(createJMP(parentLoopIndex));
    return 0;
}

BlsObject Generator::visit(AstNode::Statement::Break& ast) {
    breakIndices.push(instructions.size());
    instructions.push_back(createJMP(0));
    return 0;
}

BlsObject Generator::visit(AstNode::Statement::Declaration& ast) {

}

BlsObject Generator::visit(AstNode::Statement::Expression& ast) {

}

BlsObject Generator::visit(AstNode::Expression::Binary& ast) {
    auto op = getBinOpEnum(ast.getOp());
    switch (op) {
        case BINARY_OPERATOR::OR:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createOR());
        break;

        case BINARY_OPERATOR::AND:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createAND());
        break;

        case BINARY_OPERATOR::LT:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createLT());
        break;
        
        case BINARY_OPERATOR::LE:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createLE());
        break;

        case BINARY_OPERATOR::GT:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createGT());
        break;

        case BINARY_OPERATOR::GE:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createGE());
        break;

        case BINARY_OPERATOR::NE:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createNE());
        break;

        case BINARY_OPERATOR::EQ:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createEQ());
        break;

        case BINARY_OPERATOR::ADD:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createADD());
        break;
        
        case BINARY_OPERATOR::SUB:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createSUB());
        break;

        case BINARY_OPERATOR::MUL:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createMUL());
        break;

        case BINARY_OPERATOR::DIV:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createDIV());
        break;

        case BINARY_OPERATOR::MOD:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createMOD());
        break;
        
        case BINARY_OPERATOR::EXP:
            ast.getLeft()->accept(*this);
            ast.getRight()->accept(*this);
            instructions.push_back(createEXP());
        break;

        case BINARY_OPERATOR::ASSIGN:
            ast.getRight()->accept(*this);
            accessContext = ACCESS_CONTEXT::WRITE;
            accessContext = ACCESS_CONTEXT::READ;
        break;

        case BINARY_OPERATOR::ASSIGN_ADD:
            ast.getLeft()->accept(*this); // visit lhs as operand
            ast.getRight()->accept(*this);
            instructions.push_back(createADD());
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this); // visit lhs again as write target
            accessContext = ACCESS_CONTEXT::READ;
        break;

        case BINARY_OPERATOR::ASSIGN_SUB:
            ast.getLeft()->accept(*this); // visit lhs as operand
            ast.getRight()->accept(*this);
            instructions.push_back(createSUB());
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this); // visit lhs again as write target
            accessContext = ACCESS_CONTEXT::READ;
        break;

        case BINARY_OPERATOR::ASSIGN_MUL:
            ast.getLeft()->accept(*this); // visit lhs as operand
            ast.getRight()->accept(*this);
            instructions.push_back(createMUL());
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this); // visit lhs again as write target
            accessContext = ACCESS_CONTEXT::READ;
        break;

        case BINARY_OPERATOR::ASSIGN_DIV:
            ast.getLeft()->accept(*this); // visit lhs as operand
            ast.getRight()->accept(*this);
            instructions.push_back(createDIV());
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this); // visit lhs again as write target
            accessContext = ACCESS_CONTEXT::READ;
        break;

        case BINARY_OPERATOR::ASSIGN_MOD:
            ast.getLeft()->accept(*this); // visit lhs as operand
            ast.getRight()->accept(*this);
            instructions.push_back(createMOD());
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this); // visit lhs again as write target
            accessContext = ACCESS_CONTEXT::READ;
        break;

        case BINARY_OPERATOR::ASSIGN_EXP:
            ast.getLeft()->accept(*this); // visit lhs as operand
            ast.getRight()->accept(*this);
            instructions.push_back(createEXP());
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this); // visit lhs again as write target
            accessContext = ACCESS_CONTEXT::READ;
        break;

        default:
            throw std::runtime_error("Invalid operator supplied.");
        break;
    }
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Unary& ast) {
    // ignore operator afixment in phase 1 vm
    ast.getExpression()->accept(*this);
    auto op = getUnOpEnum(ast.getOp());
    
    switch (op) {
        case UNARY_OPERATOR::NOT:
            instructions.push_back(createNOT());
        break;

        case UNARY_OPERATOR::NEG:
            instructions.push_back(createNEG());
        break;

        case UNARY_OPERATOR::INC:
            instructions.push_back(createINC(0)); // temporary value, change to actual index using ast decoration
        break;

        case UNARY_OPERATOR::DEC:
            instructions.push_back(createDEC(0)); // temporary value, change to actual index using ast decoration
        break;

        default:
            throw std::runtime_error("Invalid operator supplied.");
        break;
    }
}

BlsObject Generator::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Generator::visit(AstNode::Expression::Method& ast) {
    // need to push object based on index using object name
    ast.getObject();
    auto& methodName = ast.getMethodName();
    for (auto&& arg : ast.getArguments()) {
        arg->accept(*this);
    }
    if (methodName == "append") {
        instructions.push_back(createAPPEND());
    }
    else if (methodName == "emplace") {
        instructions.push_back(createEMPLACE());
    }
    else if (methodName == "size") {
        instructions.push_back(createSIZE());
    }
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Function& ast) {
    auto& args = ast.getArguments();
    for (auto&& arg : args) {
        arg->accept(*this);
    }
    auto& name = ast.getName();
    if (name == "println") {
        instructions.push_back(createPRINTLN(args.size()));
    }
    else if (name == "print") {
        instructions.push_back(createPRINT(args.size()));
    }
    else {
        auto address = procedureAddresses.at(name);
        instructions.push_back(createCALL(address, args.size()));
    }
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Access& ast) {

}

BlsObject Generator::visit(AstNode::Expression::Literal& ast) {

}

BlsObject Generator::visit(AstNode::Expression::List& ast) {

}

BlsObject Generator::visit(AstNode::Expression::Set& ast) {
    throw std::runtime_error("no support for sets in phase 0");
}

BlsObject Generator::visit(AstNode::Expression::Map& ast) {

}

BlsObject Generator::visit(AstNode::Specifier::Type& ast) {
    // type declarations dont need to be visited in generator
    return 0;
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