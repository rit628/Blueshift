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
    ast->accept(this->depGraph);
    auto tempOblock = this->depGraph.getOblockMap();  
    this->symGraph.setMetadata(this->depGraph.getOblockMap(), analyzer.getOblockDescriptors()); 
    ast->accept(this->symGraph); 
    this->symGraph.annotateControllerDivide(); 



    ast->accept(masterInterpreter);
        
 
    
    auto& descriptors = analyzer.getOblockDescriptors();
    
    auto& masterOblocks = masterInterpreter.getOblocks();
    euInterpreters.assign(descriptors.size(), masterInterpreter);
    for (auto&& [descriptor, interpreter] : boost::combine(descriptors, euInterpreters)) {
        if (!masterOblocks.contains(descriptor.name)) { continue; }
        auto& oblock = masterOblocks.at(descriptor.name);
        oblocks.emplace(descriptor.name, [&oblock, &interpreter](std::vector<BlsType> v) { return oblock(interpreter, v); });
    }

}