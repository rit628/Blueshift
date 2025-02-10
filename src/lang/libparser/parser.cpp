#include "parser.hpp"
#include "ast.hpp"
#include "liblexer/token.hpp"
#include "include/reserved_tokens.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace BlsLang;

std::unique_ptr<AstNode::Source> Parser::parse(std::vector<Token> tokenStream) {
    ts.setStream(tokenStream);
    return parseSource();
}

std::unique_ptr<AstNode::Source> Parser::parseSource() {
    std::vector<std::unique_ptr<AstNode::Function>> procedures;
    std::vector<std::unique_ptr<AstNode::Function>> oblocks;
    std::unique_ptr<AstNode::Setup> setup = nullptr;

    while (!ts.empty()) {
        if (ts.peek(RESERVED_SETUP)) {
            if (setup != nullptr) {
                throw ParseException("Only one setup function allowed per source file.", ts.getLine(), ts.getColumn());
            }
            setup = parseSetup();
        }
        else if (ts.peek(RESERVED_OBLOCK)) {
            oblocks.push_back(parseFunction());
        }
        else if (ts.peek(Token::Type::IDENTIFIER)) {
            procedures.push_back(parseFunction());
        }
        else {
            throw ParseException("Invalid top level element.", ts.getLine(), ts.getColumn());
        }
    }

    if (setup == nullptr) {
        throw ParseException("No setup function found in file.", ts.getLine(), ts.getColumn());
    }

    return std::make_unique<AstNode::Source>(std::move(procedures)
                                           , std::move(oblocks)
                                           , std::move(setup));
}

std::unique_ptr<AstNode::Setup> Parser::parseSetup() {
    ts.match(RESERVED_SETUP, PARENTHESES_OPEN, PARENTHESES_CLOSE);
    auto statements = parseBlock();
    return std::make_unique<AstNode::Setup>(std::move(statements));
}

std::unique_ptr<AstNode::Function> Parser::parseFunction() {
    std::unique_ptr<AstNode::Specifier::Type> returnType = nullptr;
    if (!ts.match(RESERVED_OBLOCK)) {
        returnType = parseTypeSpecifier();
    }
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw ParseException("Expected valid identifier for function name.", ts.getLine(), ts.getColumn());
    }
    auto& name = ts.at(-1).getLiteral();
    std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes;
    std::vector<std::string> parameters;
    matchExpectedSymbol(PARENTHESES_OPEN, "at start of function parameter list.");
    if (!ts.peek(PARENTHESES_CLOSE)) {
        do {
            parameterTypes.push_back(parseTypeSpecifier());
            if (ts.match(Token::Type::IDENTIFIER)) {
                parameters.push_back(std::move(ts.at(-1).getLiteral()));
            }
            else {
                throw ParseException("Invalid function parameter.", ts.getLine(), ts.getColumn());
            }
        } while (ts.match(COMMA));
    }
    matchExpectedSymbol(PARENTHESES_CLOSE, "at end of function parameter list.");
    auto statements = parseBlock();
    if (returnType) { // parsed procedure
        return std::make_unique<AstNode::Function::Procedure>(std::move(name)
                                                            , std::move(returnType)
                                                            , std::move(parameterTypes)
                                                            , std::move(parameters)
                                                            , std::move(statements));
    }
    else { // parsed oblock
        return std::make_unique<AstNode::Function::Oblock>(std::move(name)
                                                         , std::move(parameterTypes)
                                                         , std::move(parameters)
                                                         , std::move(statements));
    }
}

std::vector<std::unique_ptr<AstNode::Statement>> Parser::parseBlock() {
    matchExpectedSymbol(BRACE_OPEN, "at start of block body.");
    std::vector<std::unique_ptr<AstNode::Statement>> body;
    while (!ts.peek(BRACE_CLOSE) && !ts.empty()) {
        body.push_back(parseStatement());
    }
    matchExpectedSymbol(BRACE_CLOSE, "at end of block body.");
    return body;
}

std::unique_ptr<AstNode::Statement> Parser::parseStatement() {
    if (ts.peek(RESERVED_IF)) {
        return parseIfStatement();
    }
    else if (ts.peek(RESERVED_FOR)) {
        return parseForStatement();
    }
    else if (ts.peek(RESERVED_WHILE)) {
        return parseWhileStatement();
    }
    else if (ts.peek(RESERVED_RETURN)) {
        return parseReturnStatement();
    }
    else if (peekTypedDeclaration()) {
        return parseDeclarationStatement();
    }
    else {
        return parseAssignmentExpressionStatement();
    }
}

std::unique_ptr<AstNode::Statement> Parser::parseAssignmentExpressionStatement() {
    auto lhs = parseExpression();
    if (ts.match(ASSIGNMENT)) {
        auto rhs = parseExpression();
        matchExpectedSymbol(SEMICOLON, "at end of assignment.");
        return std::make_unique<AstNode::Statement::Assignment>(std::move(lhs), std::move(rhs));
    }
    matchExpectedSymbol(SEMICOLON, "at end of expression.");
    return std::make_unique<AstNode::Statement::Expression>(std::move(lhs));
}

std::unique_ptr<AstNode::Statement::Declaration> Parser::parseDeclarationStatement() {
    auto type = parseTypeSpecifier();
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw ParseException("Expected valid identifier for variable name.", ts.getLine(), ts.getColumn());
    }
    auto& name = ts.at(-1).getLiteral();
    auto rhs = (ts.match(ASSIGNMENT)) ? std::make_optional(parseExpression()) : std::nullopt;
    matchExpectedSymbol(SEMICOLON, "at end of declaration.");
    return std::make_unique<AstNode::Statement::Declaration>(std::move(name), std::move(type), std::move(rhs));
}

std::unique_ptr<AstNode::Statement::Return> Parser::parseReturnStatement() {
    ts.match(RESERVED_RETURN);
    auto value = (ts.peek(SEMICOLON)) ? std::nullopt : std::make_optional(parseExpression());
    matchExpectedSymbol(SEMICOLON, "at end of return.");
    return std::make_unique<AstNode::Statement::Return>(std::move(value));
}

std::unique_ptr<AstNode::Statement::While> Parser::parseWhileStatement() {
    ts.match(RESERVED_WHILE);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'while'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after while statement condition.");
    auto block = parseBlock();
    return std::make_unique<AstNode::Statement::While>(std::move(condition), std::move(block));
}

std::unique_ptr<AstNode::Statement::For> Parser::parseForStatement() {
    ts.match(RESERVED_FOR);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'for'.");
    auto parseInnerStatement = [this]() -> std::optional<std::unique_ptr<AstNode::Statement>> {
        if (peekTypedDeclaration()) {
            return parseDeclarationStatement();
        }
        else if (!ts.match(SEMICOLON)) {
            return parseAssignmentExpressionStatement();
        }
        else {
            return std::nullopt;
        }
    };
    auto initStatement = parseInnerStatement();
    auto condition = parseInnerStatement();
    auto incrementExpression = (ts.peek(PARENTHESES_CLOSE)) ? std::nullopt : std::make_optional(parseExpression());
    matchExpectedSymbol(PARENTHESES_CLOSE, "after for statement condition statements.");
    std::vector<std::unique_ptr<AstNode::Statement>> block = parseBlock();
    return std::make_unique<AstNode::Statement::For>(std::move(initStatement)
                                                   , std::move(condition)
                                                   , std::move(incrementExpression)
                                                   , std::move(block));
}

std::unique_ptr<AstNode::Statement::If> Parser::parseIfStatement() {
    ts.match(RESERVED_IF);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'if'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after if statement condition.");
    auto block = parseBlock();
    std::vector<std::unique_ptr<AstNode::Statement::If>> elseIfStatements;
    while (ts.peek(RESERVED_ELSE, RESERVED_IF)) {
        elseIfStatements.push_back(parseIfStatement());
    }
    std::vector<std::unique_ptr<AstNode::Statement>> elseBlock;
    if (ts.match(RESERVED_ELSE)) {
        elseBlock = parseBlock();
    }
    return std::make_unique<AstNode::Statement::If>(std::move(condition)
                                                  , std::move(block)
                                                  , std::move(elseIfStatements)
                                                  , std::move(elseBlock));
}

std::unique_ptr<AstNode::Statement::If> Parser::parseElseIfStatement() {
    ts.match(RESERVED_ELSE, RESERVED_IF);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'else if'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after else if statement condition.");
    auto block = parseBlock();
    return std::make_unique<AstNode::Statement::If>(std::move(condition)
                                                  , std::move(block)
                                                  , std::vector<std::unique_ptr<AstNode::Statement::If>>()
                                                  , std::vector<std::unique_ptr<AstNode::Statement>>());
}

std::unique_ptr<AstNode::Expression> Parser::parseExpression() {
    return parseLogicalExpression();
}

std::unique_ptr<AstNode::Expression> Parser::parseLogicalExpression() {
    auto lhs = parseComparisonExpression();
    while (ts.match(LOGICAL_AND) || ts.match(LOGICAL_OR)) {
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseComparisonExpression();

        auto compoundExpression = std::make_unique<AstNode::Expression::Binary>(std::move(op), std::move(lhs), std::move(rhs));
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseComparisonExpression() {
    auto lhs = parseAdditiveExpression();
    while (ts.match(COMPARISON_LT)
        || ts.match(COMPARISON_LE)
        || ts.match(COMPARISON_GT)
        || ts.match(COMPARISON_GE)
        || ts.match(COMPARISON_EQ)
        || ts.match(COMPARISON_NE)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseAdditiveExpression();

        auto compoundExpression = std::make_unique<AstNode::Expression::Binary>(std::move(op), std::move(lhs), std::move(rhs));
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseAdditiveExpression() {
    auto lhs = parseMultiplicativeExpression();
    while (ts.match(ARITHMETIC_ADDITION) || ts.match(ARITHMETIC_SUBTRACTION)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseMultiplicativeExpression();

        auto compoundExpression = std::make_unique<AstNode::Expression::Binary>(std::move(op), std::move(lhs), std::move(rhs));
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseMultiplicativeExpression() {
    auto lhs = parseExponentialExpression();
    while (ts.match(ARITHMETIC_MULTIPLICATION)
        || ts.match(ARITHMETIC_DIVISION)
        || ts.match(ARITHMETIC_REMAINDER)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseExponentialExpression();

        auto compoundExpression = std::make_unique<AstNode::Expression::Binary>(std::move(op), std::move(lhs), std::move(rhs));
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseExponentialExpression() {
    auto lhs = parseUnaryExpression();
    while (ts.match(ARITHMETIC_EXPONENTIATION)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseUnaryExpression();

        auto compoundExpression = std::make_unique<AstNode::Expression::Binary>(std::move(op), std::move(lhs), std::move(rhs));
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseUnaryExpression() {
    std::unique_ptr<AstNode::Expression> expr;
    if (ts.match(UNARY_NOT) || ts.match(UNARY_NEGATIVE)) {
        auto& op = ts.at(-1).getLiteral();
        expr = parseUnaryExpression();
        return std::make_unique<AstNode::Expression::Unary>(std::move(op), std::move(expr));
    }
    else if (ts.match(UNARY_INCREMENT) || ts.match(UNARY_DECREMENT)) { // match pre-(in/de)crement ++a/--a
        auto& op = ts.at(-1).getLiteral();
        expr = parsePrimaryExpression();
        return std::make_unique<AstNode::Expression::Unary>(std::move(op), std::move(expr));
    }
    expr = parsePrimaryExpression();
    if (ts.match(UNARY_INCREMENT) || ts.match(UNARY_DECREMENT)) { // match post-(in/de)crement a++/a--
        auto& op = ts.at(-1).getLiteral();
        return std::make_unique<AstNode::Expression::Unary>(std::move(op), std::move(expr));
    }
    return expr;
}

std::unique_ptr<AstNode::Expression> Parser::parsePrimaryExpression() {
    if (ts.match(LITERAL_TRUE) || ts.match(LITERAL_FALSE)) {
        auto literal = LITERAL_TRUE == ts.at(-1).getLiteral();
        return std::make_unique<AstNode::Expression::Literal>(std::move(literal));
    }
    else if (ts.match(Token::Type::INTEGER)) {
        auto& literalStr = ts.at(-1).getLiteral();
        int base = 10;
        if (literalStr.starts_with("0x") || literalStr.starts_with("0X")) { // hex literal
            base = 16;
        }
        else if (literalStr.starts_with("0b") || literalStr.starts_with("0B")) { // binary literal
            base = 2;
        }
        else if (literalStr.starts_with("0")) { // octal literal
            base = 8;
        }
        auto literal = static_cast<size_t>(std::stoll(literalStr, 0, base));
        return std::make_unique<AstNode::Expression::Literal>(std::move(literal));
    }
    else if (ts.match(Token::Type::FLOAT)) {
        auto literal = std::stod(ts.at(-1).getLiteral());
        return std::make_unique<AstNode::Expression::Literal>(std::move(literal));
    }
    else if (ts.match(Token::Type::STRING)) {
        auto& literal = ts.at(-1).getLiteral();
        // TODO: clean quotes and escapes here
        return std::make_unique<AstNode::Expression::Literal>(std::move(literal));
    }
    // else if (ts.match(BRACKET_OPEN)) { //TODO: array literal
        
    // }
    else if (ts.match(PARENTHESES_OPEN)) {
        auto innerExp = parseExpression();
        matchExpectedSymbol(PARENTHESES_CLOSE, "after grouped expression.");
        return std::make_unique<AstNode::Expression::Group>(std::move(innerExp));
    }
    else if (ts.match(Token::Type::IDENTIFIER, PARENTHESES_OPEN)) { // function call
        auto& name = ts.at(-2).getLiteral();
        std::vector<std::unique_ptr<AstNode::Expression>> arguments;
        if (!ts.peek(PARENTHESES_CLOSE)) {
            do {
                arguments.push_back(parseExpression());
            } while (ts.match(COMMA));
        }
        matchExpectedSymbol(PARENTHESES_CLOSE, "at end of function argument list.");
        return std::make_unique<AstNode::Expression::Function>(std::move(name), std::move(arguments));
    }
    else if (ts.match(Token::Type::IDENTIFIER, MEMBER_ACCESS, Token::Type::IDENTIFIER, PARENTHESES_OPEN)) { // method call
        auto& object = ts.at(-4).getLiteral();
        auto& methodName = ts.at(-2).getLiteral();
        std::vector<std::unique_ptr<AstNode::Expression>> arguments;
        if (!ts.peek(PARENTHESES_CLOSE)) {
            do {
                arguments.push_back(parseExpression());
            } while (ts.match(COMMA));
        }
        matchExpectedSymbol(PARENTHESES_CLOSE, "at end of method argument list.");
        return std::make_unique<AstNode::Expression::Method>(std::move(object)
                                                           , std::move(methodName)
                                                           , std::move(arguments));
    }
    else if (ts.match(Token::Type::IDENTIFIER)) {
        auto& object = ts.at(-1).getLiteral();
        if (ts.match(MEMBER_ACCESS)) { // member access
            if (!ts.match(Token::Type::IDENTIFIER)) {
                throw ParseException("Expected data member or method call after '.' operator.", ts.getLine(), ts.getColumn());
            }
            auto& member = ts.at(-1).getLiteral();
            return std::make_unique<AstNode::Expression::Access>(std::move(object), std::move(member));
        }
        else if (ts.match(BRACKET_OPEN)) { // subscript access
            auto subscript = parseExpression();
            matchExpectedSymbol(BRACKET_CLOSE, "to match previous '[' in expression.");
            return std::make_unique<AstNode::Expression::Access>(std::move(object), std::move(subscript));
        }
        return std::make_unique<AstNode::Expression::Access>(std::move(object));
    }
    throw ParseException("Invalid Expression.", ts.getLine(), ts.getColumn());
}

std::unique_ptr<AstNode::Specifier> Parser::parseSpecifier() {
    return parseTypeSpecifier();
}

std::unique_ptr<AstNode::Specifier::Type> Parser::parseTypeSpecifier() {
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw ParseException("Invalid type specifier.", ts.getLine(), ts.getColumn());
    }
    auto& name = ts.at(-1).getLiteral();
    std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs;
    if (ts.match(TYPE_DELIMITER_OPEN)) {
        if (!CONTAINER_TYPES.contains(name)) {
            throw ParseException(invalidContainerMessage(), ts.getLine(), ts.getColumn());
        }
        do {
            typeArgs.push_back(parseTypeSpecifier());
        } while (ts.match(COMMA));
        matchExpectedSymbol(TYPE_DELIMITER_CLOSE, "to match previous '<' in type specifier.");
    }
    return std::make_unique<AstNode::Specifier::Type>(std::move(name), std::move(typeArgs));
}

bool Parser::peekTypedDeclaration() {
    return ts.peek(Token::Type::IDENTIFIER, Token::Type::IDENTIFIER) || peekNestedTypeSpecifier();
}

bool Parser::peekNestedTypeSpecifier() {
    if (!ts.peek(Token::Type::IDENTIFIER, TYPE_DELIMITER_OPEN)) return false;
    return CONTAINER_TYPES.contains(ts.at(0).getLiteral());
}

constexpr const char * const Parser::invalidContainerMessage() {
    return "Invalid container type. Container must be one of: { "
    #define CONTAINER_BEGIN(name, _) #name ", " 
    #define METHOD(__, ___)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
    "\b\b }";
}

void Parser::matchExpectedSymbol(std::string&& symbol, std::string&& message) {
    if (!ts.match(symbol)) {
        throw ParseException("Expected '" + symbol + "' " + message, ts.getLine(), ts.getColumn());
    }
}