#pragma once
#include "bls_types.hpp"
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
    concept StackType = std::same_as<T, size_t>
                     || std::same_as<T, std::string>;

    template<typename T>
    concept IntAddressable = std::same_as<T, size_t>;

    template<StackType T>
    class CallStack {
        public:
            struct Frame {
                enum class Context : uint8_t {
                    FUNCTION
                  , LOOP
                  , CONDITIONAL
                };

                using container_t = std::conditional_t<IntAddressable<T>, std::vector<BlsType>, std::unordered_map<T, BlsType>>;

                Frame(Context context) : context(context) {}
                Frame(size_t returnAddress, std::span<BlsType> arguments, size_t localSize);
                
                Context context = Context::FUNCTION;
                size_t returnAddress;
                std::stack<BlsType> operands;
                container_t locals;
            };

            CallStack() = default;

            void pushFrame(Frame::Context context);
            void pushFrame(size_t returnAddress, std::span<BlsType> arguments, size_t localSize);
            size_t popFrame();
            void pushOperand(BlsType&& operand);
            BlsType popOperand();
            void setLocal(T index, BlsType value);
            BlsType& getLocal(T index);
            Frame::Context getContext();

        private:
            std::stack<Frame> cs;
    };

    template<>
    inline CallStack<size_t>::Frame::Frame(size_t returnAddress, std::span<BlsType> arguments, size_t localSize) : returnAddress(returnAddress) {
        locals.reserve(localSize);
        locals.assign(arguments.begin(), arguments.end());
        locals.resize(localSize);
    }

    template<>
    inline void CallStack<std::string>::pushFrame(Frame::Context context) {
        auto frame = Frame(context);
        if (context != Frame::Context::FUNCTION) {
            frame.locals = cs.top().locals;
        }
        cs.push(frame);
    }

    template<>
    inline void CallStack<size_t>::pushFrame(size_t returnAddress, std::span<BlsType> arguments, size_t localSize) {
        cs.push(Frame(returnAddress, arguments, localSize));
    }

    template<StackType T>
    inline size_t CallStack<T>::popFrame() {
        auto returnAddress = cs.top().returnAddress;
        cs.pop();
        return returnAddress;
    }

    template<StackType T>
    inline void CallStack<T>::pushOperand(BlsType&& operand) {
        cs.top().operands.push(operand);
    }

    template<StackType T>
    inline BlsType CallStack<T>::popOperand() {
        auto& operands = cs.top().operands;
        auto operand = operands.top();
        operands.pop();
        return operand;
    }

    template<StackType T>
    inline void CallStack<T>::setLocal(T index, BlsType value) {
        getLocal(index) = value;
    }

    template<StackType T>
    inline BlsType& CallStack<T>::getLocal(T index) {
        return cs.top().locals[index];
    }

    template<>
    inline CallStack<std::string>::Frame::Context CallStack<std::string>::getContext() {
        return cs.top().context;
    }

}