#include <boost/regex.hpp>

namespace BlsLang {

    const boost::regex IDENTIFIER_START ("@|[A-Za-z]");
    const boost::regex IDENTIFIER_END ("[A-Za-z0-9_-]");

    const boost::regex NUMERIC_DIGIT ("[0-9]");
    const boost::regex NEGATIVE_SIGN ("-");
    const boost::regex DECIMAL_POINT (R"(\\.)");

    const boost::regex ESCAPE_SLASH (R"(\\)");
    const boost::regex ESCAPE_CHARS (R"([bnrt'\"\\])");

    const boost::regex STRING_QUOTE (R"(\")");
    const boost::regex STRING_LITERALS (R"([^\"\n\r\\])"); // Or escapes

    const boost::regex COMMENT_SLASH ("/");
    const boost::regex COMMENT_STAR (R"(\*)");

    const boost::regex WHITESPACE (R"([ \u0008\n\r\t])");

}
