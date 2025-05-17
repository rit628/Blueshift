#include "generator.hpp"
#include "ast.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "libbytecode/include/opcodes.hpp"
#include "libtypes/bls_types.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <boost/archive/binary_oarchive.hpp>
#include <utility>
#include <variant>

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
    // maybe add a discard operation for the expression result

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
        instructions.push_back(createPUSH(literalPool.at(std::monostate())));
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
    auto type = getTypeFromName(ast.getType()->getName());
    auto index = ast.getLocalIndex();
    instructions.push_back(createMKTYPE(index, static_cast<uint8_t>(type)));
    auto& value = ast.getValue();
    if (value.has_value()) {
        value->get()->accept(*this);
    }
    return 0;
}

BlsObject Generator::visit(AstNode::Statement::Expression& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Generator::visit(AstNode::Expression::Binary& ast) {
    auto op = getBinOpEnum(ast.getOp());
    static auto createBinaryOperation = [&ast, this](std::unique_ptr<INSTRUCTION>&& instruction) {
        ast.getLeft()->accept(*this);
        ast.getRight()->accept(*this);
        instructions.push_back(std::move(instruction));
    };

    static auto createCompoundAssignment = [&ast, this](std::unique_ptr<INSTRUCTION>&& instruction) {
        // visit lhs as write target
        accessContext = ACCESS_CONTEXT::WRITE;
        ast.getLeft()->accept(*this);

        // visit both sides to create operation and move store past it
        auto storeInstruction = std::move(instructions.back());
        instructions.pop_back();
        createBinaryOperation(std::move(instruction));
        instructions.push_back(std::move(storeInstruction));

        ast.getLeft()->accept(*this); // visit lhs as operation result
    };

    switch (op) {
        case BINARY_OPERATOR::OR:
            createBinaryOperation(createOR());
        break;

        case BINARY_OPERATOR::AND:
            createBinaryOperation(createAND());
        break;

        case BINARY_OPERATOR::LT:
            createBinaryOperation(createLT());
        break;
        
        case BINARY_OPERATOR::LE:
            createBinaryOperation(createLE());
        break;

        case BINARY_OPERATOR::GT:
            createBinaryOperation(createGT());
        break;

        case BINARY_OPERATOR::GE:
            createBinaryOperation(createGE());
        break;

        case BINARY_OPERATOR::NE:
            createBinaryOperation(createNE());
        break;

        case BINARY_OPERATOR::EQ:
            createBinaryOperation(createEQ());
        break;

        case BINARY_OPERATOR::ADD:
            createBinaryOperation(createADD());
        break;
        
        case BINARY_OPERATOR::SUB:
            createBinaryOperation(createSUB());
        break;

        case BINARY_OPERATOR::MUL:
            createBinaryOperation(createMUL());
        break;

        case BINARY_OPERATOR::DIV:
            createBinaryOperation(createDIV());
        break;

        case BINARY_OPERATOR::MOD:
            createBinaryOperation(createMOD());
        break;
        
        case BINARY_OPERATOR::EXP:
            createBinaryOperation(createEXP());
        break;

        case BINARY_OPERATOR::ASSIGN: {
            // visit lhs as write target
            accessContext = ACCESS_CONTEXT::WRITE;
            ast.getLeft()->accept(*this);

            // visit rhs and move store past rhs instructions
            auto storeInstruction = std::move(instructions.back());
            instructions.pop_back();
            ast.getRight()->accept(*this);
            instructions.push_back(std::move(storeInstruction));

            ast.getLeft()->accept(*this); // visit lhs as operation result
        }
        break;

        case BINARY_OPERATOR::ASSIGN_ADD:
            createCompoundAssignment(createADD());
        break;

        case BINARY_OPERATOR::ASSIGN_SUB:
            createCompoundAssignment(createSUB());
        break;

        case BINARY_OPERATOR::ASSIGN_MUL:
            createCompoundAssignment(createMUL());
        break;

        case BINARY_OPERATOR::ASSIGN_DIV:
            createCompoundAssignment(createDIV());
        break;

        case BINARY_OPERATOR::ASSIGN_MOD:
            createCompoundAssignment(createMOD());
        break;

        case BINARY_OPERATOR::ASSIGN_EXP:
            createCompoundAssignment(createEXP());
        break;

        default:
            throw std::runtime_error("Invalid operator supplied.");
        break;
    }
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Unary& ast) {
    auto op = getUnOpEnum(ast.getOp());
    auto position = ast.getPosition();

    static auto createCompoundAssignment = [&, this](std::unique_ptr<INSTRUCTION>&& instruction) {
        if (position == AstNode::Expression::Unary::OPERATOR_POSITION::POSTFIX) {
            ast.getExpression()->accept(*this); // read before operation
        }

        // visit expression as write target
        accessContext = ACCESS_CONTEXT::WRITE;
        ast.getExpression()->accept(*this);

        auto storeInstruction = std::move(instructions.back());
        instructions.pop_back();

        // create unary op
        ast.getExpression()->accept(*this); // visit as operand
        instructions.push_back(createPUSH(literalPool.at(1))); // push literal 1
        instructions.push_back(std::move(instruction));

        instructions.push_back(std::move(storeInstruction)); // move store past unary op

        if (position == AstNode::Expression::Unary::OPERATOR_POSITION::PREFIX) {
            ast.getExpression()->accept(*this); // read after operation
        }
    };
    
    switch (op) {
        case UNARY_OPERATOR::NOT:
            ast.getExpression()->accept(*this);
            instructions.push_back(createNOT());
        break;

        case UNARY_OPERATOR::NEG:
            ast.getExpression()->accept(*this);
            instructions.push_back(createNEG());
        break;

        case UNARY_OPERATOR::INC:
            createCompoundAssignment(createADD());
        break;

        case UNARY_OPERATOR::DEC:
            createCompoundAssignment(createSUB());
        break;

        default:
            throw std::runtime_error("Invalid operator supplied.");
        break;
    }

    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Group& ast) {
    return ast.getExpression()->accept(*this);
}

BlsObject Generator::visit(AstNode::Expression::Method& ast) {
    instructions.push_back(createLOAD(ast.getLocalIndex()));
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
    auto localIndex = ast.getLocalIndex();
    auto& member = ast.getMember();
    auto& subscript = ast.getSubscript();
    if (member.has_value()) {
        if (accessContext == ACCESS_CONTEXT::READ) {
            instructions.push_back(createLOAD(localIndex));
            instructions.push_back(createPUSH(literalPool.at(member.value())));
            instructions.push_back(createALOAD());
        }
        else {
            instructions.push_back(createLOAD(localIndex));
            instructions.push_back(createPUSH(literalPool.at(member.value())));
            instructions.push_back(createASTORE());
        }
    }
    else if (subscript.has_value()) {
        if (accessContext == ACCESS_CONTEXT::READ) {
            instructions.push_back(createLOAD(localIndex));
            subscript->get()->accept(*this);
            instructions.push_back(createALOAD());
        }
        else {
            instructions.push_back(createLOAD(localIndex));
            accessContext = ACCESS_CONTEXT::READ; // set accessContext to read for sub expression
            subscript->get()->accept(*this);
            instructions.push_back(createASTORE());
        }
    }
    else {
        if (accessContext == ACCESS_CONTEXT::READ) {
            instructions.push_back(createLOAD(localIndex));
        }
        else {
            instructions.push_back(createSTORE(localIndex));
        }
    }
    accessContext = ACCESS_CONTEXT::READ; // reset accessContext
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Literal& ast) {
    BlsType literal = std::visit([](auto& l){ return BlsType(l); }, ast.getLiteral());
    instructions.push_back(createPUSH(literalPool.at(literal)));
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::List& ast) {
    auto& literal = ast.getLiteral();
    auto& expressions = ast.getElements();
    auto& list = std::dynamic_pointer_cast<VectorDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(literal))->getVector();
    for (size_t i = 0; i < list.size(); i++) {
        if (std::holds_alternative<std::monostate>(list.at(i))) { // sub expression needs to be evaluated at runtime
            instructions.push_back(createPUSH(literalPool.at(literal)));
            instructions.push_back(createPUSH(i));
            expressions.at(i)->accept(*this);
        }
    }
    instructions.push_back(createPUSH(literalPool.at(literal)));
    return 0;
}

BlsObject Generator::visit(AstNode::Expression::Set& ast) {
    throw std::runtime_error("no support for sets in phase 0");
}

BlsObject Generator::visit(AstNode::Expression::Map& ast) {
    auto& literal = ast.getLiteral();
    auto& expressions = ast.getElements();
    auto& map = std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(literal))->getMap();
    for (auto&& [key, value] : expressions) {
        auto* keyLiteral = dynamic_cast<AstNode::Expression::Literal*>(key.get());
        if (!keyLiteral
         || std::holds_alternative<std::monostate>(map.at(std::get<std::string>(keyLiteral->getLiteral())))) {
            // key value pair not in map literal at runtime; emplace
            instructions.push_back(createPUSH(literalPool.at(literal)));
            key->accept(*this);
            value->accept(*this);
            instructions.push_back(createEMPLACE());
        }
    }
    instructions.push_back(createPUSH(literalPool.at(literal)));
    return 0;
}

BlsObject Generator::visit(AstNode::Specifier::Type& ast) {
    return 0; // type declarations dont need to be visited in generator
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