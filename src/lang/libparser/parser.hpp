#pragma once
#include "liblexer/token.hpp"
#include "ast.hpp"
#include "token_stream.hpp"
#include <memory>
#include <vector>
#include <sstream>

namespace BlsLang {
    
    class Parser {
        public:
            std::unique_ptr<AstNode::Source> parse(std::vector<Token> tokenStream);

        private:
            TokenStream ts;

            std::unique_ptr<AstNode::Source> parseSource();
            std::unique_ptr<AstNode::Setup> parseSetup();
            std::unique_ptr<AstNode::Function> parseFunction();
            std::vector<std::unique_ptr<AstNode::Statement>> parseBlock();
            std::unique_ptr<AstNode::Statement> parseStatement();
            std::unique_ptr<AstNode::Statement> parseAssignmentExpressionStatement();
            std::unique_ptr<AstNode::Statement::Declaration> parseDeclarationStatement();
            std::unique_ptr<AstNode::Statement::Return> parseReturnStatement();
            std::unique_ptr<AstNode::Statement::While> parseWhileStatement();
            std::unique_ptr<AstNode::Statement::For> parseForStatement();
            std::unique_ptr<AstNode::Statement::If> parseIfStatement();
            std::unique_ptr<AstNode::Statement::If> parseElseIfStatement();
            std::unique_ptr<AstNode::Expression> parseExpression();
            std::unique_ptr<AstNode::Expression> parseLogicalExpression();
            std::unique_ptr<AstNode::Expression> parseComparisonExpression();
            std::unique_ptr<AstNode::Expression> parseAdditiveExpression();
            std::unique_ptr<AstNode::Expression> parseMultiplicativeExpression();
            std::unique_ptr<AstNode::Expression> parseExponentialExpression();
            std::unique_ptr<AstNode::Expression> parseUnaryExpression();
            std::unique_ptr<AstNode::Expression> parsePrimaryExpression();
            std::unique_ptr<AstNode::Specifier> parseSpecifier();
            std::unique_ptr<AstNode::Specifier::Type> parseTypeSpecifier();

            // helpers
            void matchExpectedSymbol(std::string&& symbol, std::string&& message);
    };

    class ParseException : public std::exception {
        public:
            explicit ParseException(const std::string& message, size_t line, size_t col) {
                std::ostringstream os;
                os << "Ln " << line << ", Col " << col << ": " << message;
                this->message = os.str();
            }
        
            const char* what() const noexcept override { return message.c_str(); }

        private:
            std::string message;
    };

}