#pragma once
#include "ast.hpp"
#include <cstddef>
#include<sstream>
#include <exception>
#include <utility>

namespace BlsLang {

    class SyntaxError : public std::exception {
        public:
            explicit SyntaxError(const std::string& message, size_t line, size_t col) {
                std::ostringstream os;
                os << "Ln " << line << ", Col " << col << ": " << message;
                this->message = os.str();
            }

            explicit SyntaxError(const std::string& message, std::pair<size_t, size_t> location) {
                SyntaxError(message, location.first, location.second);
            }
        
            const char* what() const noexcept override { return message.c_str(); }

        private:
            std::string message;
    };

    class SemanticError : public std::exception {
        public:
            explicit SemanticError(const std::string& message) {
                std::ostringstream os;
                os << message;
                this->message = os.str();
            }

            explicit SemanticError(const std::string& message, const AstNode& ast) {
                std::ostringstream os;
                os << "Ln " << ast.lineStart << "-" << ast.lineEnd
                << ", Col " << ast.columnStart << "-" << ast.columnEnd
                << ": " << message;
                SemanticError(os.str());
            }
        
            const char* what() const noexcept override { return message.c_str(); }

        private:
            std::string message;
    };

    class RuntimeError : public std::exception {
        public:
            explicit RuntimeError(const std::string& message) {
                std::ostringstream os;
                os << message;
                this->message = os.str();
            }
        
            const char* what() const noexcept override { return message.c_str(); }

        private:
            std::string message;
    };

}