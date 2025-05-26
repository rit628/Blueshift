#include "../libdepgraph/depgraph.hpp"
#include "ast.hpp"
#include <memory>
#include <unordered_map>

// Contains a mapping from each statement to the device is can be split to. 



namespace BlsLang{
    enum class StatementParentType{
        NONE, 
        OBLOCK,
        IF, 
        FOR, 
        WHILE, 
        FUNCTION, 
    }; 


    // contains a statement shared ptr and the devices it can be split to.
    struct StatementData{
        StatementParentType parentType = StatementParentType::NONE; 
        std::shared_ptr<AstNode> statement; 
        std::vector<DeviceID> functionOf; 
        std::vector<StatementData> children; 
    }; 

    // contains the raw oblock tree and the device it can be split to. 
    // The parent tree will be used to create the splits. 
    struct ParentTree{
        std::unordered_map<OblockID, std::vector<StatementData>> OblockTree;
    }; 
    

    class Divider : public Printer{
        private: 
            std::unordered_map<OblockID, OblockContext> oblockCtxMap; 
            StatementData rootStatement; 

        public: 
            BlsObject visit(AstNode::Source& ast) override; 
            BlsObject visit(AstNode::Setup& ast) override; 

            BlsObject visit(AstNode::Statement::Declaration& ast) override; 
            BlsObject visit(AstNode::Expression::Function& ast) override; 
            BlsObject visit(AstNode::Expression::Access& ast) override; 
            BlsObject visit(AstNode::Expression::Literal& ast) override; 
            BlsObject visit(AstNode::Statement::Expression& ast) override; 
            BlsObject visit(AstNode::Function::Oblock &ast) override; 
            BlsObject visit(AstNode::Expression::Binary &ast) override; 
    }; 
} 