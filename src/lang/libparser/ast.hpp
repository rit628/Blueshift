#pragma once
#include <any>
#include <cstddef>
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

    class AstNode {
        public:
            class Specifier;
            class Expression;
            class Statement;
            class Function;
            class Setup;
            class Source;
            
            virtual std::any accept(Visitor& v) = 0;
            virtual ~AstNode() = default;

            friend std::ostream& operator<<(std::ostream& os, const AstNode& node) { node.print(os); return os; }
        
        private:
            virtual void print(std::ostream& os) const = 0;
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
            
            std::any accept(Visitor& v) override;

            auto& getName() { return name; }
            auto& getTypeArgs() { return typeArgs; }
        
        private:
            void print(std::ostream& os) const override;

            std::string name;
            std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs;
    };

    class AstNode::Expression : public AstNode {
        public:
            class Literal;
            class List;
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
            Literal(size_t literal)         : literal(std::move(literal)) {}
            Literal(double literal)         : literal(std::move(literal)) {}
            Literal(bool literal)           : literal(std::move(literal)) {}
            Literal(std::string literal)    : literal(std::move(literal)) {}

            std::any accept(Visitor& v) override;

            auto& getLiteral() { return literal; }

        private:
            void print(std::ostream& os) const override;

            std::variant<size_t, double, bool, std::string> literal;
    };

    class AstNode::Expression::List : public AstNode::Expression {
        public:
            enum class LIST_TYPE : uint8_t {
                ARRAY,
                SET
            };

            List() = default;
            List(LIST_TYPE type
               , std::vector<std::unique_ptr<AstNode::Expression>> elements)
               : type(type)
               , elements(std::move(elements)) {}
            List(LIST_TYPE type
               , std::initializer_list<AstNode::Expression*> elements)
               : type(type)
               , elements()
            {
                for (auto&& element : elements) {
                    this->elements.push_back(std::unique_ptr<AstNode::Expression>(element));
                }
            }

            std::any accept(Visitor& v) override;

            auto& getType() { return type; }
            auto& getElements() { return elements; }

        private:
            void print(std::ostream& os) const override;

            LIST_TYPE type;
            std::vector<std::unique_ptr<AstNode::Expression>> elements;
    };

    class AstNode::Expression::Access : public AstNode::Expression {
        public:
            Access() = default;
            Access(std::string object)
                 : object(std::move(object))
                 , subscript(std::move(std::nullopt))
                 , member(std::move(std::nullopt)) {}
            Access(std::string object
                 , std::unique_ptr<AstNode::Expression> subscript)
                 : object(std::move(object))
                 , subscript(std::move(subscript))
                 , member(std::move(std::nullopt)) {}
            Access(std::string object
                 , AstNode::Expression* subscript)
                 : object(std::move(object))
                 , subscript(std::move(subscript))
                 , member(std::move(std::nullopt)) {}
            Access(std::string object, std::string member)
                 : object(std::move(object))
                 , subscript(std::move(std::nullopt))
                 , member(std::move(member)) {}
            
            std::any accept(Visitor& v) override;

            auto& getObject() { return object; }
            auto& getSubscript() { return subscript; }
            auto& getMember() { return member; }

        private:
            void print(std::ostream& os) const override;

            std::string object;
            std::optional<std::unique_ptr<AstNode::Expression>> subscript;
            std::optional<std::string> member;
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

            std::any accept(Visitor& v) override;

            auto& getName() { return name; }
            auto& getArguments() { return arguments; }

        private:
            void print(std::ostream& os) const override;

            std::string name;
            std::vector<std::unique_ptr<AstNode::Expression>> arguments;
    };

    class AstNode::Expression::Method : public AstNode::Expression {
        public:
            Method() = default;
            Method(std::string object
                 , std::string methodName
                 , std::vector<std::unique_ptr<AstNode::Expression>> arguments)
                 : object(std::move(object))
                 , methodName(std::move(methodName))
                 , arguments(std::move(arguments)) {}
            Method(std::string object
                 , std::string methodName
                 , std::initializer_list<AstNode::Expression*> arguments)
                 : object(std::move(object))
                 , methodName(std::move(methodName))
            {
                for (auto&& arg : arguments) {
                    this->arguments.push_back(std::unique_ptr<AstNode::Expression>(arg));
                }
            }

            std::any accept(Visitor& v) override;

            auto& getObject() { return object; }
            auto& getMethodName() { return methodName; }
            auto& getArguments() { return arguments; }

        private:
            void print(std::ostream& os) const override;

            std::string object;
            std::string methodName;
            std::vector<std::unique_ptr<AstNode::Expression>> arguments;
    };

    class AstNode::Expression::Group : public AstNode::Expression {
        public:
            Group() = default;
            Group(std::unique_ptr<AstNode::Expression> expression)
                : expression(std::move(expression)) {}
            Group(AstNode::Expression* expression)
                : expression(expression) {}
            
            std::any accept(Visitor& v) override;

            auto& getExpression() { return expression; }

        private:
            void print(std::ostream& os) const override;

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
        
            std::any accept(Visitor& v) override;

            auto& getOp() { return op; }
            auto& getExpression() { return expression; }
            auto getPosition() { return position; }

        private:
            void print(std::ostream& os) const override;

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
            
            std::any accept(Visitor& v) override;

            auto& getOp() { return op; }
            auto& getLeft() { return left; }
            auto& getRight() { return right; }

        private:
            void print(std::ostream& os) const override;

            std::string op;
            std::unique_ptr<AstNode::Expression> left;
            std::unique_ptr<AstNode::Expression> right;
    };

    class AstNode::Statement : public AstNode {
        public:
            class Expression;
            class Assignment;
            class Declaration;
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

            std::any accept(Visitor& v) override;

            auto& getExpression() { return expression; }

        private:
            void print(std::ostream& os) const override;

            std::unique_ptr<AstNode::Expression> expression;
    };

    class AstNode::Statement::Assignment : public AstNode::Statement {
        public:
            Assignment() = default;
            Assignment(std::unique_ptr<AstNode::Expression> recipient
                     , std::unique_ptr<AstNode::Expression> value)
                     : recipient(std::move(recipient))
                     , value(std::move(value)) {}
            Assignment(AstNode::Expression* recipient
                     , AstNode::Expression* value)
                     : recipient(recipient)
                     , value(value) {}

            std::any accept(Visitor& v) override;

            auto& getRecipient() { return recipient; }
            auto& getValue() { return value; }

        private:
            void print(std::ostream& os) const override;

            std::unique_ptr<AstNode::Expression> recipient;
            std::unique_ptr<AstNode::Expression> value;
    };

    class AstNode::Statement::Declaration : public AstNode::Statement {
        public:
            Declaration() = default;
            Declaration(std::string name
                      , std::unique_ptr<AstNode::Specifier::Type> type
                      , std::optional<std::unique_ptr<AstNode::Expression>> value)
                      : name(std::move(name))
                      , type(std::move(type))
                      , value(std::move(value)) {}
            Declaration(std::string name
                      , AstNode::Specifier::Type* type
                      , AstNode::Expression* value)
                      : name(std::move(name))
                      , type(type)
                      , value(value) {}

            std::any accept(Visitor& v) override;

            auto& getName() { return name; }
            auto& getType() { return type; }
            auto& getValue() { return value; }

        private:
            void print(std::ostream& os) const override;

            std::string name;
            std::unique_ptr<AstNode::Specifier::Type> type;
            std::optional<std::unique_ptr<AstNode::Expression>> value;
    };

    class AstNode::Statement::Return : public AstNode::Statement {
        public:
            Return() = default;
            Return(std::optional<std::unique_ptr<AstNode::Expression>> value)
                 : value(std::move(value)) {}
            Return(AstNode::Expression* value = nullptr)
                 : value(value) {}

            std::any accept(Visitor& v) override;

            auto& getValue() { return value; }

        private:
            void print(std::ostream& os) const override;

            std::optional<std::unique_ptr<AstNode::Expression>> value;
    };

    class AstNode::Statement::While : public AstNode::Statement {
        public:
            While() = default;
            While(std::unique_ptr<AstNode::Expression> condition
                , std::vector<std::unique_ptr<AstNode::Statement>> block)
                : condition(std::move(condition))
                , block(std::move(block)) {}
            While(AstNode::Expression* condition
                , std::initializer_list<AstNode::Statement*> block)
                : condition(condition)
            {
                for (auto stmt : block){
                    this->block.push_back(std::unique_ptr<AstNode::Statement>(stmt));
                }
            }

            std::any accept(Visitor& v) override;

            auto& getCondition() { return condition; }
            auto& getBlock() { return block; }

        private:
            void print(std::ostream& os) const override;

            std::unique_ptr<AstNode::Expression> condition;
            std::vector<std::unique_ptr<AstNode::Statement>> block;
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
            
            std::any accept(Visitor& v) override;

            auto& getInitStatement() { return initStatement; }
            auto& getCondition() { return condition; }
            auto& getIncrementExpression() { return incrementExpression; }
            auto& getBlock() { return block; }

        private:
            void print(std::ostream& os) const override;

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
            
            std::any accept(Visitor& v) override;

            auto& getCondition() { return condition; }
            auto& getBlock() { return block; }
            auto& getElseIfStatements() { return elseIfStatements; }
            auto& getElseBlock() { return elseBlock; }

        private:
            void print(std::ostream& os) const override;

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
                   , std::optional<std::unique_ptr<AstNode::Specifier::Type>> returnType
                   , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                   , std::vector<std::string> parameters
                   , std::vector<std::unique_ptr<AstNode::Statement>> statements)
                   : name(std::move(name))
                   , returnType(std::move(returnType))
                   , parameterTypes(std::move(parameterTypes))
                   , parameters(std::move(parameters))
                   , statements(std::move(statements)) {}
            Function(std::string name
                   , std::optional<AstNode::Specifier::Type*> returnType
                   , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                   , std::vector<std::string> parameters
                   , std::initializer_list<AstNode::Statement*> statements)
                   : name(std::move(name))
                   , returnType(std::move(returnType))
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
            auto& getReturnType() { return returnType; }
            auto& getParameters() { return parameters; }
            auto& getStatements() { return statements; }
        
        private:
            std::string name;
            std::optional<std::unique_ptr<AstNode::Specifier::Type>> returnType;
            std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes;
            std::vector<std::string> parameters;
            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    class AstNode::Function::Procedure : public AstNode::Function {
        public:
            Procedure() = default;
            Procedure(std::string name
                    , std::optional<std::unique_ptr<AstNode::Specifier::Type>> returnType
                    , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                    , std::vector<std::string> parameters
                    , std::vector<std::unique_ptr<AstNode::Statement>> statements)
            :
            Function(std::move(name)
                   , std::move(returnType)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements)) {}
            Procedure(std::string name
                    , AstNode::Specifier::Type* returnType
                    , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                    , std::vector<std::string> parameters
                    , std::initializer_list<AstNode::Statement*> statements)
            : 
            Function(std::move(name)
                   , std::move(returnType)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements)) {}
            
            std::any accept(Visitor& v) override;

        private:
            void print(std::ostream& os) const override;
    };

    class AstNode::Function::Oblock : public AstNode::Function {
        public:
            Oblock() = default;
            Oblock(std::string name
                 , std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes
                 , std::vector<std::string> parameters
                 , std::vector<std::unique_ptr<AstNode::Statement>> statements)
            : 
            Function(std::move(name)
                   , std::move(std::nullopt)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements)) {}
            Oblock(std::string name
                 , std::initializer_list<AstNode::Specifier::Type*> parameterTypes
                 , std::vector<std::string> parameters
                 , std::initializer_list<AstNode::Statement*> statements)
            : 
            Function(std::move(name)
                   , std::move(std::nullopt)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements)) {}

            std::any accept(Visitor& v) override;
        
        private:
            void print(std::ostream& os) const override;
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

            std::any accept(Visitor& v) override;

            auto& getStatements() { return statements; }
                    
        private:
            void print(std::ostream& os) const override;

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

            std::any accept(Visitor& v) override;

            auto& getProcedures() { return procedures; }
            auto& getOblocks() { return oblocks; }
            auto& getSetup() { return setup; }
        
        private:
            void print(std::ostream& os) const override;

            std::vector<std::unique_ptr<AstNode::Function>> procedures;
            std::vector<std::unique_ptr<AstNode::Function>> oblocks;
            std::unique_ptr<AstNode::Setup> setup;
    };

};
