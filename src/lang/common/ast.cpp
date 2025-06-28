#include "ast.hpp"
#include "visitor.hpp"
#include "print_visitor.hpp"
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <variant>

using namespace BlsLang;

#define AST_NODE(Node, Base) \
BlsObject Node::accept(Visitor& v) { return v.visit(*this); } \
std::unique_ptr<AstNode> Node::clone() const { return std::unique_ptr<AstNode>(new Node(*this)); } \
std::unique_ptr<Base> Node::cloneBase() const { return std::unique_ptr<Base>(new Node(*this)); } \
Node& Node::operator=(const Node& rhs) { \
    auto temp(rhs); \
    std::swap(temp, *this); \
    return *this; \
}
#include "include/NODE_TYPES.LIST"
#undef AST_NODE

/* AstNode::Initializer::Oblock */
AstNode::Initializer::Oblock::Oblock(std::string option, std::vector<std::unique_ptr<AstNode::Expression>> args)
                                   : option(std::move(option))
                                   , args(std::move(args))
                                   {}

AstNode::Initializer::Oblock::Oblock(std::string option, std::initializer_list<AstNode::Expression*> args)
                                   : option(std::move(option))
{
    for (auto&& arg : args) {
        this->args.push_back(std::unique_ptr<AstNode::Expression>(arg));
    }
}

AstNode::Initializer::Oblock::Oblock(const Oblock& other) {
    this->option = other.option;
    for (auto&& arg : other.args) {
        this->args.push_back(arg->cloneBase());
    }
}

/* AstNode::Specifier::Type */
AstNode::Specifier::Type::Type(std::string name, std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs)
                             : name(std::move(name))
                             , typeArgs(std::move(typeArgs))
                             {}

AstNode::Specifier::Type::Type(std::string name, std::initializer_list<AstNode::Specifier::Type*> typeArgs)
                             : name(std::move(name))
{
    for (auto&& arg : typeArgs) {
        this->typeArgs.push_back(std::unique_ptr<AstNode::Specifier::Type>(arg));
    }
}

AstNode::Specifier::Type::Type(const AstNode::Specifier::Type& other) {
    this->name = other.name;
    for (auto&& arg : other.typeArgs) {
        this->typeArgs.emplace_back(new AstNode::Specifier::Type(*arg));
    }
}

/* AstNode::Expression::Literal */
AstNode::Expression::Literal::Literal(int64_t literal) : literal(std::move(literal)) {}

AstNode::Expression::Literal::Literal(double literal) : literal(std::move(literal)) {}

AstNode::Expression::Literal::Literal(bool literal) : literal(std::move(literal)) {}

AstNode::Expression::Literal::Literal(std::string literal) : literal(std::move(literal)) {}

AstNode::Expression::Literal::Literal(const AstNode::Expression::Literal& other) {
    this->literal = other.literal;
}

/* AstNode::Expression::List */
AstNode::Expression::List::List(std::vector<std::unique_ptr<AstNode::Expression>> elements, BlsType literal)
                              : elements(std::move(elements))
                              , literal(literal)
                              {}

AstNode::Expression::List::List(std::initializer_list<AstNode::Expression*> elements, BlsType literal)
                              : literal(literal)
{
    for (auto&& element : elements) {
        this->elements.push_back(std::unique_ptr<AstNode::Expression>(element));
    }
}

AstNode::Expression::List::List(const AstNode::Expression::List& other) {
    this->literal = other.literal;
    for (auto&& element : other.elements) {
        this->elements.push_back(element->cloneBase());
    }
}

/* AstNode::Expression::Set */
AstNode::Expression::Set::Set(std::vector<std::unique_ptr<AstNode::Expression>> elements)
                            : elements(std::move(elements))
                            {}

AstNode::Expression::Set::Set(std::initializer_list<AstNode::Expression*> elements)
{
    for (auto&& element : elements) {
        this->elements.push_back(std::unique_ptr<AstNode::Expression>(element));
    }
}

AstNode::Expression::Set::Set(const AstNode::Expression::Set& other) {
    for (auto&& element : other.elements) {
        this->elements.push_back(element->cloneBase());
    }
}

/* AstNode::Expression::Map */
AstNode::Expression::Map::Map(std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements, BlsType literal)
                            : elements(std::move(elements))
                            , literal(literal)
                            {}

AstNode::Expression::Map::Map(std::initializer_list<std::initializer_list<AstNode::Expression*>> elements, BlsType literal)
                            : literal(literal)
{
    for (const auto& pair : elements) {
        if (pair.size() != 2) {
            throw std::invalid_argument("Initializer list must consist only of pairs.");
        }
        auto it = pair.begin();
        auto key = std::unique_ptr<AstNode::Expression>(*it);
        auto value = std::unique_ptr<AstNode::Expression>(*(++it));
        this->elements.push_back(std::make_pair(std::move(key), std::move(value)));
    }
}

AstNode::Expression::Map::Map(const AstNode::Expression::Map& other) {
    for (auto&& [key, value] : other.elements) {
        this->elements.push_back({key->cloneBase(), value->cloneBase()});
    }
    this->literal = other.literal;
}

/* AstNode::Expression::Access */
AstNode::Expression::Access::Access(std::string object, uint8_t localIndex)
                                  : object(std::move(object))
                                  , subscript(std::move(std::nullopt))
                                  , member(std::move(std::nullopt))
                                  , localIndex(localIndex)
                                  {}

AstNode::Expression::Access::Access(std::string object, std::unique_ptr<AstNode::Expression> subscript, uint8_t localIndex)
                                  : object(std::move(object))
                                  , subscript(std::move(subscript))
                                  , member(std::move(std::nullopt))
                                  , localIndex(localIndex)
                                  {}

AstNode::Expression::Access::Access(std::string object, AstNode::Expression* subscript, uint8_t localIndex)
                                  : object(std::move(object))
                                  , subscript(std::move(subscript))
                                  , member(std::move(std::nullopt))
                                  , localIndex(localIndex)
                                  {}

AstNode::Expression::Access::Access(std::string object, std::string member, uint8_t localIndex)
                                  : object(std::move(object))
                                  , subscript(std::move(std::nullopt))
                                  , member(std::move(member))
                                  , localIndex(localIndex)
                                  {}

AstNode::Expression::Access::Access(const Access& other) {
    this->object = other.object;
    this->subscript = std::nullopt;
    if (other.subscript.has_value()) {
        this->subscript = (*other.subscript)->cloneBase();
    }
    this->member = other.member;
    this->localIndex = other.localIndex;
}

/* AstNode::Expression::Function */
AstNode::Expression::Function::Function(std::string name, std::vector<std::unique_ptr<AstNode::Expression>> arguments)
                                      : name(std::move(name))
                                      , arguments(std::move(arguments))
                                      {}

AstNode::Expression::Function::Function(std::string name, std::initializer_list<AstNode::Expression*> arguments)
                                      : name(std::move(name))
{
    for (auto&& arg : arguments) {
        this->arguments.push_back(std::unique_ptr<AstNode::Expression>(arg));
    }
}

AstNode::Expression::Function::Function(const Function& other) {
    this->name = other.name;
    for (auto&& arg : other.arguments) {
        this->arguments.push_back(arg->cloneBase());
    }
}

/* AstNode::Expression::Method */
AstNode::Expression::Method::Method(std::string object
                                  , std::string methodName
                                  , std::vector<std::unique_ptr<AstNode::Expression>> arguments
                                  , uint8_t localIndex
                                  , TYPE objectType)
                                  : object(std::move(object))
                                  , methodName(std::move(methodName))
                                  , arguments(std::move(arguments))
                                  , localIndex(localIndex)
                                  , objectType(objectType)
                                  {}

AstNode::Expression::Method::Method(std::string object
                                  , std::string methodName
                                  , std::initializer_list<AstNode::Expression*> arguments
                                  , uint8_t localIndex
                                  , TYPE objectType)
                                  : object(std::move(object))
                                  , methodName(std::move(methodName))
                                  , localIndex(localIndex)
                                  , objectType(objectType)
{
    for (auto&& arg : arguments) {
        this->arguments.push_back(std::unique_ptr<AstNode::Expression>(arg));
    }
}

AstNode::Expression::Method::Method(const Method& other) {
    this->object = other.object;
    this->methodName = other.methodName;
    for (auto&& arg : other.arguments) {
        this->arguments.push_back(arg->cloneBase());
    }
    this->localIndex = other.localIndex;
    this->objectType = other.objectType;
}

/* AstNode::Expression::Group */
AstNode::Expression::Group::Group(std::unique_ptr<AstNode::Expression> expression)
                                : expression(std::move(expression))
                                {}

AstNode::Expression::Group::Group(AstNode::Expression* expression)
                                : expression(expression)
                                {}

AstNode::Expression::Group::Group(const Group& other) {
    this->expression = other.expression->cloneBase();
}

/* AstNode::Expression::Unary */
AstNode::Expression::Unary::Unary(std::string op, std::unique_ptr<AstNode::Expression> expression, OPERATOR_POSITION position)
                                : op(std::move(op))
                                , expression(std::move(expression))
                                , position(std::move(position))
                                {}
            
AstNode::Expression::Unary::Unary(std::string op, AstNode::Expression* expression, OPERATOR_POSITION position)
                                : op(std::move(op))
                                , expression(expression)
                                , position(position)
                                {}
            
AstNode::Expression::Unary::Unary(const AstNode::Expression::Unary& other) {
    this->op = other.op;
    this->expression = other.expression->cloneBase();
    this->position = other.position;
}

/* AstNode::Expression::Binary */
AstNode::Expression::Binary::Binary(std::string op
                                  , std::unique_ptr<AstNode::Expression> left
                                  , std::unique_ptr<AstNode::Expression> right)
                                  : op(std::move(op))
                                  , left(std::move(left))
                                  , right(std::move(right))
                                  {}
        
AstNode::Expression::Binary::Binary(std::string op
                                  , AstNode::Expression* left
                                  , AstNode::Expression* right)
                                  : op(std::move(op))
                                  , left(left)
                                  , right(right)
                                  {}
        
AstNode::Expression::Binary::Binary(const AstNode::Expression::Binary& other) {
    this->op = other.op;
    this->left = other.left->cloneBase();
    this->right = other.right->cloneBase();
}

/* AstNode::Statement::Expression */
AstNode::Statement::Expression::Expression(std::unique_ptr<AstNode::Expression> expression) : expression(std::move(expression)) {}

AstNode::Statement::Expression::Expression(AstNode::Expression* expression) : expression(expression) {}

AstNode::Statement::Expression::Expression(const AstNode::Statement::Expression& other) {
    this->expression = other.expression->cloneBase();
}

/* AstNode::Statement::Declaration */
AstNode::Statement::Declaration::Declaration(std::string name
                                           , std::unordered_set<std::string> modifiers
                                           , std::unique_ptr<AstNode::Specifier::Type> type
                                           , std::optional<std::unique_ptr<AstNode::Expression>> value
                                           , uint8_t localIndex)
                                           : name(std::move(name))
                                           , modifiers(modifiers)
                                           , type(std::move(type))
                                           , value(std::move(value))
                                           , localIndex(localIndex)
                                           {}
            
AstNode::Statement::Declaration::Declaration(std::string name
                                           , std::unordered_set<std::string> modifiers
                                           , AstNode::Specifier::Type* type
                                           , std::optional<AstNode::Expression*> value
                                           , uint8_t localIndex)
                                           : name(std::move(name))
                                           , modifiers(modifiers)
                                           , type(type)
                                           , value(value)
                                           , localIndex(localIndex)
                                           {}
            
AstNode::Statement::Declaration::Declaration(const AstNode::Statement::Declaration& other) {
    this->name = other.name;
    this->modifiers = other.modifiers;
    this->type = std::make_unique<AstNode::Specifier::Type>(*other.type);
    this->value = std::nullopt;
    if (other.value.has_value()) {
        this->value = (*other.value)->cloneBase();
    }
    this->localIndex = other.localIndex;
}

/* AstNode::Statement::Return */
AstNode::Statement::Return::Return(std::optional<std::unique_ptr<AstNode::Expression>> value) : value(std::move(value)) {}

AstNode::Statement::Return::Return(AstNode::Expression* value) : value(value) {}

AstNode::Statement::Return::Return(const AstNode::Statement::Return& other) {
    this->value = std::nullopt;
    if (other.value.has_value()) {
        this->value = (*other.value)->cloneBase();
    }
}

/* AstNode::Statement::While */
AstNode::Statement::While::While(std::unique_ptr<AstNode::Expression> condition
                               , std::vector<std::unique_ptr<AstNode::Statement>> block
                               , LOOP_TYPE type)
                               : condition(std::move(condition))
                               , block(std::move(block))
                               , type(type)
                               {}
            
AstNode::Statement::While::While(AstNode::Expression* condition
                               , std::initializer_list<AstNode::Statement*> block
                               , LOOP_TYPE type)
                               : condition(condition)
                               , block()
                               , type(type)
{
    for (auto stmt : block){
        this->block.push_back(std::unique_ptr<AstNode::Statement>(stmt));
    }
}
            
AstNode::Statement::While::While(const AstNode::Statement::While& other) {
    this->condition = other.condition->cloneBase();
    for (auto&& stmt : other.block) {
        this->block.push_back(stmt->cloneBase());
    }
    this->type = other.type;
}

/* AstNode::Statement::For */
AstNode::Statement::For::For(std::optional<std::unique_ptr<AstNode::Statement>> initStatement
                           , std::optional<std::unique_ptr<AstNode::Statement>> condition
                           , std::optional<std::unique_ptr<AstNode::Expression>> incrementExpression
                           , std::vector<std::unique_ptr<AstNode::Statement>> block)
                           : initStatement(std::move(initStatement))
                           , condition(std::move(condition))
                           , incrementExpression(std::move(incrementExpression))
                           , block(std::move(block))
                           {}
            
AstNode::Statement::For::For(std::optional<AstNode::Statement*> initStatement
                           , std::optional<AstNode::Statement*> condition
                           , std::optional<AstNode::Expression*> incrementExpression
                           , std::initializer_list<AstNode::Statement*> block)
                           : initStatement(initStatement)
                           , condition(condition)
                           , incrementExpression(incrementExpression)
{
    for (auto stmt : block) {
        this->block.push_back(std::unique_ptr<AstNode::Statement>(stmt));
    }
}
            
AstNode::Statement::For::For(const AstNode::Statement::For& other) {
    this->initStatement = std::nullopt;
    if (other.initStatement.has_value()) {
        this->initStatement = (*other.initStatement)->cloneBase();
    }
    this->condition = std::nullopt;
    if (other.condition.has_value()) {
        this->condition = (*other.condition)->cloneBase();
    }
    this->incrementExpression = std::nullopt;
    if (other.incrementExpression.has_value()) {
        this->incrementExpression = (*other.incrementExpression)->cloneBase();
    }
    for (auto&& stmt : other.block) {
        this->block.push_back(stmt->cloneBase());
    }
}

/* AstNode::Statement::If */
AstNode::Statement::If::If(std::unique_ptr<AstNode::Expression> condition
                         , std::vector<std::unique_ptr<AstNode::Statement>> block
                         , std::vector<std::unique_ptr<AstNode::Statement::If>> elseIfStatements
                         , std::vector<std::unique_ptr<AstNode::Statement>> elseBlock)
                         : condition(std::move(condition))
                         , block(std::move(block))
                         , elseIfStatements(std::move(elseIfStatements))
                         , elseBlock(std::move(elseBlock))
                         {}
            
AstNode::Statement::If::If(AstNode::Expression* condition
                         , std::initializer_list<AstNode::Statement*> block
                         , std::initializer_list<AstNode::Statement::If*> elseIfStatements
                         , std::initializer_list<AstNode::Statement*> elseBlock)
                         : condition(condition)
{
    for (auto stmt : block) {
        this->block.push_back(std::unique_ptr<AstNode::Statement>(stmt));
    }
    for (auto stmt : elseIfStatements) {
        this->elseIfStatements.push_back(std::unique_ptr<AstNode::Statement::If>(stmt));
    }
    for (auto stmt : elseBlock) {
        this->elseBlock.push_back(std::unique_ptr<AstNode::Statement>(stmt));
    }
}
            
AstNode::Statement::If::If(const AstNode::Statement::If& other) {
    this->condition = other.condition->cloneBase();
    for (auto&& stmt : other.block) {
        this->block.push_back(stmt->cloneBase());
    }
    for (auto&& stmt : other.elseIfStatements) {
        this->elseIfStatements.push_back(std::make_unique<AstNode::Statement::If>(*stmt));
    }
    for (auto&& stmt : other.elseBlock) {
        this->elseBlock.push_back(stmt->cloneBase());
    }
}

/* AstNode::Function */
AstNode::Function::Function(std::string name
                          , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                          , std::vector<std::string> parameters
                          , std::vector<std::unique_ptr<AstNode::Statement>> statements)
                          : name(std::move(name))
                          , parameterTypes(std::move(parameterTypes))
                          , parameters(std::move(parameters))
                          , statements(std::move(statements))
                          {}
            
AstNode::Function::Function(std::string name
                          , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                          , std::vector<std::string> parameters
                          , std::initializer_list<AstNode::Statement*> statements)
                          : name(std::move(name))
                          , parameterTypes()
                          , parameters(std::move(parameters))
                          , statements()
{
    for (auto pt : parameterTypes) {
        this->parameterTypes.push_back(std::unique_ptr<AstNode::Specifier::Type>(pt));
    }
    for (auto stmt : statements) {
        this->statements.push_back(std::unique_ptr<AstNode::Statement>(stmt));
    }
}

/* AstNode::Function::Procedure */
AstNode::Function::Procedure::Procedure(std::string name
                                      , std::unique_ptr<AstNode::Specifier::Type> returnType
                                      , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                                      , std::vector<std::string> parameters
                                      , std::vector<std::unique_ptr<AstNode::Statement>> statements)
                                      :
                      AstNode::Function(std::move(name)
                                      , std::move(parameterTypes)
                                      , std::move(parameters)
                                      , std::move(statements))
                                      , returnType(std::move(returnType))
                                      {}
            
AstNode::Function::Procedure::Procedure(std::string name
                                      , AstNode::Specifier::Type* returnType
                                      , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                                      , std::vector<std::string> parameters
                                      , std::initializer_list<AstNode::Statement*> statements)
                                      :
                      AstNode::Function(std::move(name)
                                      , std::move(parameterTypes)
                                      , std::move(parameters)
                                      , std::move(statements))
                                      , returnType(returnType)
                                      {}
            
AstNode::Function::Procedure::Procedure(const AstNode::Function::Procedure& other) {
    this->name = other.name;
    for (auto&& type : other.parameterTypes) {
        this->parameterTypes.push_back(std::make_unique<AstNode::Specifier::Type>(*type));
    }
    this->parameters = other.parameters;
    for (auto&& stmt : other.statements) {
        this->statements.push_back(stmt->cloneBase());
    }
    this->returnType = std::make_unique<AstNode::Specifier::Type>(*other.returnType);
}

/* AstNode::Function::Oblock */
AstNode::Function::Oblock::Oblock(std::string name
                                , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                                , std::vector<std::string> parameters
                                , std::vector<std::unique_ptr<AstNode::Initializer::Oblock>> configOptions
                                , std::vector<std::unique_ptr<AstNode::Statement>> statements)
                                :
                         Function(std::move(name)
                                , std::move(parameterTypes)
                                , std::move(parameters)
                                , std::move(statements))
                                , configOptions(std::move(configOptions))
                                {}
            
AstNode::Function::Oblock::Oblock(std::string name
                                , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                                , std::vector<std::string> parameters
                                , std::initializer_list<AstNode::Initializer::Oblock*> configOptions
                                , std::initializer_list<AstNode::Statement*> statements)
                                :
                         Function(std::move(name)
                                , std::move(parameterTypes)
                                , std::move(parameters)
                                , std::move(statements))
{
    for (auto&& option : configOptions) {
        this->configOptions.push_back(std::unique_ptr<AstNode::Initializer::Oblock>(option));
    }
}
            
AstNode::Function::Oblock::Oblock(const AstNode::Function::Oblock& other) {
    this->name = other.name;
    for (auto&& type : other.parameterTypes) {
        this->parameterTypes.push_back(std::make_unique<AstNode::Specifier::Type>(*type));
    }
    this->parameters = other.parameters;
    for (auto&& stmt : other.statements) {
        this->statements.push_back(stmt->cloneBase());
    }
    for (auto&& option : other.configOptions) {
        this->configOptions.push_back(std::make_unique<AstNode::Initializer::Oblock>(*option));
    }
}

/* AstNode::Setup */
AstNode::Setup::Setup(std::vector<std::unique_ptr<AstNode::Statement>> statements)
                    : statements(std::move(statements))
                    {}
            
AstNode::Setup::Setup(std::initializer_list<AstNode::Statement*> statements) {
    for (auto stmt : statements) {
        this->statements.push_back(std::unique_ptr<AstNode::Statement>(stmt));
    }
}
            
AstNode::Setup::Setup(const AstNode::Setup& other) {
    for (auto&& stmt : other.statements) {
        this->statements.push_back(stmt->cloneBase());
    }
}

/* AstNode::Source */
AstNode::Source::Source(std::vector<std::unique_ptr<AstNode::Function>> procedures
                      , std::vector<std::unique_ptr<AstNode::Function>> oblocks
                      , std::unique_ptr<AstNode::Setup> setup)
                      : procedures(std::move(procedures))
                      , oblocks(std::move(oblocks))
                      , setup(std::move(setup))
                      {}
            
AstNode::Source::Source(std::initializer_list<AstNode::Function*> procedures
                      , std::initializer_list<AstNode::Function*> oblocks
                      , AstNode::Setup* setup)
{
    for (auto proc : procedures) {
        this->procedures.push_back(std::unique_ptr<AstNode::Function>(proc));
    }
    for (auto obl : oblocks) {
        this->oblocks.push_back(std::unique_ptr<AstNode::Function>(obl));
    }
    this->setup.reset(setup);
}
            
AstNode::Source::Source(const AstNode::Source& other) {
    for (auto&& proc : other.procedures) {
        this->procedures.push_back(proc->cloneBase());
    }
    for (auto&& obl : other.oblocks) {
        this->oblocks.push_back(obl->cloneBase());
    }
    this->setup = nullptr;
    if (other.setup) {
        this->setup = std::make_unique<AstNode::Setup>(*other.setup);
    }
}

std::ostream& BlsLang::operator<<(std::ostream& os, const AstNode& node) {
    Printer printer(os);
    const_cast<AstNode&>(node).accept(printer);
    return os;
}
