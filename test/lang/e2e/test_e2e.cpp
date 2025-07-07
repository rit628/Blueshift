#include "libtype/bls_types.hpp"
#include "fixtures/e2e_test.hpp"
#include "libtype/typedefs.hpp"
#include "test_macros.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(E2ETest, TrapTests, print) {
        std::string fileName = "print.blu";
        TEST_E2E_SOURCE(fileName);
        auto T1 = createBlsType(TypeDef::TIMER_TEST());
        std::string expectedStdout = "A string.\n";
        TEST_E2E_OBLOCK("printString", {T1}, {T1}, expectedStdout);
        expectedStdout = "A string. Another string.\n";
        TEST_E2E_OBLOCK("printAppendedString", {T1}, {T1}, expectedStdout);
        expectedStdout = "arg1 arg2\n";
        TEST_E2E_OBLOCK("printMultiArg", {T1}, {T1}, expectedStdout);
    }

    GROUP_TEST_F(E2ETest, ExecutionTests, BinaryOperations) {
        std::string fileName = "binary_operations.blu";
        TEST_E2E_SOURCE(fileName);
        std::string expectedStdout = "";
        TEST_E2E_OBLOCK("add", {0, 0.0}, {21, 21.1}, expectedStdout);
        TEST_E2E_OBLOCK("subtract", {0, 0.0}, {-1, -6.28}, expectedStdout);
        TEST_E2E_OBLOCK("multiply", {0, 0.0}, {90, 45.0}, expectedStdout);
        TEST_E2E_OBLOCK("divide", {0, 0.0}, {0, .9}, expectedStdout);
        TEST_E2E_OBLOCK("grouped", {0, 0.0}, {-10, 71.0}, expectedStdout);
    }

    GROUP_TEST_F(E2ETest, ExecutionTests, ProcedureCalls) {
        std::string fileName = "procedure_calls.blu";
        TEST_E2E_SOURCE(fileName);
        std::string expectedStdout = "";
        TEST_E2E_OBLOCK("simpleCall", {0}, {8}, expectedStdout);
    }

}