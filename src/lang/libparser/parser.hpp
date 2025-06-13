#pragma once
#include "token.hpp"
#include "ast.hpp"
#include "token_stream.hpp"
#include <memory>
#include <vector>

namespace BlsLang {
    
    class Parser {
        public:
            friend class ParserTest;
            std::unique_ptr<AstNode::Source> parse(std::vector<Token> tokenStream);

        private:
            TokenStream ts;

            std::unique_ptr<AstNode::Source> parseSource();
            std::unique_ptr<AstNode::Setup> parseSetup();
            std::unique_ptr<AstNode::Function> parseFunction();
            std::vector<std::unique_ptr<AstNode::Statement>> parseBlock();
            std::unique_ptr<AstNode::Statement> parseStatement();
            std::unique_ptr<AstNode::Statement::Expression> parseExpressionStatement();
            std::unique_ptr<AstNode::Statement::Declaration> parseDeclarationStatement();
            std::unique_ptr<AstNode::Statement::Return> parseReturnStatement();
            std::unique_ptr<AstNode::Statement::While> parseWhileStatement();
            std::unique_ptr<AstNode::Statement::While> parseDoWhileStatement();
            std::unique_ptr<AstNode::Statement::For> parseForStatement();
            std::unique_ptr<AstNode::Statement::If> parseIfStatement();
            std::unique_ptr<AstNode::Statement::If> parseElseIfStatement();
            std::unique_ptr<AstNode::Expression> parseExpression();
            std::unique_ptr<AstNode::Expression> parseAssignmentExpression();
            std::unique_ptr<AstNode::Expression> parseLogicalExpression();
            std::unique_ptr<AstNode::Expression> parseComparisonExpression();
            std::unique_ptr<AstNode::Expression> parseAdditiveExpression();
            std::unique_ptr<AstNode::Expression> parseMultiplicativeExpression();
            std::unique_ptr<AstNode::Expression> parseExponentialExpression();
            std::unique_ptr<AstNode::Expression> parseUnaryExpression();
            std::unique_ptr<AstNode::Expression> parsePrimaryExpression();
            std::unique_ptr<AstNode::Specifier> parseSpecifier();
            std::unique_ptr<AstNode::Specifier::Type> parseTypeSpecifier();
            std::unique_ptr<AstNode::Initializer::Oblock> parseOblockInitializer();

            // helpers
            void cleanLiteral(std::string& literal);
            bool peekModifier();
            bool peekTypeSpecifier();
            void matchExpectedSymbol(std::string&& symbol, std::string&& message);
    };

}