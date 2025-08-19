#pragma once
#include "libtype/bls_types.hpp"
#include "error_types.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
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

                using container_t = std::conditional_t<IntAddressable<T>, std::vector<BlsType>, std::unordered_map<T, std::pair<uint8_t, std::shared_ptr<BlsType>>>>;

                Frame(Context context, const std::string& name) requires StringAddressable<T> : context(context), name(name) {}
                Frame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T>;
                
                Context context;
                std::string name;
                uint8_t localCount = 0;

                size_t returnAddress;
                std::stack<BlsType> operands;
                container_t locals;
            };

            CallStack() = default;
            CallStack(Frame::Context defaultContext) requires StringAddressable<T>;

            void pushFrame(Frame::Context context, const std::string& name = "") requires StringAddressable<T>;
            void pushFrame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T>;
            size_t popFrame();
            void pushOperand(BlsType&& operand) requires IntAddressable<T>;
            void pushOperand(BlsType& operand) requires IntAddressable<T>;
            BlsType popOperand() requires IntAddressable<T>;
            void addLocal(T index, BlsType value);
            BlsType& getLocal(T index);
            Frame::container_t&& extractLocals() requires IntAddressable<T>;
            bool checkContext(Frame::Context context) requires StringAddressable<T>;
            bool checkLocalInFrame(T index) requires StringAddressable<T>;
            const std::string& getFrameName() requires StringAddressable<T>;
            uint8_t getLocalIndex(T index) requires StringAddressable<T>;

        private:
            using cstack_t = std::conditional_t<std::same_as<T, std::string>, std::vector<Frame>, std::stack<Frame>>;
            cstack_t cs;
    };

    template<StackType T>
    inline CallStack<T>::CallStack(Frame::Context defaultContext) requires StringAddressable<T> {
        pushFrame(defaultContext);
    }

    template<StackType T>
    inline CallStack<T>::Frame::Frame(size_t returnAddress, std::span<BlsType> arguments) requires IntAddressable<T> : returnAddress(returnAddress) {
        locals.assign(arguments.begin(), arguments.end());
    }

    template<StackType T>
    inline void CallStack<T>::pushFrame(Frame::Context context, const std::string& name) requires StringAddressable<T> {
        auto frame = Frame(context, name);
        if (context != Frame::Context::FUNCTION && context != Frame::Context::SETUP) {
            frame.localCount = cs.back().localCount; // transfer local count to new context
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
    inline void CallStack<T>::addLocal(T index, BlsType value) {
        if constexpr (std::is_same<T, std::string>()) {
            auto& frame = cs.back();
            frame.locals.try_emplace(index, frame.localCount++, new BlsType(value));
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
            for (auto it = cs.rbegin(); it != cs.rend(); it++) {
                auto& locals = it->locals;
                if (locals.contains(index)) {
                    return *locals[index].second;
                }
                else if (it->context == Frame::Context::FUNCTION) { // exhausted current function frame scope
                    throw SemanticError("local variable: " + index + " does not exist");
                }
            }
            throw SemanticError("local variable: " + index + " does not exist");
        }
        else {
            return cs.top().locals[index];
        }
    }

    template<StackType T>
    CallStack<T>::Frame::container_t&& CallStack<T>::extractLocals() requires IntAddressable<T> {
        return std::move(cs.top().locals);
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
    inline bool CallStack<T>::checkLocalInFrame(T index) requires StringAddressable<T> {
        return cs.back().locals.contains(index);
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

    template<StackType T>
    inline uint8_t CallStack<T>::getLocalIndex(T index) requires StringAddressable<T> {
        for (auto it = cs.rbegin(); it != cs.rend(); it++) {
            auto& locals = it->locals;
            if (locals.contains(index)) {
                return locals[index].first;
            }
            else if (it->context == Frame::Context::FUNCTION) { // exhausted current function frame scope
                throw SemanticError("local variable: " + index + " does not exist");
            }
        }
        throw SemanticError("local variable: " + index + " does not exist");
    }

}