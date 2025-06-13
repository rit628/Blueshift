#pragma once
#include "libtypes/bls_types.hpp"
#include <unordered_set>
#include <variant>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <utility>
#include <variant>
#include <vector>
#include <string>
#include <optional>

namespace BlsLang {

    class Visitor;
    using BlsObject = std::variant<BlsType, std::reference_wrapper<BlsType>>;

    class AstNode {
        public:
            class Initializer;
            class Specifier;
            class Expression;
            class Statement;
            class Function;
            class Setup;
            class Source;
            
            virtual BlsObject accept(Visitor& v) = 0;
            virtual ~AstNode() = default;

            friend std::ostream& operator<<(std::ostream& os, const AstNode& node);
    };

    class AstNode::Initializer : public AstNode {
        public:
            class Oblock;

            virtual ~Initializer() = default;
    };

    class AstNode::Initializer::Oblock : public AstNode::Initializer {
        public:

            Oblock() = default;
            Oblock(std::string option, std::vector<std::unique_ptr<AstNode::Expression>> args)
                 : option(std::move(option)), args(std::move(args)) {}
            Oblock(std::string option, std::initializer_list<AstNode::Expression*> args)
                 : option(std::move(option))
            {
                for (auto&& arg : args) {
                    this->args.push_back(std::unique_ptr<AstNode::Expression>(arg));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getOption() { return option; }
            auto& getArgs() { return args; }

        private:
            std::string option;
            std::vector<std::unique_ptr<AstNode::Expression>> args;
    };

    class AstNode::Specifier : public AstNode {
        public:
            class Type;
        
            virtual ~Specifier() = default;
    };

    class AstNode::Specifier::Type : public AstNode::Specifier {
        public:
            Type() = default;
            Type(std::string name, std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs)
               : name(std::move(name)), typeArgs(std::move(typeArgs)) {}
            Type(std::string name, std::initializer_list<AstNode::Specifier::Type*> typeArgs)
               : name(std::move(name))
            {
                for (auto&& arg : typeArgs) {
                    this->typeArgs.push_back(std::unique_ptr<AstNode::Specifier::Type>(arg));
                }
            }
            
            BlsObject accept(Visitor& v) override;

            auto& getName() { return name; }
            auto& getTypeArgs() { return typeArgs; }
        
        private:
            std::string name;
            std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs;
    };

    class AstNode::Expression : public AstNode {
        public:
            class Literal;
            class List;
            class Set;
            class Map;
            class Access;
            class Function;
            class Method;
            class Group;
            class Unary;
            class Binary;

            virtual ~Expression() = default;
    };

    class AstNode::Expression::Literal : public AstNode::Expression {
        public:
            Literal() = default;
            Literal(int64_t literal)        : literal(std::move(literal)) {}
            Literal(double literal)         : literal(std::move(literal)) {}
            Literal(bool literal)           : literal(std::move(literal)) {}
            Literal(std::string literal)    : literal(std::move(literal)) {}

            BlsObject accept(Visitor& v) override;

            auto& getLiteral() { return literal; }

        private:
            std::variant<int64_t, double, bool, std::string> literal;
    };

    class AstNode::Expression::List : public AstNode::Expression {
        public:
            List() = default;
            List(std::vector<std::unique_ptr<AstNode::Expression>> elements
               , BlsType literal = std::monostate())
               : elements(std::move(elements))
               , literal(literal) {}
            List(std::initializer_list<AstNode::Expression*> elements
               , BlsType literal = std::monostate())
               : literal(literal)
            {
                for (auto&& element : elements) {
                    this->elements.push_back(std::unique_ptr<AstNode::Expression>(element));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getElements() { return elements; }
            auto& getLiteral() { return literal; }

        private:
            std::vector<std::unique_ptr<AstNode::Expression>> elements;
            BlsType literal;
    };

    class AstNode::Expression::Set : public AstNode::Expression {
        public:
            Set() = default;
            Set(std::vector<std::unique_ptr<AstNode::Expression>> elements)
               : elements(std::move(elements)) {}
            Set(std::initializer_list<AstNode::Expression*> elements)
            {
                for (auto&& element : elements) {
                    this->elements.push_back(std::unique_ptr<AstNode::Expression>(element));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getElements() { return elements; }

        private:
            std::vector<std::unique_ptr<AstNode::Expression>> elements;
    };

    class AstNode::Expression::Map : public AstNode::Expression {
        public:
            Map() = default;
            Map(std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements
              , BlsType literal = std::monostate())
              : elements(std::move(elements))
              , literal(literal) {}
            Map(std::initializer_list<std::initializer_list<AstNode::Expression*>> elements
              , BlsType literal = std::monostate())
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

            BlsObject accept(Visitor& v) override;

            auto& getElements() { return elements; }
            auto& getLiteral() { return literal; }

        private:
            std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements;
            BlsType literal;
    };

    class AstNode::Expression::Access : public AstNode::Expression {
        public:
            Access() = default;
            Access(std::string object
                 , uint8_t localIndex = 0)
                 : object(std::move(object))
                 , subscript(std::move(std::nullopt))
                 , member(std::move(std::nullopt))
                 , localIndex(localIndex) {}
            Access(std::string object
                 , std::unique_ptr<AstNode::Expression> subscript
                 , uint8_t localIndex = 0)
                 : object(std::move(object))
                 , subscript(std::move(subscript))
                 , member(std::move(std::nullopt))
                 , localIndex(localIndex) {}
            Access(std::string object
                 , AstNode::Expression* subscript
                 , uint8_t localIndex = 0)
                 : object(std::move(object))
                 , subscript(std::move(subscript))
                 , member(std::move(std::nullopt))
                 , localIndex(localIndex) {}
            Access(std::string object
                 , std::string member
                 , uint8_t localIndex = 0)
                 : object(std::move(object))
                 , subscript(std::move(std::nullopt))
                 , member(std::move(member))
                 , localIndex(localIndex) {}
            
            BlsObject accept(Visitor& v) override;

            auto& getObject() { return object; }
            auto& getSubscript() { return subscript; }
            auto& getMember() { return member; }
            auto& getLocalIndex() { return localIndex; }

        private:
            std::string object;
            std::optional<std::unique_ptr<AstNode::Expression>> subscript;
            std::optional<std::string> member;
            uint8_t localIndex;
    };

    class AstNode::Expression::Function : public AstNode::Expression {
        public:
            Function() = default;
            Function(std::string name
                   , std::vector<std::unique_ptr<AstNode::Expression>> arguments)
                   : name(std::move(name))
                   , arguments(std::move(arguments)) {}
            Function(std::string name
                   , std::initializer_list<AstNode::Expression*> arguments)
                   : name(std::move(name))
            {
                for (auto&& arg : arguments) {
                    this->arguments.push_back(std::unique_ptr<AstNode::Expression>(arg));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getName() { return name; }
            auto& getArguments() { return arguments; }

        private:
            std::string name;
            std::vector<std::unique_ptr<AstNode::Expression>> arguments;
    };

    class AstNode::Expression::Method : public AstNode::Expression {
        public:
            Method() = default;
            Method(std::string object
                 , std::string methodName
                 , std::vector<std::unique_ptr<AstNode::Expression>> arguments
                 , uint8_t localIndex = 0)
                 : object(std::move(object))
                 , methodName(std::move(methodName))
                 , arguments(std::move(arguments))
                 , localIndex(localIndex) {}
            Method(std::string object
                 , std::string methodName
                 , std::initializer_list<AstNode::Expression*> arguments
                 , uint8_t localIndex = 0)
                 : object(std::move(object))
                 , methodName(std::move(methodName))
                 , localIndex(localIndex)
            {
                for (auto&& arg : arguments) {
                    this->arguments.push_back(std::unique_ptr<AstNode::Expression>(arg));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getObject() { return object; }
            auto& getMethodName() { return methodName; }
            auto& getArguments() { return arguments; }
            auto& getLocalIndex() { return localIndex; }

        private:
            std::string object;
            std::string methodName;
            std::vector<std::unique_ptr<AstNode::Expression>> arguments;
            uint8_t localIndex;
    };

    class AstNode::Expression::Group : public AstNode::Expression {
        public:
            Group() = default;
            Group(std::unique_ptr<AstNode::Expression> expression)
                : expression(std::move(expression)) {}
            Group(AstNode::Expression* expression)
                : expression(expression) {}
            
            BlsObject accept(Visitor& v) override;

            auto& getExpression() { return expression; }

        private:
            std::unique_ptr<AstNode::Expression> expression;
    };

    class AstNode::Expression::Unary : public AstNode::Expression {
        public:
            enum class OPERATOR_POSITION : uint8_t {
                PREFIX,
                POSTFIX
            };

            Unary() = default;
            Unary(std::string op
                , std::unique_ptr<AstNode::Expression> expression
                , OPERATOR_POSITION position = OPERATOR_POSITION::PREFIX)
                : op(std::move(op))
                , expression(std::move(expression))
                , position(std::move(position)) {}
            Unary(std::string op
                , AstNode::Expression* expression
                , OPERATOR_POSITION position = OPERATOR_POSITION::PREFIX)
                : op(std::move(op))
                , expression(expression)
                , position(position) {}
        
            BlsObject accept(Visitor& v) override;

            auto& getOp() { return op; }
            auto& getExpression() { return expression; }
            auto getPosition() { return position; }

        private:
            std::string op;
            std::unique_ptr<AstNode::Expression> expression;
            OPERATOR_POSITION position;
    };

    class AstNode::Expression::Binary : public AstNode::Expression {
        public:
            Binary() = default;
            Binary(std::string op
                 , std::unique_ptr<AstNode::Expression> left
                 , std::unique_ptr<AstNode::Expression> right)
                 : op(std::move(op))
                 , left(std::move(left))
                 , right(std::move(right)) {}
            Binary(std::string op
                 , AstNode::Expression* left
                 , AstNode::Expression* right)
                 : op(std::move(op))
                 , left(left)
                 , right(right) {}
            
            BlsObject accept(Visitor& v) override;

            auto& getOp() { return op; }
            auto& getLeft() { return left; }
            auto& getRight() { return right; }

        private:
            std::string op;
            std::unique_ptr<AstNode::Expression> left;
            std::unique_ptr<AstNode::Expression> right;
    };

    class AstNode::Statement : public AstNode {
        public:
            class Expression;
            class Declaration;
            class Continue;
            class Break;
            class Return;
            class While;
            class For;
            class If;

            virtual ~Statement() = default;
    };

    class AstNode::Statement::Expression : public AstNode::Statement {
        public:
            Expression() = default;
            Expression(std::unique_ptr<AstNode::Expression> expression)
                     : expression(std::move(expression)) {}
            Expression(AstNode::Expression* expression)
                     : expression(expression) {}

            BlsObject accept(Visitor& v) override;

            auto& getExpression() { return expression; }

        private:
            std::unique_ptr<AstNode::Expression> expression;
    };

    class AstNode::Statement::Declaration : public AstNode::Statement {
        public:
            Declaration() = default;
            Declaration(std::string name
                      , std::unordered_set<std::string> modifiers
                      , std::unique_ptr<AstNode::Specifier::Type> type
                      , std::optional<std::unique_ptr<AstNode::Expression>> value
                      , uint8_t localIndex = 0)
                      : name(std::move(name))
                      , modifiers(modifiers)
                      , type(std::move(type))
                      , value(std::move(value))
                      , localIndex(localIndex) {}
            Declaration(std::string name
                      , std::unordered_set<std::string> modifiers
                      , AstNode::Specifier::Type* type
                      , std::optional<AstNode::Expression*> value
                      , uint8_t localIndex = 0)
                      : name(std::move(name))
                      , modifiers(modifiers)
                      , type(type)
                      , value(value)
                      , localIndex(localIndex) {}

            BlsObject accept(Visitor& v) override;

            auto& getName() { return name; }
            auto& getModifiers() { return modifiers; }
            auto& getType() { return type; }
            auto& getValue() { return value; }
            auto& getLocalIndex() { return localIndex; }

        private:
            std::string name;
            std::unordered_set<std::string> modifiers;
            std::unique_ptr<AstNode::Specifier::Type> type;
            std::optional<std::unique_ptr<AstNode::Expression>> value;
            uint8_t localIndex;
    };

    class AstNode::Statement::Continue : public AstNode::Statement {
        public:
            Continue() = default;

            BlsObject accept(Visitor& v) override;
    };

    class AstNode::Statement::Break : public AstNode::Statement {
        public:
            Break() = default;

            BlsObject accept(Visitor& v) override;
    };

    class AstNode::Statement::Return : public AstNode::Statement {
        public:
            Return() = default;
            Return(std::optional<std::unique_ptr<AstNode::Expression>> value)
                 : value(std::move(value)) {}
            Return(AstNode::Expression* value = nullptr)
                 : value(value) {}

            BlsObject accept(Visitor& v) override;

            auto& getValue() { return value; }

        private:
            std::optional<std::unique_ptr<AstNode::Expression>> value;
    };

    class AstNode::Statement::While : public AstNode::Statement {
        public:
            enum class LOOP_TYPE : uint8_t {
                WHILE,
                DO
            };

            While() = default;
            While(std::unique_ptr<AstNode::Expression> condition
                , std::vector<std::unique_ptr<AstNode::Statement>> block
                , LOOP_TYPE type = LOOP_TYPE::WHILE)
                : condition(std::move(condition))
                , block(std::move(block))
                , type(type) {}
            While(AstNode::Expression* condition
                , std::initializer_list<AstNode::Statement*> block
                , LOOP_TYPE type = LOOP_TYPE::WHILE)
                : condition(condition)
                , block()
                , type(type)
            {
                for (auto stmt : block){
                    this->block.push_back(std::unique_ptr<AstNode::Statement>(stmt));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getCondition() { return condition; }
            auto& getBlock() { return block; }
            auto& getType() { return type; }

        private:
            std::unique_ptr<AstNode::Expression> condition;
            std::vector<std::unique_ptr<AstNode::Statement>> block;
            LOOP_TYPE type;
    };

    class AstNode::Statement::For : public AstNode::Statement {
        public:
            For() = default;
            For(std::optional<std::unique_ptr<AstNode::Statement>> initStatement
              , std::optional<std::unique_ptr<AstNode::Statement>> condition
              , std::optional<std::unique_ptr<AstNode::Expression>> incrementExpression
              , std::vector<std::unique_ptr<AstNode::Statement>> block)
              : initStatement(std::move(initStatement))
              , condition(std::move(condition))
              , incrementExpression(std::move(incrementExpression))
              , block(std::move(block)) {}
            For(std::optional<AstNode::Statement*> initStatement
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
            
            BlsObject accept(Visitor& v) override;

            auto& getInitStatement() { return initStatement; }
            auto& getCondition() { return condition; }
            auto& getIncrementExpression() { return incrementExpression; }
            auto& getBlock() { return block; }

        private:
            std::optional<std::unique_ptr<AstNode::Statement>> initStatement;
            std::optional<std::unique_ptr<AstNode::Statement>> condition;
            std::optional<std::unique_ptr<AstNode::Expression>> incrementExpression;
            std::vector<std::unique_ptr<AstNode::Statement>> block;
    };

    class AstNode::Statement::If : public AstNode::Statement {
        public:
            If() = default;
            If(std::unique_ptr<AstNode::Expression> condition
             , std::vector<std::unique_ptr<AstNode::Statement>> block
             , std::vector<std::unique_ptr<AstNode::Statement::If>> elseIfStatements
             , std::vector<std::unique_ptr<AstNode::Statement>> elseBlock)
             : condition(std::move(condition))
             , block(std::move(block))
             , elseIfStatements(std::move(elseIfStatements))
             , elseBlock(std::move(elseBlock)) {}
            If(AstNode::Expression* condition
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
            
            BlsObject accept(Visitor& v) override;

            auto& getCondition() { return condition; }
            auto& getBlock() { return block; }
            auto& getElseIfStatements() { return elseIfStatements; }
            auto& getElseBlock() { return elseBlock; }

        private:
            std::unique_ptr<AstNode::Expression> condition;
            std::vector<std::unique_ptr<AstNode::Statement>> block;
            std::vector<std::unique_ptr<AstNode::Statement::If>> elseIfStatements;
            std::vector<std::unique_ptr<AstNode::Statement>> elseBlock;
    };

    class AstNode::Function : public AstNode {
        public:
            class Procedure;
            class Oblock;

            Function() = default;
            Function(std::string name
                   , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                   , std::vector<std::string> parameters
                   , std::vector<std::unique_ptr<AstNode::Statement>> statements)
                   : name(std::move(name))
                   , parameterTypes(std::move(parameterTypes))
                   , parameters(std::move(parameters))
                   , statements(std::move(statements)) {}
            Function(std::string name
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
            virtual ~Function() = default;

            auto& getName() { return name; }
            auto& getParameterTypes() { return parameterTypes; }
            auto& getParameters() { return parameters; }
            auto& getStatements() { return statements; }
        
        private:
            std::string name;
            std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes;
            std::vector<std::string> parameters;
            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    class AstNode::Function::Procedure : public AstNode::Function {
        public:
            Procedure() = default;
            Procedure(std::string name
                    , std::unique_ptr<AstNode::Specifier::Type> returnType
                    , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                    , std::vector<std::string> parameters
                    , std::vector<std::unique_ptr<AstNode::Statement>> statements)
            :
            Function(std::move(name)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements))
                   , returnType(std::move(returnType)) {}
            Procedure(std::string name
                    , AstNode::Specifier::Type* returnType
                    , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                    , std::vector<std::string> parameters
                    , std::initializer_list<AstNode::Statement*> statements)
            :
            Function(std::move(name)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements))
                   , returnType(returnType) {}
            
            BlsObject accept(Visitor& v) override;
            
            auto& getReturnType() { return returnType; }

        private:
            std::unique_ptr<AstNode::Specifier::Type> returnType;

    };

    class AstNode::Function::Oblock : public AstNode::Function {
        public:
            Oblock() = default;
            Oblock(std::string name
                 , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                 , std::vector<std::string> parameters
                 , std::vector<std::unique_ptr<AstNode::Initializer::Oblock>> configOptions
                 , std::vector<std::unique_ptr<AstNode::Statement>> statements)
            : 
            Function(std::move(name)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements))
                   , configOptions(std::move(configOptions)) {}
            Oblock(std::string name
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

            BlsObject accept(Visitor& v) override;

            auto& getConfigOptions() { return configOptions; }

        private:
            std::vector<std::unique_ptr<AstNode::Initializer::Oblock>> configOptions;
    };

    class AstNode::Setup : public AstNode {
        public:
            Setup() = default;
            Setup(std::vector<std::unique_ptr<AstNode::Statement>> statements)
                : statements(std::move(statements)) {}
            Setup(std::initializer_list<AstNode::Statement*> statements)
            {
                for (auto stmt : statements) {
                    this->statements.push_back(std::unique_ptr<AstNode::Statement>(stmt));
                }
            }

            BlsObject accept(Visitor& v) override;

            auto& getStatements() { return statements; }
                    
        private:
            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    class AstNode::Source : public AstNode {
        public:
            Source() = default;
            Source(std::vector<std::unique_ptr<AstNode::Function>> procedures
                 , std::vector<std::unique_ptr<AstNode::Function>> oblocks
                 , std::unique_ptr<AstNode::Setup> setup)
                 : procedures(std::move(procedures))
                 , oblocks(std::move(oblocks))
                 , setup(std::move(setup)) {}
            Source(std::initializer_list<AstNode::Function*> procedures
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

            BlsObject accept(Visitor& v) override;

            auto& getProcedures() { return procedures; }
            auto& getOblocks() { return oblocks; }
            auto& getSetup() { return setup; }
        
        private:
            std::vector<std::unique_ptr<AstNode::Function>> procedures;
            std::vector<std::unique_ptr<AstNode::Function>> oblocks;
            std::unique_ptr<AstNode::Setup> setup;
    };

};