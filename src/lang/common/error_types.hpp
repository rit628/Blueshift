#pragma once
#include<sstream>
#include <exception>

namespace BlsLang {

    class SyntaxError : public std::exception {
        public:
            explicit SyntaxError(const std::string& message, size_t line, size_t col) {
                std::ostringstream os;
                os << "Ln " << line << ", Col " << col << ": " << message;
                this->message = os.str();
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