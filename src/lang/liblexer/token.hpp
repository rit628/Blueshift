#pragma once
#include <cstddef>
#include <string>

namespace BlsLang {

    class Token {
        public:
            enum class Type {
                IDENTIFIER,
                INTEGER,
                DECIMAL,
                STRING,
                OPERATOR,
                COMMENT
            };

            Token(Type type, const std::string& literal, size_t absIdx, size_t lineNum, size_t colNum)
                : type(type)
                , literal(literal)
                , absIdx(absIdx)
                , lineNum(lineNum)
                , colNum(colNum) {}
            Type getType() const { return type; }
            const std::string& getLiteral() const { return literal; }
            size_t getAbsIdx() const { return absIdx; }
            size_t getLineNum() const { return lineNum; }
            size_t getColNum() const { return colNum; }

        private:
            enum Type type;
            const std::string literal;
            size_t absIdx, lineNum, colNum;
    };

}
