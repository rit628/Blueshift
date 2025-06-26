#include "libtypes/bls_types.hpp"
#include "fixtures/e2e_test.hpp"
#include "libtypes/typedefs.hpp"
#include "test_macros.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(E2ETest, MasterTrapTests, PrintString) {
        std::string fileName = "print_string.blu";
        TEST_E2E_SOURCE(fileName);
        TypeDef::TIMER_TEST states;
        auto T1 = createBlsType(states);
        std::string expectedStdout = "A string.\n";
        TEST_E2E_OBLOCK("printString", {T1}, {T1}, expectedStdout);
        expectedStdout = "A string. Another string.\n";
        TEST_E2E_OBLOCK("printAppendedString", {T1}, {T1}, expectedStdout);
    }

}