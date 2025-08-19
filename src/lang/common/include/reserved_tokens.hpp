#pragma once
#include <unordered_set>
#include <string>
#include "libtype/bls_types.hpp"

namespace BlsLang {

    constexpr auto LITERAL_TRUE                         ("true");
    constexpr auto LITERAL_FALSE                        ("false");
    
    constexpr auto RESERVED_IF                          ("if");
    constexpr auto RESERVED_ELSE                        ("else");
    constexpr auto RESERVED_FOR                         ("for");
    constexpr auto RESERVED_DO                          ("do");
    constexpr auto RESERVED_WHILE                       ("while");
    constexpr auto RESERVED_RETURN                      ("return");
    constexpr auto RESERVED_BREAK                       ("break");
    constexpr auto RESERVED_CONTINUE                    ("continue");

    constexpr auto RESERVED_VIRTUAL                     ("virtual");
    
    constexpr auto RESERVED_OBLOCK                      ("oblock");
    constexpr auto RESERVED_MASTER                      ("MASTER"); 
    constexpr auto RESERVED_SETUP                       ("setup");

    constexpr auto BRACE_OPEN                           ("{");
    constexpr auto BRACE_CLOSE                          ("}");

    constexpr auto PARENTHESES_OPEN                     ("(");
    constexpr auto PARENTHESES_CLOSE                    (")");

    constexpr auto BRACKET_OPEN                         ("[");
    constexpr auto BRACKET_CLOSE                        ("]");

    constexpr auto COMMA                                (",");

    constexpr auto COLON                                (":");

    constexpr auto SEMICOLON                            (";");
    
    constexpr auto MEMBER_ACCESS                        (".");

    constexpr auto TYPE_DELIMITER_OPEN                  ("<");
    constexpr auto TYPE_DELIMITER_CLOSE                 (">");

    constexpr auto UNARY_NOT                            ("!");
    constexpr auto UNARY_NEGATIVE                       ("-");
    constexpr auto UNARY_INCREMENT                      ("++");
    constexpr auto UNARY_DECREMENT                      ("--");

    constexpr auto LOGICAL_AND                          ("&&");
    constexpr auto LOGICAL_OR                           ("||");

    constexpr auto COMPARISON_LT                        ("<");
    constexpr auto COMPARISON_GT                        (">");
    constexpr auto COMPARISON_LE                        ("<=");
    constexpr auto COMPARISON_GE                        (">=");
    constexpr auto COMPARISON_NE                        ("!=");
    constexpr auto COMPARISON_EQ                        ("==");

    constexpr auto ARITHMETIC_EXPONENTIATION            ("^");
    constexpr auto ARITHMETIC_MULTIPLICATION            ("*");
    constexpr auto ARITHMETIC_DIVISION                  ("/");
    constexpr auto ARITHMETIC_REMAINDER                 ("%");
    constexpr auto ARITHMETIC_ADDITION                  ("+");
    constexpr auto ARITHMETIC_SUBTRACTION               ("-");

    constexpr auto ASSIGNMENT                           ("=");
    constexpr auto ASSIGNMENT_ADDITION                  ("+=");
    constexpr auto ASSIGNMENT_SUBTRACTION               ("-=");
    constexpr auto ASSIGNMENT_MULTIPLICATION            ("*=");
    constexpr auto ASSIGNMENT_DIVISION                  ("/=");
    constexpr auto ASSIGNMENT_REMAINDER                 ("%=");
    constexpr auto ASSIGNMENT_EXPONENTIATION            ("^=");

    const static std::unordered_set<std::string> TYPES {
          PRIMITIVE_VOID
        , PRIMITIVE_BOOL
        , PRIMITIVE_INT
        , PRIMITIVE_FLOAT
        , PRIMITIVE_STRING
        , CONTAINER_LIST
        , CONTAINER_MAP

        #define DEVTYPE_BEGIN(name, ...) \
        , DEVTYPE_##name
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END

        , SPECIAL_ANY
    };

    const static std::unordered_set<std::string> MODIFIERS {
        RESERVED_VIRTUAL
    };

}