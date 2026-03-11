#pragma once
#include "tester.hpp"
#include <gtest/gtest.h>
#include <tuple>
#include <optional>
#include <utility>
#include <variant>
#include <boost/range/combine.hpp>

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
    inline BlsObject Tester::testChild(const char *& name [[ maybe_unused ]]
                                     , std::unique_ptr<Node>& node
                                     , std::unique_ptr<Node>& expectedNode)
    {
        expectedVisits.push(std::move(expectedNode));
        node->accept(*this);
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
            expectedVisits.push(std::move(*expectedNode));
            node->get()->accept(*this);
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
            expectedVisits.push(std::move(expectedElement));
            element->accept(*this);
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
            expectedVisits.push(std::move(expectedPair.first));
            pair.first->accept(*this);
            expectedVisits.pop();

            expectedVisits.push(std::move(expectedPair.second));
            pair.second->accept(*this);
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

}