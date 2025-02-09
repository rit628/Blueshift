#include "parser.hpp"
#include "ast.hpp"
#include "liblexer/token.hpp"
#include "include/reserved_tokens.hpp"
#include <memory>
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

    while (ts.peek(Token::Type::IDENTIFIER, Token::Type::IDENTIFIER, "(") ||
           ts.peek(RESERVED_SETUP, "(", ")")) {

        if (ts.peek(RESERVED_SETUP)) {
            if (setup == nullptr) {
                throw ParseException("Only one setup function allowed per source file.", ts.getLine(), ts.getColumn());
            }
            setup = parseSetup();
        }
        else if (ts.peek(RESERVED_OBLOCK)) {
            oblocks.push_back(parseOblock());
        }
        else {
            procedures.push_back(parseProcedure());
        }

    }

    if (!ts.empty()) {
        throw ParseException("Invalid top level element.", ts.getLine(), ts.getColumn());
    }
    else if (setup == nullptr) {
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
    std::vector<std::string> parameterTypes;
    std::vector<std::string> parameters;
    parseFunctionParams(parameterTypes, parameters);
    auto statements = parseBlock();
    return std::make_unique<AstNode::Function::Oblock>(std::move(name)
                                                        , std::move(parameterTypes)
                                                        , std::move(parameters)
                                                        , std::move(statements));
}

std::unique_ptr<AstNode::Function::Procedure> Parser::parseProcedure() {
    auto name = ts.at(0).getLiteral();
    auto returnType = ts.at(1).getLiteral();
    ts.match(Token::Type::IDENTIFIER, Token::Type::IDENTIFIER, "(");
    std::vector<std::string> parameterTypes;
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

}

std::unique_ptr<AstNode::Statement> Parser::parseStatement() {

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

void Parser::parseFunctionParams(std::vector<std::string>& parameterTypes, std::vector<std::string>& parameters) {
    if (!ts.peek(")")) {
        do {
            if (ts.peek(Token::Type::IDENTIFIER, Token::Type::IDENTIFIER)) {
                parameterTypes.push_back(ts.at(0).getLiteral());
                parameters.push_back(ts.at(1).getLiteral());
                ts.match(Token::Type::IDENTIFIER, Token::Type::IDENTIFIER);
            }
            else {
                ts.match(Token::Type::IDENTIFIER); // Move index forward if necessary for error reporting
                throw ParseException("Invalid function parameter.", ts.getLine(), ts.getColumn());
            }
        } while (ts.match(","));
    }
    if (!ts.match(")")) {
        throw new ParseException("Unclosed function parameter list.", ts.getLine(), ts.getColumn());
    }
}