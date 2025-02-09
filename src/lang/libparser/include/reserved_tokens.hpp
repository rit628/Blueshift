#pragma once

namespace BlsLang {

    #define DEVTYPE_BEGIN(typename) \
    constexpr auto DEVTYPE_##typename (#typename);
    #define ATTRIBUTE(_, __)
    #define DEVTYPE_END
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    
    constexpr auto PRIMITIVE_BOOL                       ("bool");
    constexpr auto PRIMITIVE_INT                        ("int");
    constexpr auto PRIMITIVE_FLOAT                      ("float");
    constexpr auto PRIMITIVE_STRING                     ("string");

    constexpr auto CONTAINER_ARRAY                      ("array");
    constexpr auto CONTAINER_MAP                        ("map");
        
    constexpr auto LITERAL_TRUE                         ("true");
    constexpr auto LITERAL_FALSE                        ("false");

    constexpr auto RESERVED_IF                          ("if");
    constexpr auto RESERVED_ELSE                        ("else");
    constexpr auto RESERVED_FOR                         ("for");
    constexpr auto RESERVED_WHILE                       ("while");
    constexpr auto RESERVED_RETURN                      ("return");

    constexpr auto RESERVED_OBLOCK                      ("oblock");
    constexpr auto RESERVED_SETUP                       ("setup");

    constexpr auto BLOCK_OPEN                           ("{");
    constexpr auto BLOCK_CLOSE                          ("}");

    constexpr auto PARENTHESES_OPEN                     ("(");
    constexpr auto PARENTHESES_CLOSE                    (")");

    constexpr auto ARGUMENT_SEPERATOR                   (",");

    constexpr auto STATEMENT_TERMINATOR                 (";");
    
    constexpr auto MEMBER_ACCESS                        (".");

    constexpr auto SUBSCRIPT_ACCESS_OPEN                ("[");
    constexpr auto SUBSCRIPT_ACCESS_CLOSE               ("]");

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
    constexpr auto ASSIGNMENT_EXPONENTIATION            ("^=");
    constexpr auto ASSIGNMENT_REMAINDER                 ("%=");

}