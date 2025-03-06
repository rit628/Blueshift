#include "bls_types.hpp"
#include "fixtures/e2e_test.hpp"
#include "test_macros.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(E2ETest, MasterTrapTests, PrintString) {
        std::string fileName = "print_string.blu";
        TEST_E2E_SOURCE(fileName);
        auto L1 = createDevtype({
            {"state", false}
        });
        std::string expectedStdout = "A string.\n";
        TEST_E2E_OBLOCK("printString", {L1}, {L1}, expectedStdout);
    }

}