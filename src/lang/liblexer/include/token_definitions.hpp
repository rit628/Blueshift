#pragma once
#include <boost/regex.hpp>

namespace BlsLang {

    const static boost::regex IDENTIFIER_START                      (R"([A-Za-z])");
    const static boost::regex IDENTIFIER_END                        (R"([A-Za-z0-9_])");

    const static char ZERO                                          ('0');
    const static char NEGATIVE_SIGN                                 ('-');
    const static char DECIMAL_POINT                                 ('.');
    const static boost::regex NUMERIC_DIGIT                         (R"([0-9])");
    const static boost::regex OCTAL_DIGIT                           (R"([0-7])");
    const static boost::regex HEX_START                             (R"([xX])");
    const static boost::regex HEX_DIGIT                             (R"([0-9a-fA-F])");
    const static boost::regex BINARY_START                          (R"([bB])");
    const static boost::regex BINARY_DIGIT                          (R"([0-1])");

    const static char ESCAPE_SLASH                                  ('\\');
    const static boost::regex ESCAPE_CHARS                          (R"([bnrt'\"\\])");

    const static char STRING_QUOTE                                  ('"');
    const static boost::regex STRING_LITERALS                       (R"([^\"\n\r\\])"); // or escapes

    const static char COMMENT_SLASH                                 ('/');
    const static char COMMENT_STAR                                  ('*');
    const static boost::regex COMMENT_CONTENTS_SINGLELINE           (R"([^\n])");
    const static boost::regex COMMENT_CONTENTS_MULTILINE            (R"(.)");

    const static char OPERATOR_EQUALS                               ('=');
    const static char OPERATOR_PLUS                                 ('+');
    const static char OPERATOR_MINUS                                ('-');
    const static char OPERATOR_AND                                  ('&');
    const static char OPERATOR_OR                                   ('|');
    const static boost::regex OPERATOR_EQUALS_PREFIX                (R"([<>!=+\-*/^%])");
    const static boost::regex OPERATOR_GENERIC                      (R"(.)");

    const static boost::regex WHITESPACE                            ("[ \u000B\u0008\\n\\r\\f\\t]"); // not a raw string to include unicode characters
}