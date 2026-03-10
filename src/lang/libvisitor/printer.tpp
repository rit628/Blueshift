#pragma once
#include "ast.hpp"
#include "printer.hpp"
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

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
    
        template<typename T1, typename T2> requires (std::tuple_size_v<T1> == std::tuple_size_v<T2>)
        constexpr auto zip(T1&& t1, T2&& t2) {
            constexpr auto N = std::tuple_size_v<T1>;
    
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::make_tuple(
                    std::pair
                    <
                        decltype(std::get<Is>(std::forward<T1>(t1))),
                        decltype(std::get<Is>(std::forward<T2>(t2)))
                    >
                    (
                        std::get<Is>(std::forward<T1>(t1)),
                        std::get<Is>(std::forward<T2>(t2))
                    )...
                );
            }(std::make_index_sequence<N>{});
        }
    }
    
    inline BlsObject Printer::printChild(auto& value) {
        using T = std::remove_reference_t<decltype(value)>;
        if constexpr (std::is_enum_v<T>) {
            os << static_cast<std::underlying_type_t<T>>(value);
        }
        else if constexpr (std::ranges::range<T> && !std::same_as<T, std::string>) {
            os << '[';
            for (auto&& element : value) {
                os << element << ", ";
            }
            os << ']';
        }
        else if constexpr (Optional<T>) {
            if (value.has_value()) {
                os << *value;
            }
        }
        else if constexpr (std::same_as<T, std::variant<int64_t, double, bool, std::string>>) {
            std::visit(overloads {
                [](std::monostate) { },
                [this](std::string resolved) { os << '"' << resolved << '"'; },
                [this](bool resolved) { os << std::boolalpha << resolved; },
                [this](auto resolved) { os << resolved; }
            }, value);
        }
        else if constexpr (std::same_as<T, uint8_t>) {
            os << +value;
        }
        else {
            os << value;
        }
        os << std::endl;
        return std::monostate();
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Printer::printChild(std::unique_ptr<Node>& node) {
        os << std::endl;
        return node->accept(*this);
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Printer::printChild(std::optional<std::unique_ptr<Node>>& node) {
        if (node.has_value()) {
            os << std::endl;
            return node->get()->accept(*this);
        }
        else {
            os << "null" << std::endl;
        }
        return std::monostate();
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Printer::printChild(std::vector<std::unique_ptr<Node>>& nodes) {
        if (nodes.empty()) {
            os << "[]" << std::endl;
            return std::monostate();
        }
    
        BlsObject result = std::monostate();
        os << '\n' << indent << '[' << std::endl;
        indent.append(4, ' ');
        for (auto&& node : nodes) {
            result = node->accept(*this);
        }
        indent.resize(indent.size() - 4);
        os << indent << ']' << std::endl;
        return result;
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Printer::printChild(std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& nodes) {
        if (nodes.empty()) {
            os << "[]" << std::endl;
            return std::monostate();
        }
        
        BlsObject result = std::monostate();
        os << '\n' << indent << '[' << std::endl;
        indent.append(4, ' ');
        for (auto&& node : nodes) {
            result = node.first->accept(*this);
            os << indent << ':' << std::endl;
            result = node.second->accept(*this);
        }
        indent.resize(indent.size() - 4);
        os << indent << ']' << std::endl;
        return result;
    }
    
    template<typename T>
    BlsObject Printer::printChild(std::pair<const char *&&, T&>& value) {
        os << indent << value.first << ": ";
        return printChild(value.second);
    }
    
    template<std::derived_from<AstNode> Node>
    inline BlsObject Printer::printChildren(Node& node) {
        auto zipped = zip(node.getChildNames(), node.getChildren());
        return std::apply(
            [this](auto&... children) -> BlsObject {
                return (this->printChild(children), ..., std::monostate());
            },
            zipped
        );
    }

}
