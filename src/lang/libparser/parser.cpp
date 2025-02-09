#include "parser.hpp"
#include "ast.hpp"
#include "liblexer/token.hpp"
#include "include/reserved_tokens.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using namespace BlsLang;

std::unique_ptr<AstNode::Source> Parser::parse(std::vector<Token> tokenStream) {
    ts.setStream(tokenStream);
    return parseSource();
}

std::unique_ptr<AstNode::Source> Parser::parseSource() {
    std::vector<std::unique_ptr<AstNode::Function::Procedure>> procedures;
    std::vector<std::unique_ptr<AstNode::Function::Oblock>> oblocks;
    std::unique_ptr<AstNode::Setup> setup = nullptr;

    while (!ts.empty()) {
        if (ts.peek(RESERVED_SETUP)) {
            if (setup == nullptr) {
                throw ParseException("Only one setup function allowed per source file.", ts.getLine(), ts.getColumn());
            }
            setup = parseSetup();
        }
        else if (ts.peek(RESERVED_OBLOCK)) {
            oblocks.push_back(parseOblock());
        }
        else if (ts.peek(Token::Type::IDENTIFIER)) {
            procedures.push_back(parseProcedure());
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
    ts.match(RESERVED_SETUP, "(", ")");
    auto statements = parseBlock();
    return std::make_unique<AstNode::Setup>(std::move(statements));
}

std::unique_ptr<AstNode::Function::Oblock> Parser::parseOblock() {
    auto name = ts.at(1).getLiteral();
    ts.match(RESERVED_OBLOCK, Token::Type::IDENTIFIER, "(");
    std::vector<std::vector<std::string>> parameterTypes;
    std::vector<std::string> parameters;
    parseFunctionParams(parameterTypes, parameters);
    auto statements = parseBlock();
    return std::make_unique<AstNode::Function::Oblock>(std::move(name)
                                                        , std::move(parameterTypes)
                                                        , std::move(parameters)
                                                        , std::move(statements));
}

std::unique_ptr<AstNode::Function::Procedure> Parser::parseProcedure() {
    auto returnType = parseTypeIdentifier();
    auto name = ts.at(0).getLiteral();
    ts.match(Token::Type::IDENTIFIER, "(");
    std::vector<std::vector<std::string>> parameterTypes;
    std::vector<std::string> parameters;
    parseFunctionParams(parameterTypes, parameters);
    auto statements = parseBlock();
    return std::make_unique<AstNode::Function::Procedure>(std::move(name)
                                                        , std::move(returnType)
                                                        , std::move(parameterTypes)
                                                        , std::move(parameters)
                                                        , std::move(statements));
}

std::vector<std::unique_ptr<AstNode::Statement>> Parser::parseBlock() {
    if (!ts.match("{")) {
        throw ParseException("Missing opening brace for block body.", ts.getLine(), ts.getColumn());
    }
    std::vector<std::unique_ptr<AstNode::Statement>> body;
    while (!ts.peek("}") && !ts.empty()) {
        body.push_back(parseStatement());
    }
    if (!ts.match("}")) {
        throw ParseException("Unclosed block body.", ts.getLine(), ts.getColumn());
    }
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

}

std::unique_ptr<AstNode::Statement::Expression> Parser::parseExpressionStatement() {

}

std::unique_ptr<AstNode::Statement::Assignment> Parser::parseAssignmentStatement() {

}

std::unique_ptr<AstNode::Statement::Declaration> Parser::parseDeclarationStatement() {

}

std::unique_ptr<AstNode::Statement::Return> Parser::parseReturnStatement() {

}

std::unique_ptr<AstNode::Statement::While> Parser::parseWhileStatement() {

}

std::unique_ptr<AstNode::Statement::For> Parser::parseForStatement() {

}

std::unique_ptr<AstNode::Statement::If> Parser::parseIfStatement() {

}

std::unique_ptr<AstNode::Expression> Parser::parseExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parseLogicalExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parseComparisonExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parseAdditiveExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parseMultiplicativeExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parseExponentialExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parseUnaryExpression() {

}

std::unique_ptr<AstNode::Expression> Parser::parsePrimaryExpression() {

}

std::vector<std::string> Parser::parseTypeIdentifier() {
    std::vector<std::string> type;
    size_t delimCount = 0;
    do {
        if (!ts.match(Token::Type::IDENTIFIER)) {
            throw ParseException("Invalid type identifier.", ts.getLine(), ts.getColumn());
        }
        type.push_back(ts.at(-1).getLiteral());
        delimCount++;
    } while (ts.match(TYPE_DELIMITER_OPEN));

    delimCount--; // remove one delimiter to account for initial identifier
    for (size_t i = 0; i < delimCount; i++) {
        if (!ts.match(TYPE_DELIMITER_CLOSE)) {
            throw ParseException("Unclosed type identifier.", ts.getLine(), ts.getColumn());
        }
    }
    return type;
}

void Parser::parseFunctionParams(std::vector<std::vector<std::string>>& parameterTypes, std::vector<std::string>& parameters) {
    if (!ts.peek(")")) {
        do {
            parameterTypes.push_back(parseTypeIdentifier());
            if (ts.match(Token::Type::IDENTIFIER)) {
                parameters.push_back(ts.at(-1).getLiteral());
            }
            else {
                throw ParseException("Invalid function parameter.", ts.getLine(), ts.getColumn());
            }
        } while (ts.match(","));
    }
    if (!ts.match(")")) {
        throw new ParseException("Unclosed function parameter list.", ts.getLine(), ts.getColumn());
    }
}