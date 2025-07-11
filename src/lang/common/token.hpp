#pragma once
#include <cstddef>
#include <string>
#include <array>

namespace BlsLang {

    class Token {
        public:
            enum class Type {
                IDENTIFIER,
                INTEGER,
                FLOAT,
                STRING,
                COMMENT,
                OPERATOR,
                COUNT
            };

            static constexpr std::array<const char *, static_cast<int>(Type::COUNT)> typeStrings {
                "IDENTIFIER",
                "INTEGER",
                "FLOAT",
                "STRING",
                "COMMENT",
                "OPERATOR"
            };

            Token(Type type, const std::string& literal, size_t absIdx = 0, size_t lineNum = 0, size_t colNum = 0)
                : type(type)
                , literal(literal)
                , absIdx(absIdx)
                , lineNum(lineNum)
                , colNum(colNum) {}
            Type getType() const { return type; }
            std::string getTypeName() const { return typeStrings.at(static_cast<int>(type)); }
            const std::string& getLiteral() const { return literal; }
            size_t getAbsIdx() const { return absIdx; }
            size_t getLineNum() const { return lineNum; }
            size_t getColNum() const { return colNum; }
            bool operator==(const Token&) const = default;

        private:
            enum Type type;
            std::string literal;
            size_t absIdx, lineNum, colNum;
    };

}
