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
    
}

std::unique_ptr<AstNode::Setup> Parser::parseSetup() {

}

std::unique_ptr<AstNode::Function::Oblock> Parser::parseOblock() {

}

std::unique_ptr<AstNode::Function::Procedure> Parser::parseProcedure() {

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

