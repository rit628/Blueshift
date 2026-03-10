#pragma once
#include "bls_types.hpp"
#include <tuple>
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

    struct AstNode {
        protected:
            template<typename... Args>
            static auto packMembers(Args&... members) { return std::make_tuple(std::ref(members)...); }

            template<typename... Args>
            auto packChildren(Args&... childMembers)
            { return packMembers(lineStart, lineEnd, columnStart, columnEnd, bytecodeStart, bytecodeEnd, childMembers...); }

            template<typename... Args>
            constexpr auto packChildNames(Args&&... childNames) const
            { return std::make_tuple("lineStart", "lineEnd", "columnStart", "columnEnd", "bytecodeStart", "bytecodeEnd", childNames...); }

        public:
            struct Initializer;
            struct Specifier;
            struct Expression;
            struct Statement;
            struct Function;
            struct Setup;
            struct Source;
            
            virtual BlsObject accept(Visitor& v) = 0;
            virtual std::unique_ptr<AstNode> clone() const = 0;
            virtual ~AstNode() = default;

            friend std::ostream& operator<<(std::ostream& os, const AstNode& node);
            
            size_t lineStart = 0, lineEnd = 0, columnStart = 0, columnEnd = 0;
            size_t bytecodeStart = 0, bytecodeEnd = 0;
    };

    struct AstNode::Initializer : public AstNode {
        struct Task;

        virtual ~Initializer() override = default;
        virtual std::unique_ptr<AstNode::Initializer> cloneBase() const = 0;
    };

    struct AstNode::Initializer::Task : public AstNode::Initializer {
        Task() = default;
        Task(std::string option, std::vector<std::unique_ptr<AstNode::Expression>> args);
        Task(std::string option, std::initializer_list<AstNode::Expression*> args);
        Task(const Task& other);
        Task& operator=(const Task& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Initializer> cloneBase() const override;

        auto getChildren() { return packChildren(option, args); }
        constexpr auto getChildNames() { return packChildNames("option", "args"); }

        std::string option;
        std::vector<std::unique_ptr<AstNode::Expression>> args;
    };

    struct AstNode::Specifier : public AstNode {
        struct Type;
    
        virtual ~Specifier() override = default;
        virtual std::unique_ptr<AstNode::Specifier> cloneBase() const = 0;
    };

    struct AstNode::Specifier::Type : public AstNode::Specifier {
        Type() = default;
        Type(std::string name, std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs);
        Type(std::string name, std::initializer_list<AstNode::Specifier::Type*> typeArgs);
        Type(const Type& other);
        Type& operator=(const Type& rhs);
        
        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Specifier> cloneBase() const override;

        auto getChildren() { return packChildren(name, typeArgs); }
        constexpr auto getChildNames() { return packChildNames("name", "typeArgs"); }

        std::string name;
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs;
    };

    struct AstNode::Expression : public AstNode {
        struct Literal;
        struct List;
        struct Set;
        struct Map;
        struct Access;
        struct Function;
        struct Method;
        struct Group;
        struct Unary;
        struct Binary;

        virtual ~Expression() override = default;
        virtual std::unique_ptr<AstNode::Expression> cloneBase() const = 0;
    };

    struct AstNode::Expression::Literal : public AstNode::Expression {
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

        auto getChildren() { return packChildren(literal); }
        constexpr auto getChildNames() { return packChildNames("literal"); }

        std::variant<int64_t, double, bool, std::string> literal;
    };

    struct AstNode::Expression::List : public AstNode::Expression {
        List() = default;
        List(std::vector<std::unique_ptr<AstNode::Expression>> elements, BlsType literal = std::monostate());
        List(std::initializer_list<AstNode::Expression*> elements, BlsType literal = std::monostate());
        List(const List& other);
        List& operator=(const List& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Expression> cloneBase() const override;

        auto getChildren() { return packChildren(elements, literal); }
        constexpr auto getChildNames() const { return packChildNames("elements", "literal"); }

        std::vector<std::unique_ptr<AstNode::Expression>> elements;
        BlsType literal;
    };

    struct AstNode::Expression::Set : public AstNode::Expression {
        Set() = default;
        Set(std::vector<std::unique_ptr<AstNode::Expression>> elements);
        Set(std::initializer_list<AstNode::Expression*> elements);
        Set(const Set& other);
        Set& operator=(const Set& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Expression> cloneBase() const override;

        auto getChildren() { return packChildren(elements); }
        constexpr auto getChildNames() { return packChildNames("elements"); }

        std::vector<std::unique_ptr<AstNode::Expression>> elements;
    };

    struct AstNode::Expression::Map : public AstNode::Expression {
        Map() = default;
        Map(std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements, BlsType literal = std::monostate());
        Map(std::initializer_list<std::initializer_list<AstNode::Expression*>> elements, BlsType literal = std::monostate());
        Map(const Map& other);
        Map& operator=(const Map& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Expression> cloneBase() const override;

        auto getChildren() { return packChildren(elements, literal); }
        constexpr auto getChildNames() { return packChildNames("elements", "literal"); }

        std::vector<std::pair<std::unique_ptr<AstNode::Expression>, std::unique_ptr<AstNode::Expression>>> elements;
        BlsType literal;
    };

    struct AstNode::Expression::Access : public AstNode::Expression {
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

        auto getChildren() { return packChildren(object, subscript, member, localIndex); }
        constexpr auto getChildNames() { return packChildNames("object", "subscript", "member", "localIndex"); }

        std::string object;
        std::optional<std::unique_ptr<AstNode::Expression>> subscript;
        std::optional<std::string> member;
        uint8_t localIndex;
    };

    struct AstNode::Expression::Function : public AstNode::Expression {
        Function() = default;
        Function(std::string name, std::vector<std::unique_ptr<AstNode::Expression>> arguments);
        Function(std::string name, std::initializer_list<AstNode::Expression*> arguments);
        Function(const Function& other);
        Function& operator=(const Function& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Expression> cloneBase() const override;

        auto getChildren() { return packChildren(name, arguments); }
        constexpr auto getChildNames() { return packChildNames("name", "arguments"); }

        std::string name;
        std::vector<std::unique_ptr<AstNode::Expression>> arguments;
    };

    struct AstNode::Expression::Method : public AstNode::Expression {
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

        auto getChildren() { return packChildren(object, methodName, arguments, localIndex, objectType); }
        constexpr auto getChildNames() { return packChildNames("object", "methodName", "arguments", "localIndex", "objectType"); }

        std::string object;
        std::string methodName;
        std::vector<std::unique_ptr<AstNode::Expression>> arguments;
        uint8_t localIndex;
        TYPE objectType;
    };

    struct AstNode::Expression::Group : public AstNode::Expression {
        Group() = default;
        Group(std::unique_ptr<AstNode::Expression> expression);
        Group(AstNode::Expression* expression);
        Group(const Group& other);
        Group& operator=(const Group& rhs);
        
        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Expression> cloneBase() const override;

        auto getChildren() { return packChildren(expression); }
        constexpr auto getChildNames() { return packChildNames("expression"); }

        std::unique_ptr<AstNode::Expression> expression;
    };

    struct AstNode::Expression::Unary : public AstNode::Expression {
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

        auto getChildren() { return packChildren(op, expression, position); }
        constexpr auto getChildNames() { return packChildNames("op", "expression", "position"); }

        std::string op;
        std::unique_ptr<AstNode::Expression> expression;
        OPERATOR_POSITION position;
    };

    struct AstNode::Expression::Binary : public AstNode::Expression {
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

        auto getChildren() { return packChildren(op, left, right); }
        constexpr auto getChildNames() { return packChildNames("op", "left", "right"); }

        std::string op;
        std::unique_ptr<AstNode::Expression> left;
        std::unique_ptr<AstNode::Expression> right;
    };

    struct AstNode::Statement : public AstNode {
        protected:
            template<typename... Args>
            auto packChildren(Args&... childMembers)
            { return AstNode::packChildren(controllerSplit, childMembers...); }

            template<typename... Args>
            constexpr auto packChildNames(Args&&... childNames) const
            { return AstNode::packChildNames("controllerSplit", childNames...); }

        public:
            struct Expression;
            struct Declaration;
            struct Continue;
            struct Break;
            struct Return;
            struct While;
            struct For;
            struct If;
            
            Statement() = default; 
            Statement(std::unordered_set<std::string> controllerSplit) : controllerSplit(controllerSplit) {}

            virtual ~Statement() override = default;
            virtual std::unique_ptr<AstNode::Statement> cloneBase() const = 0;

            std::unordered_set<std::string> controllerSplit; 
    };

    struct AstNode::Statement::Expression : public AstNode::Statement {
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

        auto getChildren() { return packChildren(expression); }
        constexpr auto getChildNames() { return packChildNames("expression"); }

        std::unique_ptr<AstNode::Expression> expression;
    };

    struct AstNode::Statement::Declaration : public AstNode::Statement {
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

        auto getChildren() { return packChildren(name, modifiers, type, value, localIndex); }
        constexpr auto getChildNames() { return packChildNames("name", "modifiers", "type", "value", "localIndex"); }

        std::string name;
        std::unordered_set<std::string> modifiers;
        std::unique_ptr<AstNode::Specifier::Type> type;
        std::optional<std::unique_ptr<AstNode::Expression>> value;
        uint8_t localIndex;
    };

    struct AstNode::Statement::Continue : public AstNode::Statement {
        Continue() = default;
        Continue(const Continue& other) = default;
        Continue& operator=(const Continue& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Statement> cloneBase() const override;
        
        auto getChildren() { return packChildren(); }
        constexpr auto getChildNames() { return packChildNames(); }
    };

    struct AstNode::Statement::Break : public AstNode::Statement {
        Break() = default;
        Break(const Break& other) = default;
        Break& operator=(const Break& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Statement> cloneBase() const override;
        
        auto getChildren() { return packChildren(); }
        constexpr auto getChildNames() { return packChildNames(); }
    };

    struct AstNode::Statement::Return : public AstNode::Statement {
        Return() = default;
        Return(std::optional<std::unique_ptr<AstNode::Expression>> value);
        Return(AstNode::Expression* value);
        Return(const Return& other);
        Return& operator=(const Return& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode::Statement> cloneBase() const override;

        auto getChildren() { return packChildren(value); }
        constexpr auto getChildNames() { return packChildNames("value"); }

        std::optional<std::unique_ptr<AstNode::Expression>> value;
    };

    struct AstNode::Statement::While : public AstNode::Statement {
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

        auto getChildren() { return packChildren(condition, block, type); }
        constexpr auto getChildNames() { return packChildNames("condition", "block", "type"); }

        std::unique_ptr<AstNode::Expression> condition;
        std::vector<std::unique_ptr<AstNode::Statement>> block;
        LOOP_TYPE type;
    };

    struct AstNode::Statement::For : public AstNode::Statement {
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

        auto getChildren() { return packChildren(initStatement, condition, incrementExpression, block); }
        constexpr auto getChildNames() { return packChildNames("initStatement", "condition", "incrementExpression", "block"); }

        std::optional<std::unique_ptr<AstNode::Statement>> initStatement;
        std::optional<std::unique_ptr<AstNode::Statement>> condition;
        std::optional<std::unique_ptr<AstNode::Expression>> incrementExpression;
        std::vector<std::unique_ptr<AstNode::Statement>> block;
    };

    struct AstNode::Statement::If : public AstNode::Statement {
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

        auto getChildren() { return packChildren(condition, block, elseIfStatements, elseBlock); }
        constexpr auto getChildNames() { return packChildNames("condition", "block", "elseIfStatements", "elseBlock"); }

        std::unique_ptr<AstNode::Expression> condition;
        std::vector<std::unique_ptr<AstNode::Statement>> block;
        std::vector<std::unique_ptr<AstNode::Statement::If>> elseIfStatements;
        std::vector<std::unique_ptr<AstNode::Statement>> elseBlock;
    };

    struct AstNode::Function : public AstNode {
        protected:
            template<typename... Args>
            auto packChildren(Args&... childMembers)
            { return AstNode::packChildren(name, parameterTypes, parameters, statements, childMembers...); }
        
            template<typename... Args>
            constexpr auto packChildNames(Args&&... childNames) const
            { return AstNode::packChildNames("name", "parameterTypes", "parameters", "statements", childNames...); }

        public:
            struct Procedure;
            struct Task;

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

            std::string name;
            std::vector<std::unique_ptr<AstNode::Specifier::Type>> parameterTypes;
            std::vector<std::string> parameters;
            std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    struct AstNode::Function::Procedure : public AstNode::Function {
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

        auto getChildren() { return packChildren(returnType); }
        constexpr auto getChildNames() { return packChildNames("returnType"); }
        
        std::unique_ptr<AstNode::Specifier::Type> returnType;
    };

    struct AstNode::Function::Task : public AstNode::Function {
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

        auto getChildren() { return packChildren(configOptions); }
        constexpr auto getChildNames() { return packChildNames("configOptions"); }

        std::vector<std::unique_ptr<AstNode::Initializer::Task>> configOptions;
    };

    struct AstNode::Setup : public AstNode {
        Setup() = default;
        Setup(std::vector<std::unique_ptr<AstNode::Statement>> statements);
        Setup(std::initializer_list<AstNode::Statement*> statements);
        Setup(const Setup& other);
        Setup& operator=(const Setup& rhs);

        BlsObject accept(Visitor& v) override;
        std::unique_ptr<AstNode> clone() const override;
        std::unique_ptr<AstNode> cloneBase() const;

        auto getChildren() { return packChildren(statements); }
        constexpr auto getChildNames() { return packChildNames("statements"); }

        std::vector<std::unique_ptr<AstNode::Statement>> statements;
    };

    struct AstNode::Source : public AstNode {
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

        auto getChildren() { return packChildren(procedures, tasks, setup); }
        constexpr auto getChildNames() { return packChildNames("procedures", "tasks", "setup"); }

        std::vector<std::unique_ptr<AstNode::Function>> procedures;
        std::vector<std::unique_ptr<AstNode::Function>> tasks;
        std::unique_ptr<AstNode::Setup> setup;
    };

}