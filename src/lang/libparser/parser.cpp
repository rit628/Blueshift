#include "parser.hpp"
#include "ast.hpp"
#include "include/reserved_tokens.hpp"
#include "error_types.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <boost/regex.hpp>

using namespace BlsLang;

template<std::derived_from<AstNode> Node, typename... Args>
std::unique_ptr<Node> createNode(size_t lineStart, size_t columnStart, size_t lineEnd, size_t columnEnd, Args&&... args) {
    auto result = std::make_unique<Node>(std::move(args)...);
    result->lineStart = lineStart;
    result->columnStart = columnStart;
    result->lineEnd = lineEnd;
    result->columnEnd = columnEnd;
    return result;
}

template<std::derived_from<AstNode> Node, typename... Args>
std::unique_ptr<Node> createNode(std::pair<size_t, size_t> startLocation, std::pair<size_t, size_t> endLocation, Args&&... args) {
    return createNode<Node>(startLocation.first, startLocation.second, endLocation.first, endLocation.second, std::move(args)...);
}

std::unique_ptr<AstNode::Source> Parser::parse(std::vector<Token> tokenStream) {
    ts.setStream(tokenStream);
    return parseSource();
}

std::unique_ptr<AstNode::Source> Parser::parseSource() {
    auto startLocation = ts.getLocation();
    std::vector<std::unique_ptr<AstNode::Function>> procedures;
    std::vector<std::unique_ptr<AstNode::Function>> tasks;
    std::unique_ptr<AstNode::Setup> setup = nullptr;

    while (!ts.empty()) {
        if (ts.peek(RESERVED_SETUP)) {
            if (setup != nullptr) {
                throw SyntaxError("Only one setup function allowed per source file.", ts.getLocation());
            }
            setup = parseSetup();
        }
        else if (ts.peek(RESERVED_TASK)) {
            tasks.push_back(parseFunction());
        }
        else if (ts.peek(Token::Type::IDENTIFIER)) {
            procedures.push_back(parseFunction());
        }
        else {
            throw SyntaxError("Invalid top level element.", ts.getLocation());
        }
    }

    if (setup == nullptr) {
        throw SyntaxError("No setup function found in file.", ts.getLocation());
    }

    return createNode<AstNode::Source>(startLocation, ts.getLocation()
                                      , procedures
                                      , tasks
                                      , setup);
}

std::unique_ptr<AstNode::Setup> Parser::parseSetup() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_SETUP, PARENTHESES_OPEN, PARENTHESES_CLOSE);
    auto statements = parseBlock();
    return createNode<AstNode::Setup>(startLocation, ts.getLocation(), statements);
}

std::unique_ptr<AstNode::Function> Parser::parseFunction() {
    auto startLocation = ts.getLocation();
    std::unique_ptr<AstNode::Specifier::Type> returnType = nullptr;
    if (!ts.match(RESERVED_TASK)) {
        returnType = parseTypeSpecifier();
    }
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw SyntaxError("Expected valid identifier for function name.", ts.getLocation());
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
                throw SyntaxError("Invalid function parameter.", ts.getLocation());
            }
        } while (ts.match(COMMA));
    }
    matchExpectedSymbol(PARENTHESES_CLOSE, "at end of function parameter list.");
    if (returnType) { // parsed procedure
        auto statements = parseBlock();
        return createNode<AstNode::Function::Procedure>(startLocation, ts.getLocation()
                                                       , name
                                                       , returnType
                                                       , parameterTypes
                                                       , parameters
                                                       , statements);
    }
    else { // parsed task
        std::vector<std::unique_ptr<AstNode::Initializer::Task>> configOptions;
        if (ts.match(COLON)) {
            do {
                configOptions.push_back(parseTaskInitializer());
            } while (ts.match(COMMA));
        }
        auto statements = parseBlock();
        return createNode<AstNode::Function::Task>(startLocation, ts.getLocation()
                                                  , name
                                                  , parameterTypes
                                                  , parameters
                                                  , configOptions
                                                  , statements);
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
    auto startLocation = ts.getLocation();
    if (ts.peek(RESERVED_IF)) {
        return parseIfStatement();
    }
    else if (ts.peek(RESERVED_FOR)) {
        return parseForStatement();
    }
    else if (ts.peek(RESERVED_WHILE)) {
        return parseWhileStatement();
    }
    else if (ts.peek(RESERVED_DO)) {
        return parseDoWhileStatement();
    }
    else if (ts.peek(RESERVED_RETURN)) {
        return parseReturnStatement();
    }
    else if (ts.match(RESERVED_BREAK)) {
        matchExpectedSymbol(SEMICOLON, "after break statement.");
        return createNode<AstNode::Statement::Break>(startLocation, ts.getLocation());
    }
    else if (ts.match(RESERVED_CONTINUE)) {
        matchExpectedSymbol(SEMICOLON, "after continue statement.");
        return createNode<AstNode::Statement::Continue>(startLocation, ts.getLocation());
    }
    else if (peekTypeSpecifier() || peekModifier()) {
        return parseDeclarationStatement();
    }
    else {
        return parseExpressionStatement();
    }
}

std::unique_ptr<AstNode::Statement::Expression> Parser::parseExpressionStatement() {
    auto startLocation = ts.getLocation();
    auto expression = parseExpression();
    matchExpectedSymbol(SEMICOLON, "at end of expression.");
    return createNode<AstNode::Statement::Expression>(startLocation, ts.getLocation(), expression);
}

std::unique_ptr<AstNode::Statement::Declaration> Parser::parseDeclarationStatement() {
    auto startLocation = ts.getLocation();
    std::unordered_set<std::string> modifiers;
    while (peekModifier()) {
        ts.match(Token::Type::IDENTIFIER);
        auto& modifier = ts.at(-1).getLiteral();
        if (modifiers.contains(modifier)) {
            std::cerr << "Duplicate " << modifier << " declaration modifier." << std::endl;
        }
        modifiers.emplace(modifier);
    }
    auto type = parseTypeSpecifier();
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw SyntaxError("Expected valid identifier for variable name.", ts.getLocation());
    }
    auto& name = ts.at(-1).getLiteral();
    auto rhs = (ts.match(ASSIGNMENT)) ? std::make_optional(parseExpression()) : std::nullopt;
    matchExpectedSymbol(SEMICOLON, "at end of declaration.");
    return createNode<AstNode::Statement::Declaration>(startLocation, ts.getLocation()
                                                      , name
                                                      , modifiers
                                                      , type
                                                      , rhs);
}

std::unique_ptr<AstNode::Statement::Return> Parser::parseReturnStatement() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_RETURN);
    auto value = (ts.peek(SEMICOLON)) ? std::nullopt : std::make_optional(parseExpression());
    matchExpectedSymbol(SEMICOLON, "at end of return.");
    return createNode<AstNode::Statement::Return>(startLocation, ts.getLocation(), value);
}

std::unique_ptr<AstNode::Statement::While> Parser::parseWhileStatement() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_WHILE);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'while'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after while statement condition.");
    auto block = parseBlock();
    return createNode<AstNode::Statement::While>(startLocation, ts.getLocation(), condition, block);
}

std::unique_ptr<AstNode::Statement::While> Parser::parseDoWhileStatement() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_DO);
    auto block = parseBlock();
    matchExpectedSymbol(RESERVED_WHILE, "after 'do' block.");
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'while'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after while statement condition.");
    matchExpectedSymbol(SEMICOLON, "after while statement condition.");
    return createNode<AstNode::Statement::While>(startLocation, ts.getLocation()
                                                , condition
                                                , block
                                                , AstNode::Statement::While::LOOP_TYPE::DO);
}

std::unique_ptr<AstNode::Statement::For> Parser::parseForStatement() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_FOR);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'for'.");
    auto parseInnerStatement = [this]() -> std::optional<std::unique_ptr<AstNode::Statement>> {
        if (peekTypeSpecifier() || peekModifier()) {
            return parseDeclarationStatement();
        }
        else if (!ts.match(SEMICOLON)) {
            return parseExpressionStatement();
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
    return createNode<AstNode::Statement::For>(startLocation, ts.getLocation()
                                              , initStatement
                                              , condition
                                              , incrementExpression
                                              , block);
}

std::unique_ptr<AstNode::Statement::If> Parser::parseIfStatement() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_IF);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'if'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after if statement condition.");
    auto block = parseBlock();
    std::vector<std::unique_ptr<AstNode::Statement::If>> elseIfStatements;
    while (ts.peek(RESERVED_ELSE, RESERVED_IF)) {
        elseIfStatements.push_back(parseElseIfStatement());
    }
    std::vector<std::unique_ptr<AstNode::Statement>> elseBlock;
    if (ts.match(RESERVED_ELSE)) {
        elseBlock = parseBlock();
    }
    return createNode<AstNode::Statement::If>(startLocation, ts.getLocation()
                                             , condition
                                             , block
                                             , elseIfStatements
                                             , elseBlock);
}

std::unique_ptr<AstNode::Statement::If> Parser::parseElseIfStatement() {
    auto startLocation = ts.getLocation();
    ts.match(RESERVED_ELSE, RESERVED_IF);
    matchExpectedSymbol(PARENTHESES_OPEN, "after 'else if'.");
    auto condition = parseExpression();
    matchExpectedSymbol(PARENTHESES_CLOSE, "after else if statement condition.");
    auto block = parseBlock();
    return createNode<AstNode::Statement::If>(startLocation, ts.getLocation()
                                             , condition
                                             , block
                                             , std::vector<std::unique_ptr<AstNode::Statement::If>>()
                                             , std::vector<std::unique_ptr<AstNode::Statement>>());
}

std::unique_ptr<AstNode::Expression> Parser::parseExpression() {
    return parseAssignmentExpression();
}

std::unique_ptr<AstNode::Expression> Parser::parseAssignmentExpression() {
    auto startLocation = ts.getLocation();
    auto lhs = parseLogicalExpression();
    if (ts.match(ASSIGNMENT)
     || ts.match(ASSIGNMENT_ADDITION)
     || ts.match(ASSIGNMENT_SUBTRACTION)
     || ts.match(ASSIGNMENT_MULTIPLICATION)
     || ts.match(ASSIGNMENT_DIVISION)
     || ts.match(ASSIGNMENT_REMAINDER)
     || ts.match(ASSIGNMENT_EXPONENTIATION)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseExpression(); // right associative, build rhs completely first, then combine with lhs

        auto compoundExpression = createNode<AstNode::Expression::Binary>(startLocation, ts.getLocation(), op, lhs, rhs);
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseLogicalExpression() {
    /*
        for consistency with right associative expressions, left associative expressions
        will index from location of leftmost sub-expression rather than from current operator
    */
    auto startLocation = ts.getLocation();
    auto lhs = parseComparisonExpression();
    while (ts.match(LOGICAL_AND) || ts.match(LOGICAL_OR)) {
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseComparisonExpression();

        auto compoundExpression = createNode<AstNode::Expression::Binary>(startLocation, ts.getLocation(), op, lhs, rhs);
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseComparisonExpression() {
    /*
        for consistency with right associative expressions, left associative expressions
        will index from location of leftmost sub-expression rather than from current operator
    */
    auto startLocation = ts.getLocation();
    auto lhs = parseAdditiveExpression();
    while (ts.match(COMPARISON_LT)
        || ts.match(COMPARISON_LE)
        || ts.match(COMPARISON_GT)
        || ts.match(COMPARISON_GE)
        || ts.match(COMPARISON_EQ)
        || ts.match(COMPARISON_NE)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseAdditiveExpression();

        auto compoundExpression = createNode<AstNode::Expression::Binary>(startLocation, ts.getLocation(), op, lhs, rhs);
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseAdditiveExpression() {
    /*
        for consistency with right associative expressions, left associative expressions
        will index from location of leftmost sub-expression rather than from current operator
    */
    auto startLocation = ts.getLocation();
    auto lhs = parseMultiplicativeExpression();
    while (ts.match(ARITHMETIC_ADDITION) || ts.match(ARITHMETIC_SUBTRACTION)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseMultiplicativeExpression();

        auto compoundExpression = createNode<AstNode::Expression::Binary>(startLocation, ts.getLocation(), op, lhs, rhs);
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseMultiplicativeExpression() {
    /*
        for consistency with right associative expressions, left associative expressions
        will index from location of leftmost sub-expression rather than from current operator
    */
    auto startLocation = ts.getLocation();
    auto lhs = parseExponentialExpression();
    while (ts.match(ARITHMETIC_MULTIPLICATION)
        || ts.match(ARITHMETIC_DIVISION)
        || ts.match(ARITHMETIC_REMAINDER)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseExponentialExpression();

        auto compoundExpression = createNode<AstNode::Expression::Binary>(startLocation, ts.getLocation(), op, lhs, rhs);
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseExponentialExpression() {
    /*
        for consistency with right associative expressions, left associative expressions
        will index from location of leftmost sub-expression rather than from current operator
    */
    auto startLocation = ts.getLocation();
    auto lhs = parseUnaryExpression();
    while (ts.match(ARITHMETIC_EXPONENTIATION)) {
            
        auto& op = ts.at(-1).getLiteral();
        auto rhs = parseUnaryExpression();

        auto compoundExpression = createNode<AstNode::Expression::Binary>(startLocation, ts.getLocation(), op, lhs, rhs);
        lhs = std::move(compoundExpression);
    }
    return lhs;
}

std::unique_ptr<AstNode::Expression> Parser::parseUnaryExpression() {
    auto startLocation = ts.getLocation();
    std::unique_ptr<AstNode::Expression> expr;
    if (ts.match(UNARY_NOT) || ts.match(UNARY_NEGATIVE)) {
        auto& op = ts.at(-1).getLiteral();
        expr = parseUnaryExpression();
        return createNode<AstNode::Expression::Unary>(startLocation, ts.getLocation(), op, expr);
    }
    else if (ts.match(UNARY_INCREMENT) || ts.match(UNARY_DECREMENT)) { // match pre-(in/de)crement ++a/--a
        auto& op = ts.at(-1).getLiteral();
        expr = parsePrimaryExpression();
        return createNode<AstNode::Expression::Unary>(startLocation, ts.getLocation(), op, expr);
    }
    expr = parsePrimaryExpression();
    if (ts.match(UNARY_INCREMENT) || ts.match(UNARY_DECREMENT)) { // match post-(in/de)crement a++/a--
        auto& op = ts.at(-1).getLiteral();
        return createNode<AstNode::Expression::Unary>(startLocation, ts.getLocation()
                                                     , op
                                                     , expr
                                                     , AstNode::Expression::Unary::OPERATOR_POSITION::POSTFIX);
    }
    return expr;
}

std::unique_ptr<AstNode::Expression> Parser::parsePrimaryExpression() {
    auto startLocation = ts.getLocation();
    if (ts.match(LITERAL_TRUE) || ts.match(LITERAL_FALSE)) {
        auto literal = LITERAL_TRUE == ts.at(-1).getLiteral();
        return createNode<AstNode::Expression::Literal>(startLocation, ts.getLocation(), literal);
    }
    else if (ts.match(Token::Type::INTEGER)) {
        auto& literalStr = ts.at(-1).getLiteral();
        size_t index = 0;
        int base = 10;
        if (literalStr.starts_with("0x") || literalStr.starts_with("0X")) { // hex literal
            base = 16;
            index += 2;
        }
        else if (literalStr.starts_with("0b") || literalStr.starts_with("0B")) { // binary literal
            base = 2;
            index += 2;
        }
        else if (literalStr.starts_with("0")) { // octal literal
            base = 8;
            index++;
        }
        index = std::min(index, literalStr.size() - 1);
        auto literal = static_cast<int64_t>(std::stoll(literalStr.data() + index, 0, base));
        return createNode<AstNode::Expression::Literal>(startLocation, ts.getLocation(), literal);
    }
    else if (ts.match(Token::Type::FLOAT)) {
        auto literal = std::stod(ts.at(-1).getLiteral());
        return createNode<AstNode::Expression::Literal>(startLocation, ts.getLocation(), literal);
    }
    else if (ts.match(Token::Type::STRING)) {
        auto literal = ts.at(-1).getLiteral();
        cleanLiteral(literal);
        return createNode<AstNode::Expression::Literal>(startLocation, ts.getLocation(), literal);
    }
    else if (ts.match(BRACKET_OPEN)) { // list expression
        std::vector<std::unique_ptr<AstNode::Expression>> elements;
        if (!ts.peek(BRACKET_CLOSE)) {
            do {
                elements.push_back(parseExpression());
            } while (ts.match(COMMA));
        }
        matchExpectedSymbol(BRACKET_CLOSE, "at end of list expression.");
        return createNode<AstNode::Expression::List>(startLocation, ts.getLocation(), elements);
    }
    else if (ts.match(BRACE_OPEN)) {
        if (ts.peek(BRACE_CLOSE)) { // No elements, default to set
            return createNode<AstNode::Expression::Set>(startLocation, ts.getLocation());
        }
        auto key = parseExpression();
        if (ts.match(COLON)) { // map expression
            std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements;
            auto value = parseExpression();
            elements.push_back(std::make_pair(std::move(key), std::move(value)));
            while (ts.match(COMMA)) {
                key = parseExpression();
                matchExpectedSymbol(COLON, "after key in map expression.");
                value = parseExpression();
                elements.push_back(std::make_pair(std::move(key), std::move(value)));
            }
            matchExpectedSymbol(BRACE_CLOSE, "at end of map expression.");
            return createNode<AstNode::Expression::Map>(startLocation, ts.getLocation(), elements);
        }
        else { // set expression
            std::vector<std::unique_ptr<AstNode::Expression>> elements;
            elements.push_back(std::move(key));
            while (ts.match(COMMA)) {
                elements.push_back(parseExpression());
            }
            matchExpectedSymbol(BRACE_CLOSE, "at end of set expression.");
            return createNode<AstNode::Expression::Set>(startLocation, ts.getLocation(), elements);
        }
    }
    else if (ts.match(PARENTHESES_OPEN)) {
        auto innerExp = parseExpression();
        matchExpectedSymbol(PARENTHESES_CLOSE, "after grouped expression.");
        return createNode<AstNode::Expression::Group>(startLocation, ts.getLocation(), innerExp);
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
        return createNode<AstNode::Expression::Function>(startLocation, ts.getLocation(), name, arguments);
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
        return createNode<AstNode::Expression::Method>(startLocation, ts.getLocation(), object
                                                           , methodName
                                                           , arguments);
    }
    else if (ts.match(Token::Type::IDENTIFIER)) {
        auto& object = ts.at(-1).getLiteral();
        if (ts.match(MEMBER_ACCESS)) { // member access
            if (!ts.match(Token::Type::IDENTIFIER)) {
                throw SyntaxError("Expected data member or method call after '.' operator.", ts.getLocation());
            }
            auto& member = ts.at(-1).getLiteral();
            return createNode<AstNode::Expression::Access>(startLocation, ts.getLocation(), object, member);
        }
        else if (ts.match(BRACKET_OPEN)) { // subscript access
            auto subscript = parseExpression();
            matchExpectedSymbol(BRACKET_CLOSE, "to match previous '[' in expression.");
            return createNode<AstNode::Expression::Access>(startLocation, ts.getLocation(), object, subscript);
        }
        return createNode<AstNode::Expression::Access>(startLocation, ts.getLocation(), object);
    }
    throw SyntaxError("Invalid Expression.", ts.getLocation());
}

std::unique_ptr<AstNode::Specifier> Parser::parseSpecifier() {
    return parseTypeSpecifier();
}

std::unique_ptr<AstNode::Specifier::Type> Parser::parseTypeSpecifier() {
    auto startLocation = ts.getLocation();
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw SyntaxError("Invalid type specifier.", ts.getLocation());
    }
    auto& name = ts.at(-1).getLiteral();
    std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs;
    if (ts.match(TYPE_DELIMITER_OPEN)) {
        do {
            typeArgs.push_back(parseTypeSpecifier());
        } while (ts.match(COMMA));
        matchExpectedSymbol(TYPE_DELIMITER_CLOSE, "to match previous '<' in type specifier.");
    }
    return createNode<AstNode::Specifier::Type>(startLocation, ts.getLocation(), name, typeArgs);
}

std::unique_ptr<AstNode::Initializer::Task> Parser::parseTaskInitializer() {
    auto startLocation = ts.getLocation();
    if (!ts.match(Token::Type::IDENTIFIER)) {
        throw SyntaxError("Invalid task initialization option", ts.getLocation());
    }
    auto& option = ts.at(-1).getLiteral();
    std::vector<std::unique_ptr<AstNode::Expression>> args;
    if (ts.match(PARENTHESES_OPEN)) {
        do {
            args.push_back(parseExpression());
        } while (ts.match(COMMA));
        matchExpectedSymbol(PARENTHESES_CLOSE, "to match previous ')' in task initialization option.");
    }
    return createNode<AstNode::Initializer::Task>(startLocation, ts.getLocation(), option, args);
}

void Parser::cleanLiteral(std::string& literal) {
    literal = literal.substr(1, literal.size() - 2); // remove start and end quotes
    static const boost::regex escapes(R"(\\([bnrt'\"\\]))");
    auto replacements = [](const boost::smatch& match) -> std::string {
        std::string matched = match[1].str();
        if (matched == "b") return "\\";
        if (matched == "n")  return "\n";
        if (matched == "r")  return "\r";
        if (matched == "t")  return "\t";
        if (matched == "'")  return "'";
        if (matched == "\"") return "\"";
        if (matched == "\\") return "\\";
        return matched;
    };
    literal = boost::regex_replace(literal, escapes, replacements, boost::match_default | boost::format_all);
}

bool Parser::peekModifier() {
    return MODIFIERS.contains(ts.at(0).getLiteral());
}

bool Parser::peekTypeSpecifier() {
    return TYPES.contains(ts.at(0).getLiteral());
}

void Parser::matchExpectedSymbol(std::string&& symbol, std::string&& message) {
    if (!ts.match(symbol)) {
        throw SyntaxError("Expected '" + symbol + "' " + message, ts.getLocation());
    }
}