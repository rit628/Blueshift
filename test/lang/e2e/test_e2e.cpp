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

    GROUP_TEST_F(E2ETest, MasterTrapTests, PrintString) {
        std::string fileName = "print_string.blu";
        TEST_E2E_SOURCE(fileName);
        auto T1 = createBlsType(TypeDef::TIMER_TEST());
        std::string expectedStdout = "A string.\n";
        TEST_E2E_OBLOCK("printString", {T1}, {T1}, expectedStdout);
        expectedStdout = "A string. Another string.\n";
        TEST_E2E_OBLOCK("printAppendedString", {T1}, {T1}, expectedStdout);
    }

    GROUP_TEST_F(E2ETest, ExecutionTests, BinaryOperations) {
        std::string fileName = "binary_operations.blu";
        TEST_E2E_SOURCE(fileName);
        std::string expectedStdout = "";
        TEST_E2E_OBLOCK("add", {0, 0.0}, {21, 21.1}, expectedStdout);
        TEST_E2E_OBLOCK("subtract", {0, 0.0}, {-1, -6.28}, expectedStdout);
        TEST_E2E_OBLOCK("multiply", {0, 0.0}, {90, 45.0}, expectedStdout);
        TEST_E2E_OBLOCK("divide", {0, 0.0}, {0, .9}, expectedStdout);
    }

}