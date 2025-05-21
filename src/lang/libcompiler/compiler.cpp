#include "compiler.hpp"
#include <cstddef>
#include <fstream>
#include <sstream>
#include <tuple>
#include <boost/range/combine.hpp>

using namespace BlsLang;

void Compiler::compileFile(const std::string& source) {
    std::ifstream file;
    file.open(source);
    if (!file.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    std::stringstream ss;
    ss << file.rdbuf();
    compileSource(ss.str());
}

void Compiler::compileSource(const std::string& source) {
    tokens = lexer.lex(source);
    ast = parser.parse(tokens);
    ast->accept(analyzer);
    ast->accept(generator);
    generator.writeBytecode(outputStream);
    ast->accept(masterInterpreter);
    auto& descriptors = analyzer.getOblockDescriptors();
    auto& masterOblocks = masterInterpreter.getOblocks();
    euInterpreters.assign(descriptors.size(), masterInterpreter);
    for (auto&& [descPair, interpreter] : boost::combine(descriptors, euInterpreters)) {
        auto& [oblockName, descriptor] = descPair;
        if (!masterOblocks.contains(oblockName)) { continue; }
        auto& oblock = masterOblocks.at(oblockName);
        oblocks.emplace(oblockName, [&oblock, &interpreter](std::vector<BlsType> v) { return oblock(interpreter, v); });
        oblockDescriptors.push_back(descriptor);
    }
}