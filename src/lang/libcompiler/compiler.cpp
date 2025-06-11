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
    ast->accept(this->depGraph);
    auto tempOblock = this->depGraph.getOblockMap();  
    this->divider.setOblocks(this->depGraph.getOblockMap()); 
    ast->accept(this->divider); 
    ast->accept(masterInterpreter);
    
    auto& omar = this->divider.getContextsDebug(); 
    for(auto& james : omar){
        for(auto& item: james.second.symbolDeps){
            std::cout<<"Level: "<<item.first<<std::endl; 
            for(auto& item: item.second){
                std::cout<<"item: "<<item<<std::endl; 
            }
            std::cout<<"-----------------------"<<std::endl; 
        } 
    }

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