#pragma once
#include "bls_types.hpp"
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
#include <unordered_set> 

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
            virtual std::unique_ptr<AstNode> clone() const = 0;
            virtual ~AstNode() = default;

            friend std::ostream& operator<<(std::ostream& os, const AstNode& node);
    };

    class AstNode::Initializer : public AstNode {
        public:
            class Task;

            virtual ~Initializer() override = default;
            virtual std::unique_ptr<AstNode::Initializer> cloneBase() const = 0;
    };

    class AstNode::Initializer::Task : public AstNode::Initializer {
        public:

            Task() = default;
            Task(std::string option, std::vector<std::unique_ptr<AstNode::Expression>> args);
            Task(std::string option, std::initializer_list<AstNode::Expression*> args);
            Task(const Task& other);
            Task& operator=(const Task& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Initializer> cloneBase() const override;

            auto& getOption() { return option; }
            auto& getArgs() { return args; }

        private:
            std::string option;
            std::vector<std::unique_ptr<AstNode::Expression>> args;
    };

    class AstNode::Specifier : public AstNode {
        public:
            class Type;
        
            virtual ~Specifier() override = default;
            virtual std::unique_ptr<AstNode::Specifier> cloneBase() const = 0;
    };

    class AstNode::Specifier::Type : public AstNode::Specifier {
        public:
            Type() = default;
            Type(std::string name, std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs);
            Type(std::string name, std::initializer_list<AstNode::Specifier::Type*> typeArgs);
            Type(const Type& other);
            Type& operator=(const Type& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Specifier> cloneBase() const override;

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

            virtual ~Expression() override = default;
            virtual std::unique_ptr<AstNode::Expression> cloneBase() const = 0;
    };

    class AstNode::Expression::Literal : public AstNode::Expression {
        public:
            Literal() = default;
            Literal(int64_t literal);
            Literal(double literal);
            Literal(bool literal);
            Literal(std::string literal);
            Literal(const Literal& other);
            Literal& operator=(const Literal& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

            auto& getLiteral() { return literal; }

        private:
            std::variant<int64_t, double, bool, std::string> literal;
    };

    class AstNode::Expression::List : public AstNode::Expression {
        public:
            List() = default;
            List(std::vector<std::unique_ptr<AstNode::Expression>> elements, BlsType literal = std::monostate());
            List(std::initializer_list<AstNode::Expression*> elements, BlsType literal = std::monostate());
            List(const List& other);
            List& operator=(const List& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

            auto& getElements() { return elements; }
            auto& getLiteral() { return literal; }

        private:
            std::vector<std::unique_ptr<AstNode::Expression>> elements;
            BlsType literal;
    };

    class AstNode::Expression::Set : public AstNode::Expression {
        public:
            Set() = default;
            Set(std::vector<std::unique_ptr<AstNode::Expression>> elements);
            Set(std::initializer_list<AstNode::Expression*> elements);
            Set(const Set& other);
            Set& operator=(const Set& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

            auto& getElements() { return elements; }

        private:
            std::vector<std::unique_ptr<AstNode::Expression>> elements;
    };

    class AstNode::Expression::Map : public AstNode::Expression {
        public:
            Map() = default;
            Map(std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements, BlsType literal = std::monostate());
            Map(std::initializer_list<std::initializer_list<AstNode::Expression*>> elements, BlsType literal = std::monostate());
            Map(const Map& other);
            Map& operator=(const Map& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

            auto& getElements() { return elements; }
            auto& getLiteral() { return literal; }

        private:
            std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements;
            BlsType literal;
    };

    class AstNode::Expression::Access : public AstNode::Expression {
        public:
            Access() = default;
            Access(std::string object, uint8_t localIndex = 0);
            Access(std::string object, std::unique_ptr<AstNode::Expression> subscript, uint8_t localIndex = 0);
            Access(std::string object, AstNode::Expression* subscript, uint8_t localIndex = 0);
            Access(std::string object, std::string member, uint8_t localIndex = 0);
            Access(const Access& other);
            Access& operator=(const Access& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

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
            Function(std::string name, std::vector<std::unique_ptr<AstNode::Expression>> arguments);
            Function(std::string name, std::initializer_list<AstNode::Expression*> arguments);
            Function(const Function& other);
            Function& operator=(const Function& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

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
                 , uint8_t localIndex = 0
                 , TYPE objectType = TYPE::NONE);
            Method(std::string object
                 , std::string methodName
                 , std::initializer_list<AstNode::Expression*> arguments
                 , uint8_t localIndex = 0
                 , TYPE objectType = TYPE::NONE);
            Method(const Method& other);
            Method& operator=(const Method& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

            auto& getObject() { return object; }
            auto& getMethodName() { return methodName; }
            auto& getArguments() { return arguments; }
            auto& getLocalIndex() { return localIndex; }
            auto& getObjectType() { return objectType; }

        private:
            std::string object;
            std::string methodName;
            std::vector<std::unique_ptr<AstNode::Expression>> arguments;
            uint8_t localIndex;
            TYPE objectType;
    };

    class AstNode::Expression::Group : public AstNode::Expression {
        public:
            Group() = default;
            Group(std::unique_ptr<AstNode::Expression> expression);
            Group(AstNode::Expression* expression);
            Group(const Group& other);
            Group& operator=(const Group& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

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
                , OPERATOR_POSITION position = OPERATOR_POSITION::PREFIX);
            Unary(std::string op
                , AstNode::Expression* expression
                , OPERATOR_POSITION position = OPERATOR_POSITION::PREFIX);
            Unary(const Unary& other);
            Unary& operator=(const Unary& rhs);
        
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

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
                 , std::unique_ptr<AstNode::Expression> right);
            Binary(std::string op
                 , AstNode::Expression* left
                 , AstNode::Expression* right);
            Binary(const Binary& other);
            Binary& operator=(const Binary& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Expression> cloneBase() const override;

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

         
            Statement() = default; 
            Statement(std::unordered_set<std::string> controllerSplit)
            : controllerSplit(controllerSplit) {}

            virtual ~Statement() override = default;
            virtual std::unique_ptr<AstNode::Statement> cloneBase() const = 0;

            auto& getControllerSplit() { return controllerSplit; }

            private:
                std::unordered_set<std::string> controllerSplit; 
            
    };

    class AstNode::Statement::Expression : public AstNode::Statement {
        public:
            Expression() = default;
            Expression(std::unique_ptr<AstNode::Expression> expression
                     , std::unordered_set<std::string> controllerSplit = {});
            Expression(AstNode::Expression* expression
                     , std::unordered_set<std::string> controllerSplit = {});
            Expression(const Expression& other);
            Expression& operator=(const Expression& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;

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
                      , uint8_t localIndex = 0);
            Declaration(std::string name
                      , std::unordered_set<std::string> modifiers
                      , AstNode::Specifier::Type* type
                      , std::optional<AstNode::Expression*> value
                      , uint8_t localIndex = 0);
            Declaration(const Declaration& other);
            Declaration& operator=(const Declaration& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;

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
            Continue(const Continue& other) = default;
            Continue& operator=(const Continue& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;
    };

    class AstNode::Statement::Break : public AstNode::Statement {
        public:
            Break() = default;
            Break(const Break& other) = default;
            Break& operator=(const Break& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;
    };

    class AstNode::Statement::Return : public AstNode::Statement {
        public:
            Return() = default;
            Return(std::optional<std::unique_ptr<AstNode::Expression>> value);
            Return(AstNode::Expression* value);
            Return(const Return& other);
            Return& operator=(const Return& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;

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
                , LOOP_TYPE type = LOOP_TYPE::WHILE);
            While(AstNode::Expression* condition
                , std::initializer_list<AstNode::Statement*> block
                , LOOP_TYPE type = LOOP_TYPE::WHILE);
            While(const While& other);
            While& operator=(const While& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;

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
              , std::vector<std::unique_ptr<AstNode::Statement>> block);
            For(std::optional<AstNode::Statement*> initStatement
              , std::optional<AstNode::Statement*> condition
              , std::optional<AstNode::Expression*> incrementExpression
              , std::initializer_list<AstNode::Statement*> block);
            For(const For& other);
            For& operator=(const For& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;

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
             , std::vector<std::unique_ptr<AstNode::Statement>> elseBlock);
            If(AstNode::Expression* condition
             , std::initializer_list<AstNode::Statement*> block
             , std::initializer_list<AstNode::Statement::If*> elseIfStatements
             , std::initializer_list<AstNode::Statement*> elseBlock);
            If(const If& other);
            If& operator=(const If& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Statement> cloneBase() const override;

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
            class Task;

            Function() = default;
            Function(std::string name
                   , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                   , std::vector<std::string> parameters
                   , std::vector<std::unique_ptr<AstNode::Statement>> statements);
            Function(std::string name
                   , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                   , std::vector<std::string> parameters
                   , std::initializer_list<AstNode::Statement*> statements);
            virtual ~Function() override = default;
            virtual std::unique_ptr<AstNode::Function> cloneBase() const = 0;

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
                    , std::vector<std::unique_ptr<AstNode::Statement>> statements);
            Procedure(std::string name
                    , AstNode::Specifier::Type* returnType
                    , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                    , std::vector<std::string> parameters
                    , std::initializer_list<AstNode::Statement*> statements);
            Procedure(const Procedure& other);
            Procedure& operator=(const Procedure& rhs);
            
            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Function> cloneBase() const override;
            
            auto& getReturnType() { return returnType; }

        private:
            std::unique_ptr<AstNode::Specifier::Type> returnType;

    };

    class AstNode::Function::Task : public AstNode::Function {
        public:
            Task() = default;
            Task(std::string name
                 , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                 , std::vector<std::string> parameters
                 , std::vector<std::unique_ptr<AstNode::Initializer::Task>> configOptions
                 , std::vector<std::unique_ptr<AstNode::Statement>> statements);
            Task(std::string name
                 , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                 , std::vector<std::string> parameters
                 , std::initializer_list<AstNode::Initializer::Task*> configOptions
                 , std::initializer_list<AstNode::Statement*> statements);
            Task(const Task& other);
            Task& operator=(const Task& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode::Function> cloneBase() const override;

            auto& getConfigOptions() { return configOptions; }

        private:
            std::vector<std::unique_ptr<AstNode::Initializer::Task>> configOptions;
    };

    class AstNode::Setup : public AstNode {
        public:
            Setup() = default;
            Setup(std::vector<std::unique_ptr<AstNode::Statement>> statements);
            Setup(std::initializer_list<AstNode::Statement*> statements);
            Setup(const Setup& other);
            Setup& operator=(const Setup& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            virtual std::unique_ptr<AstNode> cloneBase() const;

            auto& getStatements() { return statements; }
                    
        private:
            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    class AstNode::Source : public AstNode {
        public:
            Source() = default;
            Source(std::vector<std::unique_ptr<AstNode::Function>> procedures
                 , std::vector<std::unique_ptr<AstNode::Function>> tasks
                 , std::unique_ptr<AstNode::Setup> setup);
            Source(std::initializer_list<AstNode::Function*> procedures
                 , std::initializer_list<AstNode::Function*> tasks
                 , AstNode::Setup* setup);
            Source(const Source& other);
            Source& operator=(const Source& rhs);

            BlsObject accept(Visitor& v) override;
            std::unique_ptr<AstNode> clone() const override;
            std::unique_ptr<AstNode> cloneBase() const;

            auto& getProcedures() { return procedures; }
            auto& getTasks() { return tasks; }
            auto& getSetup() { return setup; }
        
        private:
            std::vector<std::unique_ptr<AstNode::Function>> procedures;
            std::vector<std::unique_ptr<AstNode::Function>> tasks;
            std::unique_ptr<AstNode::Setup> setup;
    };

}