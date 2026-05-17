#include "compiler.hpp"
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>
#include <boost/range/combine.hpp>
#include <variant>
#include <vector>


using namespace BlsLang;

void Compiler::compileFile(const std::string& source, ostream_t outputStream) {
    std::ifstream file;
    file.open(source);
    if (!file.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    std::stringstream ss;
    ss << file.rdbuf();
    compileSource(ss.str(), outputStream);
}

void Compiler::compileSource(const std::string& source, ostream_t outputStream) {
    tokens = lexer.lex(source);
    ast = parser.parse(tokens);
    ast->accept(analyzer);
    ast->accept(generator);
    ast->accept(bindmap);

    // Get and load the bind map to the generator
    auto item = bindmap.get_map();  
    dag.load_bind_data(item);
    dag.DEBUG_show_map(); 

    ast->accept(dag); 


    if (auto* stream = std::get_if<std::reference_wrapper<std::vector<char>>>(&outputStream)) {
        generator.writeBytecode(*stream);
    }
    else {
        generator.writeBytecode(std::get<std::reference_wrapper<std::ostream>>(outputStream));
    }
    ast->accept(this->depGraph);
    ast->accept(masterInterpreter);
        
    auto& descriptors = analyzer.getBoundTasks();

    auto& masterTasks = masterInterpreter.getTasks();
    euInterpreters.assign(descriptors.size(), masterInterpreter);
    for (auto&& [taskDescriptor, interpreter] : boost::combine(descriptors, euInterpreters)) {
        if (!masterTasks.contains(taskDescriptor.name)) { continue; }
        auto& task = masterTasks.at(taskDescriptor.name);
        tasks.emplace(taskDescriptor.name, [&task, &interpreter](std::vector<BlsType> v) { return task(interpreter, v); });
    }

}