#include "bytecode_printer.hpp"

int main() {

    BytecodePrinter bsmReader;
    bsmReader.loadBytecode("./samples/bsm/test.bsm");
    bsmReader.dispatch();
}