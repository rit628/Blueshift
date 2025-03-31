#pragma once
#include "libtypes/bls_types.hpp"
#include "error_types.hpp"
#include <cstddef>
#include <span>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <concepts>

namespace BlsLang {

    template<typename T>
    concept IntAddressable = std::same_as<T, size_t>;

    template<typename T>
    concept StringAddressable = std::same_as<T, std::string>;

    template<typename T>
    concept StackType = IntAddressable<T> || StringAddressable<T>;

    template<StackType T>
    class CallStack {
        public:
            struct Frame {
                enum class Context : uint8_t {
                    FUNCTION
                  , SETUP
                  , LOOP
                  , CONDITIONAL
                };

                using container_t = std::conditional_t<IntAddressable<T>, std::vector<BlsType>, std::unordered_map<T, BlsType>>;

                Frame(Context context, const std::string& name) requires StringAddressable<T> : context(context), name(name) {}
                Frame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T>;
                
                std::string name;
                Context context;
                size_t returnAddress;
                std::stack<BlsType> operands;
                container_t locals;
            };

            CallStack() = default;

            void pushFrame(Frame::Context context, const std::string& name = "") requires StringAddressable<T>;
            void pushFrame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T>;
            size_t popFrame();
            void pushOperand(BlsType&& operand) requires IntAddressable<T>;
            void pushOperand(BlsType& operand) requires IntAddressable<T>;
            BlsType popOperand() requires IntAddressable<T>;
            void setLocal(T index, BlsType value);
            BlsType& getLocal(T index);
            bool checkContext(Frame::Context context) requires StringAddressable<T>;
            const std::string& getFrameName() requires StringAddressable<T>;

        private:
            using cstack_t = std::conditional_t<std::same_as<T, std::string>, std::vector<Frame>, std::stack<Frame>>;
            cstack_t cs;
    };

    template<StackType T>
    inline CallStack<T>::Frame::Frame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T> : returnAddress(returnAddress) {
        locals.assign(arguments.begin(), arguments.end());
    }

    template<StackType T>
    inline void CallStack<T>::pushFrame(Frame::Context context, const std::string& name) requires StringAddressable<T> {
        auto frame = Frame(context, name);
        if (context != Frame::Context::FUNCTION && context != Frame::Context::SETUP) {
            frame.locals = cs.back().locals; // transfer locals to new context
        }
        cs.push_back(frame);
    }

    template<StackType T>
    inline void CallStack<T>::pushFrame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T> {
        cs.push(Frame(returnAddress, arguments));
    }

    template<StackType T>
    inline size_t CallStack<T>::popFrame() {
        size_t returnAddress;
        if constexpr (std::is_same<T, std::string>()) {
            returnAddress = cs.back().returnAddress;
            cs.pop_back();
        }
        else {
            returnAddress = cs.top().returnAddress;
            cs.pop();
        }
        return returnAddress;
    }

    template<StackType T>
    inline void CallStack<T>::pushOperand(BlsType&& operand) requires IntAddressable<T> {
        cs.top().operands.push(operand);
    }

    template<StackType T>
    inline void CallStack<T>::pushOperand(BlsType& operand) requires IntAddressable<T> {
        cs.top().operands.push(operand);
    }

    template<StackType T>
    inline BlsType CallStack<T>::popOperand() requires IntAddressable<T> {
        auto& operands = cs.top().operands;
        auto operand = operands.top();
        operands.pop();
        return operand;
    }

    template<StackType T>
    inline void CallStack<T>::setLocal(T index, BlsType value) {
        if constexpr (std::is_same<T, std::string>()) {
            cs.back().locals[index] = value;
        }
        else {
            auto& locals = cs.top().locals;
            if (index < locals.size()) {
                locals[index] = value;
            }
            else {
                locals.push_back(value);
            }
        }
    }

    template<StackType T>
    inline BlsType& CallStack<T>::getLocal(T index) {
        if constexpr (std::is_same<T, std::string>()) {
            auto& locals = cs.back().locals;
            if (!locals.contains(index)) {
                throw RuntimeError("local variable: " + index + " does not exist");
            }
            return locals[index];
        }
        else {
            return cs.top().locals[index];
        }
    }

    template<StackType T>
    inline bool CallStack<T>::checkContext(Frame::Context context) requires StringAddressable<T> {
        for (auto it = cs.rbegin(); it != cs.rend(); it++) {
            if (it->context == context) {
                return true;
            }
        }
        return false;
    }

    template<StackType T>
    inline const std::string& CallStack<T>::getFrameName() requires StringAddressable<T> {
        for (auto it = cs.rbegin(); it != cs.rend(); it++) {
            if (!it->name.empty()) {
                return it->name;
            }
        }
        return cs.back().name;
    }

}