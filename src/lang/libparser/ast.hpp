#pragma once
#include "visitor.hpp"
#include <any>
#include <cstddef>
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

    class AstNode::Expression : public AstNode {
        public:
            class Literal;
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

            std::any accept(Visitor& v) override { return v.visit(*this); }

        private:
            void print(std::ostream& os) const override;

            std::variant<size_t, double, bool, std::string> literal;
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
            Access(std::string object, std::string member)
                 : object(std::move(object))
                 , subscript(std::move(std::nullopt))
                 , member(std::move(member)) {}
            
            std::any accept(Visitor& v) override { return v.visit(*this); }

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

            std::any accept(Visitor& v) override { return v.visit(*this); }

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

            std::any accept(Visitor& v) override { return v.visit(*this); }

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
            
            std::any accept(Visitor& v) override { return v.visit(*this); }

        private:
            void print(std::ostream& os) const override;

            std::unique_ptr<AstNode::Expression> expression;
    };

    class AstNode::Expression::Unary : public AstNode::Expression {
        public:
            Unary() = default;
            Unary(std::string op
                , std::unique_ptr<AstNode::Expression> expression
                , bool prefix = true)
                : op(std::move(op))
                , expression(std::move(expression))
                , prefix(std::move(prefix)) {}
        
            std::any accept(Visitor& v) override { return v.visit(*this); }

        private:
            void print(std::ostream& os) const override;

            std::string op;
            std::unique_ptr<AstNode::Expression> expression;
            bool prefix;
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
            
            std::any accept(Visitor& v) override { return v.visit(*this); }

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

            std::any accept(Visitor& v) override { return v.visit(*this); }

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

            std::any accept(Visitor& v) override { return v.visit(*this); }

        private:
            void print(std::ostream& os) const override;

            std::unique_ptr<AstNode::Expression> recipient;
            std::unique_ptr<AstNode::Expression> value;
    };

    class AstNode::Statement::Declaration : public AstNode::Statement {
        public:
            Declaration() = default;
            Declaration(std::string name
                      , std::vector<std::string> type
                      , std::optional<std::unique_ptr<AstNode::Expression>> value)
                      : name(std::move(name))
                      , type(std::move(type))
                      , value(std::move(value)) {}

            std::any accept(Visitor& v) override { return v.visit(*this); }

        private:
            void print(std::ostream& os) const override;

            std::string name;
            std::vector<std::string> type;
            std::optional<std::unique_ptr<AstNode::Expression>> value;
    };

    class AstNode::Statement::Return : public AstNode::Statement {
        public:
            Return() = default;
            Return(std::optional<std::unique_ptr<AstNode::Expression>> value)
                 : value(std::move(value)) {}

            std::any accept(Visitor& v) override { return v.visit(*this); }

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

            std::any accept(Visitor& v) override { return v.visit(*this); }

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
            
            std::any accept(Visitor& v) override { return v.visit(*this); }

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
            
            std::any accept(Visitor& v) override { return v.visit(*this); }

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
            class Setup;

        private:
            Function() = default;
            Function(std::string name
                   , std::optional<std::vector<std::string>> returnType
                   , std::vector<std::vector<std::string>> parameterTypes
                   , std::vector<std::string> parameters
                   , std::vector<std::unique_ptr<AstNode::Statement>> statements)
                   : name(std::move(name))
                   , returnType(std::move(returnType))
                   , parameterTypes(std::move(parameterTypes))
                   , parameters(std::move(parameters))
                   , statements(std::move(statements)) {}
            virtual ~Function() = default;
   
            std::string name;
            std::optional<std::vector<std::string>> returnType;
            std::vector<std::vector<std::string>> parameterTypes;
            std::vector<std::string> parameters;
            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    class AstNode::Function::Procedure : public AstNode::Function {
        public:
            Procedure() = default;
            Procedure(std::string name
                    , std::optional<std::vector<std::string>> returnType
                    , std::vector<std::vector<std::string>> parameterTypes
                    , std::vector<std::string> parameters
                    , std::vector<std::unique_ptr<AstNode::Statement>> statements)
            :
            Function(std::move(name)
                   , std::move(returnType)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements)) {}
            
            std::any accept(Visitor& v) override { return v.visit(*this); }

        private:
            void print(std::ostream& os) const override;
    };

    class AstNode::Function::Oblock : public AstNode::Function {
        public:
            Oblock() = default;
            Oblock(std::string name
                 , std::vector<std::vector<std::string>> parameterTypes
                 , std::vector<std::string> parameters
                 , std::vector<std::unique_ptr<AstNode::Statement>> statements)
            : 
            Function(std::move(name)
                   , std::move(std::nullopt)
                   , std::move(parameterTypes)
                   , std::move(parameters)
                   , std::move(statements)) {}
            
            std::any accept(Visitor& v) override { return v.visit(*this); }
        
        private:
            void print(std::ostream& os) const override;
    };

    class AstNode::Setup : public AstNode {
        public:
            Setup() = default;
            Setup(std::vector<std::unique_ptr<AstNode::Statement>> statements)
                : statements(std::move(statements)) {}
            
            std::any accept(Visitor& v) override { return v.visit(*this); }
        
        private:
            void print(std::ostream& os) const override;

            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    class AstNode::Source : public AstNode {
        public:
            Source() = default;
            Source(std::vector<std::unique_ptr<AstNode::Function::Procedure>> procedures
                 , std::vector<std::unique_ptr<AstNode::Function::Oblock>> oblocks
                 , std::unique_ptr<AstNode::Setup> setup)
                 : procedures(std::move(procedures))
                 , oblocks(std::move(oblocks))
                 , setup(std::move(setup)) {}

            std::any accept(Visitor& v) override { return v.visit(*this); }
        
        private:
            void print(std::ostream& os) const override;

            std::vector<std::unique_ptr<AstNode::Function::Procedure>> procedures;
            std::vector<std::unique_ptr<AstNode::Function::Oblock>> oblocks;
            std::unique_ptr<AstNode::Setup> setup;
    };

};
