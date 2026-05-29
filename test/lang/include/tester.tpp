#pragma once
#include "tester.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <tuple>
#include <optional>
#include <utility>
#include <variant>
#include <boost/range/combine.hpp>
#include <boost/type_index.hpp>

namespace BlsLang {

    namespace {
        template<typename T>
        concept Optional = requires(T t)
        {
            typename T::value_type;
            std::same_as<T, std::optional<typename T::value_type>>;
            { t.has_value() } -> std::same_as<bool>;
        };
    
        template<class... Ts>
        struct overloads : Ts... { using Ts::operator()...; };
    
        template<typename T1, typename T2, typename T3>
        requires ((std::tuple_size_v<T1> == std::tuple_size_v<T2>)
               && (std::tuple_size_v<T1> == std::tuple_size_v<T3>))
        constexpr auto zip(T1&& t1, T2&& t2, T3&& t3) {
            constexpr auto N = std::tuple_size_v<T1>;
    
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::make_tuple(
                    std::tuple
                    <
                        decltype(std::get<Is>(std::forward<T1>(t1))),
                        decltype(std::get<Is>(std::forward<T2>(t2))),
                        decltype(std::get<Is>(std::forward<T3>(t3)))
                    >
                    {
                        std::get<Is>(std::forward<T1>(t1)),
                        std::get<Is>(std::forward<T2>(t2)),
                        std::get<Is>(std::forward<T3>(t3)),
                    }...
                );
            }(std::make_index_sequence<N>{});
        }
    }

    template<std::derived_from<AstNode> Node>
    inline void Tester::checkedAccept(const char *& name, std::unique_ptr<Node>& node) {
        ASSERT_NE(node, nullptr) << "For member: " << name;
        node->accept(*this);
    }

    inline BlsObject Tester::testChild(const char *& name, auto& value, auto& expectedValue) {
        using T = std::remove_reference_t<decltype(value)>;

        if constexpr (Optional<T>) {
            EXPECT_EQ(value.has_value(), expectedValue.has_value()) << "For member: " << name;
            if (value.has_value()) {
                EXPECT_EQ(value, expectedValue) << "For member: " << name;
            }
        }
        else if constexpr (std::same_as<T, std::variant<int64_t, double, bool, std::string>>) {
            std::visit(overloads {
                [&name]<typename T>(T& a, T& b) { EXPECT_EQ(a, b) << "For member: " << name; },
                [&name](auto&, auto&){ FAIL() << "For member: " << name; }
            }, value, expectedValue);
        }
        else {
            EXPECT_EQ(value, expectedValue) << "For member: " << name;
        }
        return std::monostate();
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Tester::testChild(const char *& name
                                     , std::unique_ptr<Node>& node
                                     , std::unique_ptr<Node>& expectedNode)
    {
        expectedVisits.push(expectedNode.get());
        checkedAccept(name, node);
        expectedVisits.pop();
        return std::monostate();
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Tester::testChild(const char *& name
                                     , std::optional<std::unique_ptr<Node>>& node
                                     , std::optional<std::unique_ptr<Node>>& expectedNode)
    {
        EXPECT_EQ(expectedNode.has_value(), node.has_value()) << "For member: " << name;
        if (expectedNode.has_value()) {
            expectedVisits.push(expectedNode->get());
            checkedAccept(name, node.value());
            expectedVisits.pop();
        }
        return std::monostate();
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Tester::testChild(const char *& name
                                     , std::vector<std::unique_ptr<Node>>& nodes
                                     , std::vector<std::unique_ptr<Node>>& expectedNodes)
    {
        EXPECT_EQ(nodes.size(), expectedNodes.size()) << "For member: " << name;
        for (auto&& [element, expectedElement] : boost::combine(nodes, expectedNodes)) {
            expectedVisits.push(expectedElement.get());
            checkedAccept(name, element);
            expectedVisits.pop();
        }
        return std::monostate();
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Tester::testChild(const char *& name
                                     , std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& nodes
                                     , std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& expectedNodes)
    {
        EXPECT_EQ(nodes.size(), expectedNodes.size()) << "For member: " << name;
        for (auto&& [pair, expectedPair] : boost::combine(nodes, expectedNodes)) {
            expectedVisits.push(expectedPair.first.get());
            checkedAccept(name, pair.first);
            expectedVisits.pop();

            expectedVisits.push(expectedPair.second.get());
            checkedAccept(name, pair.second);
            expectedVisits.pop();
        }
        return std::monostate();
    }
    
    template<typename T>
    BlsObject Tester::testChild(std::tuple<const char *&&, T&, T&>& subject) {
        return testChild(std::get<0>(subject), std::get<1>(subject), std::get<2>(subject));
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Tester::testChildren(Node& node, Node& expectedNode) {
        auto zipped = zip(node.getChildNames(), node.getChildren(), expectedNode.getChildren());
        return std::apply(
            [this](auto&... children) -> BlsObject {
                return (this->testChild(children), ..., std::monostate());
            },
            zipped
        );
    }

    void Tester::compare(std::unique_ptr<AstNode>& ast, std::unique_ptr<AstNode>& expectedAst) {
        if (!(ast && expectedAst)) FAIL() << "At least one of the provided asts are null";
        this->expectedAst = expectedAst.get();
        ast->accept(*this);
        this->expectedAst = nullptr;
        if (testing::Test::HasFailure()) {
            FAIL() << "Expected Tree: \n" << *expectedAst << "\nReceived Tree: \n" << *ast;
        }
    }

    #define AST_NODE(Node, ...) \
    inline BlsObject Tester::visit(Node& ast) { \
        auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top(); \
        BlsObject result = std::monostate(); \
        [&](){ \
            try { \
                if (!toCast) FAIL() << "Expected ast is null"; \
                auto& expected = dynamic_cast<Node&>(*toCast); \
                result = testChildren(ast, expected); \
            } catch (const std::bad_cast& e) { \
                std::string expectedType = boost::typeindex::type_id_runtime(*toCast).pretty_name(); \
                expectedType = expectedType.substr(expectedType.find(':') + 2); \
                FAIL() << "Expected " << expectedType << " received " << #Node; \
            } \
        }(); \
        return result; \
    }
    #include "include/NODE_TYPES.LIST"
    #undef AST_NODE

}