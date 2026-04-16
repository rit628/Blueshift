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

            Token(Type type
                , const std::string& literal
                , size_t absIdx = 0
                , size_t lineStart = 0
                , size_t colStart = 0
                , size_t lineEnd = 0
                , size_t colEnd = 0)
                : type(type)
                , literal(literal)
                , absIdx(absIdx)
                , lineStart(lineStart)
                , colStart(colStart)
                , lineEnd(lineEnd)
                , colEnd(colEnd) {}
            Type getType() const { return type; }
            std::string getTypeName() const { return typeStrings.at(static_cast<int>(type)); }
            const std::string& getLiteral() const { return literal; }
            size_t getAbsIdx() const { return absIdx; }
            size_t getLineStart() const { return lineStart; }
            size_t getColStart() const { return colStart; }
            size_t getLineEnd() const { return lineEnd; }
            size_t getColEnd() const { return colEnd; }
            bool operator==(const Token&) const = default;

        private:
            enum Type type;
            std::string literal;
            size_t absIdx, lineStart, colStart, lineEnd, colEnd;
    };

}
