#include "dependency_graph.hpp"
#include "ast.hpp"
#include "binding_parser.hpp"
#include <string>
#include <unordered_set>
#include <variant>
#include <boost/range/combine.hpp>

using namespace BlsLang;

BlsObject DependencyGraph::visit(AstNode::Specifier::Type& ast) {
    // type specifiers create no dependencies
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Literal& ast) {
    // literals create no dependencies
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::List& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Set& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Map& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Access& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Function& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Method& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Group& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Unary& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Expression::Binary& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Expression& ast) {
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

BlsObject DependencyGraph::visit(AstNode::Statement::Continue& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Break& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::Return& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::While& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::For& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Statement::If& ast) {
    return std::monostate();
}

BlsObject DependencyGraph::visit(AstNode::Function::Procedure& ast) {
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
