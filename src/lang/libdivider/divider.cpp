#include "divider.hpp"
#include "ast.hpp"
#include <variant>

using namespace BlsLang; 

BlsObject Divider::visit(AstNode::Source &ast){

    return std::monostate(); 

}