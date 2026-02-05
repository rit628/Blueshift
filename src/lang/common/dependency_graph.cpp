#include "dependency_graph.hpp"
#include "ast.hpp"
#include "binding_parser.hpp"
#include <string>
#include <unordered_set>
#include <variant>
#include <boost/range/combine.hpp>

using namespace BlsLang;

BlsObject DependencyGraph::visit(AstNode::Specifier::Type&) {
    // type specifiers create no dependencies
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Literal&) {
    // literals create no dependencies
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::List&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Set&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Map&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Access&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Function&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Method&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Group&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Unary&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Binary&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Expression&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Declaration& ast) {
    auto lhsSymbol = ast.getName() + "%" + std::to_string(static_cast<int>(ast.getLocalIndex()));
    std::unordered_set<std::string> rhsSymbols;
    if (ast.getValue().has_value()) {
        rhsSymbols = symbolicate(*ast.getValue()->get());
    }
    std::unordered_map<std::string, DeviceDescriptor> deviceDependencies;
    for (auto&& symbol : rhsSymbols) {
        auto& rhsDeps = currentSymbolDependencies->at(symbol).deviceDependencies;
        deviceDependencies.merge(rhsDeps);
    }
    currentSymbolDependencies->emplace(lhsSymbol, DependencyData{
        deviceDependencies,
        rhsSymbols,
        {&ast}
    });
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Continue&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Break&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Return&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::While&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::For&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::If&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Function::Procedure&) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Function::Task& ast) {
    auto& taskName = ast.getName();
    currentStatementDependencies = &taskStatementDependencies[taskName];
    currentSymbolDependencies = &taskSymbolDependencies[taskName];
    auto& boundDevices = taskDescriptors.at(taskName).binded_devices;
    uint8_t argc = 0;
    for (auto&& [symbol, device] : boost::combine(ast.getParameters(), boundDevices)) {
        auto symbolName = symbol + "%" + std::to_string(static_cast<int>(argc++));
        currentSymbolDependencies->emplace(symbolName, DependencyData{
            {{device.device_name, device}},
            {symbolName},
            {}
        });
    }
    for (auto&& statement : ast.getStatements()) {
        statement->accept(*this);
    }
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Setup& ast) {
    /* TEMPORARY COPY OF ANALYZER BODY FOR TESTING */
    std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
    for (auto&& statement : ast.getStatements()) {
        if (auto* deviceBinding = dynamic_cast<AstNode::Statement::Declaration*>(statement.get())) {
            auto& name = deviceBinding->getName();
            auto devtypeObj = resolve(deviceBinding->getType()->accept(*this));
            auto devtype = static_cast<TYPE>(getType(devtypeObj));

            auto& value = deviceBinding->getValue();
            auto binding = dynamic_cast<AstNode::Expression::Literal*>(value->get())->getLiteral();
            auto& bindStr = std::get<std::string>(binding);
            auto devDesc = parseDeviceBinding(bindStr);
            devDesc.device_name = name;
            devDesc.type = devtype;
            deviceDescriptors.emplace(name, devDesc);
        }
        else if (auto* statementExpression = dynamic_cast<AstNode::Statement::Expression*>(statement.get())) {
            auto* taskBinding = dynamic_cast<AstNode::Expression::Function*>(statementExpression->getExpression().get());
            if (!taskBinding) {
                throw SemanticError("Statement within setup() must be an task binding expression or device binding declaration");
            }
            auto& name = taskBinding->getName();
            auto& args = taskBinding->getArguments();
            TaskDescriptor desc;
            desc.name = name;
            auto& devices = desc.binded_devices;
            for (auto&& arg : args) {
                auto* expr = dynamic_cast<AstNode::Expression::Access*>(arg.get());
                if (expr == nullptr || expr->getMember().has_value() || expr->getSubscript().has_value()) {
                    throw SemanticError("Invalid task binding in setup()");
                }
                try {
                    devices.push_back(deviceDescriptors.at(expr->getObject()));
                }
                catch (std::out_of_range) {
                    throw RuntimeError(expr->getObject() + " does not refer to a device binding");
                }
            }
            taskDescriptors.emplace(name, desc);
        }
    }
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Source& ast) {
    ast.getSetup()->accept(*this);
    for (auto&& task : ast.getTasks()) {
        task->accept(*this);
    }
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Initializer::Task& ast) {

}