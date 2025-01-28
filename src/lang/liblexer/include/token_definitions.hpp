#pragma once
#include <boost/regex.hpp>

namespace BlsLang {

    const boost::regex IDENTIFIER_START (R"(@|[A-Za-z])");
    const boost::regex IDENTIFIER_END (R"([A-Za-z0-9_-])");

    const boost::regex NUMERIC_DIGIT (R"([0-9])");
    const boost::regex NEGATIVE_SIGN (R"(-)");
    const boost::regex DECIMAL_POINT (R"(\.)");

    const boost::regex ESCAPE_SLASH (R"(\\)");
    const boost::regex ESCAPE_CHARS (R"([bnrt'\"\\])");

    const boost::regex STRING_QUOTE (R"(\")");
    const boost::regex STRING_LITERALS (R"([^\"\n\r\\])"); // Or escapes

    const boost::regex COMMENT_SLASH ("/");
    const boost::regex COMMENT_STAR (R"(\*)");
    const boost::regex COMMENT_CONTENTS_SINGLELINE (R"([^\n])");
    const boost::regex COMMENT_CONTENTS_MULTILINE (R"([^*])");

    const boost::regex OPERATOR_EQUALS_PREFIX(R"([<>!=+\-*/^%])");
    const boost::regex OPERATOR_EQUALS(R"(\=)");
    const boost::regex OPERATOR_PLUS(R"(\+)");
    const boost::regex OPERATOR_MINUS(R"(\-)");
    const boost::regex OPERATOR_AND(R"(\&)");
    const boost::regex OPERATOR_OR(R"(\|)");
    const boost::regex OPERATOR_GENERIC(R"(.)");

    const boost::regex WHITESPACE (R"([ \u0008\n\r\t])");
}
