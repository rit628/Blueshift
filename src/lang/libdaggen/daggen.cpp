#include "daggen.hpp"
#include "Serialization.hpp"
#include "ast.hpp"
#include "bls_types.hpp"
#include "visitor.hpp"
#include <exception>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

using namespace BlsLang; 
using SymbolDescriptor = SymbolGraph::SymbolDescriptor; 

#define DEVTYPE_BEGIN(name, type) \
    "DEVTYPE_BEGIN("#name", "#type")\n"
#define ATTRIBUTE(name, type...) \
    "   ATTRIBUTE("#name", "#type")\n"
#define DEVTYPE_END \
    "DEVTYPE_END\n\n"
std::string devtype_list = 
#include "DEVTYPES.LIST"
;
std::stringstream file_stream(devtype_list); 
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE 
#undef DEVTYPE_END



/*
    String manipulation functions Place into its own DEVTYPE Reader Class: 
*/


bool starts_with(std::string &space, std::string&& query){
    if(query.length() > space.length()){
        return false; 
    }

    for(int i = 0; i < query.length(); i++){
        if(query.at(i) != space.at(i)){
            return false; 
        }
    }

    return true; 
}

bool char_is_in(char c, std::vector<char>& vect){
    for(char item : vect){
        if(item == c){
            return true;
        }
    }
    return false; 
}

std::string strip_string(std::string string, std::vector<char>&& vect){
    std::string strip_str; 
    for(int i = 0; i < string.length(); i++){
        char inspect = string.at(i); 
        if(!char_is_in(inspect, vect)){
            strip_str += inspect; 
        }
    }
    return strip_str; 
}

std::vector<std::string> get_bracket_indicies(std::string &str){
    std::vector<std::string> type_cont; 
    std::string type_name = ""; 
    for(char c : str){
        switch(c){
            case '<': 
            case ',': 
                type_cont.push_back(type_name); 
                type_name = ""; 
                break; 
            case '>':
                type_cont.push_back(type_name); 
                type_name = ""; 
                goto end;
            default: 
                type_name += c; 
                break; 
        }
    }

    end: 

    return type_cont; 
}

std::string extract_devtype(std::string &dev_begin){
        int open_loc = dev_begin.find("(") + 1; 
        int comma_loc = dev_begin.find(","); 
        auto res = dev_begin.substr(open_loc, comma_loc - open_loc); 
        return res; 
}

std::vector<std::string> split_on_char(std::string &list, char c){
    std::vector<std::string> value; 
    std::string string_push; 
    int index = list.find(","); 
    value.push_back(list.substr(0, index));
    value.push_back(list.substr(index + 1, list.size() - index)); 
    return value;  
}

std::pair<std::string, std::string> extract_attributes(std::string &attr_string){
    int open_loc = attr_string.find('(') + 1; 
    int closed_loc  = attr_string.find(')'); 

    auto sub_str = attr_string.substr(open_loc, closed_loc - open_loc);
    auto val = split_on_char(sub_str, ','); 

    auto pp = std::pair{val.at(0), val.at(1)}; 
    return pp; 
}

std::vector<std::string> extract_top_level_delimit(std::string &parse_str){
    std::stack<char> brk_stk; 
    std::vector<std::string> vect; 
    int last_point = 0; 

    int i = 0; 
    for(; i < parse_str.size(); i++){
        char c = parse_str.at(i); 
        switch(c){
            case('<'):
                brk_stk.push(c);
                break;
            case('>'):
                brk_stk.pop(); 
                break; 
            case(','):
                if(brk_stk.size() == 0){
                    vect.push_back(parse_str.substr(last_point, i));
                    last_point = i + 1;  
                }
                break; 
        }
    }
    vect.push_back(parse_str.substr(last_point, i)); 
    return vect; 
}



/*
    Parsing the devtypes file
*/

SymbolGraph::EXPR_TYPE DagGen::get_bin_expr_type(std::string &bin_op){
    const std::unordered_set<std::string> vol_1 = {"+", "*"};
    
    if(vol_1.contains(bin_op)){
        return SymbolGraph::EXPR_TYPE::BINARY_COM; 
    }
    return SymbolGraph::EXPR_TYPE::BINARY_NO_COM; 
} 



void DagGen::print_type_helper(TypeContainer &tc, const std::string &name, int tab_cnt){
    for(int i = 0 ; i < tab_cnt ; i++){
        std::cout<<"   "; 
    }
    std::cout<<name<<" : "<<tc.root_type<<" | cost: "<<tc.send_cost<<std::endl;
    tab_cnt++;
    for(auto& [param_name, type] : tc.child_types){
        print_type_helper(type, param_name, tab_cnt); 
    }
}

void DagGen::DEBUG_print_type(TypeContainer& tc){
    print_type_helper(tc, "ROOT", 0); 
}


void DagGen::DEBUG_print_type_stk(){
    auto stk_cpy = this->curr_task_ctx.type_stk;
    while(!stk_cpy.empty()){
        auto type = stk_cpy.top();
        stk_cpy.pop(); 
        DEBUG_print_type(type);    
    }
    std::cout<<std::endl; 
}


TypeContainer DagGen::parse_template_type(std::string &type_name){
    TypeContainer tc; 
    
    if(type_name.starts_with("map")){
        tc.root_type = "map"; 
        //find top seperator index: 
        auto sub_str = type_name.substr(4, type_name.size() - 5); 
        auto temp_args = extract_top_level_delimit(sub_str);

        // Get nested types
        auto key_type = produce_type_data(temp_args.at(0)); 
        auto val_type = produce_type_data(temp_args.at(1)); 

        // Get the nested types
        tc.child_types.emplace("key", key_type);
        tc.child_types.emplace("value", val_type); 
    }
    else if(type_name.starts_with("list")){
        tc.root_type = "list";

        auto sub_str = type_name.substr(5, type_name.size() - 6);
        
        auto acc_type = produce_type_data(sub_str);

        tc.child_types.emplace("%access", acc_type); 
    }
    else{
        std::cout<<"Unknown/Custom template type: "<<type_name<<std::endl; 
    }
    return tc; 
}


void DagGen::assign_score(TypeContainer &tc){
    if(tc.root_type == "int"){
        tc.send_cost = 8;    
    }
    else if(tc.root_type == "float"){
        tc.send_cost = 8; 
    }
    else if(tc.root_type == "string"){
        tc.send_cost = 15; 
    }
    else if(tc.root_type == "bool"){
        tc.send_cost = 1;
    }
    else if(tc.root_type.starts_with("list")){
        auto& contained = tc.child_types.at("%access"); 
        assign_score(contained); 
        tc.send_cost = 10 * contained.send_cost; 
    }
    else if(tc.root_type.starts_with("map")){
        auto& key_type = tc.child_types.at("key"); 
        auto& val_type = tc.child_types.at("value"); 

        assign_score(key_type); 
        assign_score(val_type); 
        tc.send_cost = 10 * (key_type.send_cost + val_type.send_cost); 
    }
    else{
        int sum = 0; 
        for(auto& [name, child_node] : tc.child_types){
            assign_score(child_node); 
            sum += child_node.send_cost; 
        }
        tc.send_cost = sum; 
    }   
}


TypeContainer DagGen::produce_type_data(std::string &type_name){
    TypeContainer new_tc; 

    if(type_name.starts_with("list") || type_name.starts_with("map")){
        new_tc = parse_template_type(type_name); 
    }
    else{
        new_tc.root_type = type_name; 
    }

    return new_tc; 


}


void DagGen::load_device_params(){
    std::stringstream file_stream(devtype_list); 
    std::string line; 
    DeviceType tp; 
    while(std::getline(file_stream, line)){
        line = strip_string(line, {'\t', ' '}); 
        if(starts_with(line, "DEVTYPE_BEGIN")){
            auto dev_name = extract_devtype(line); 
            tp.root_type = dev_name; 
        }
        else if(starts_with(line, "ATTRIBUTE")){
            auto [name, type] = extract_attributes(line); 
            TypeContainer tc = produce_type_data(type);
            tp.params.emplace(name, tc);  
        }
        else if(starts_with(line, "DEVTYPE_END")){
            this->device_map.emplace(tp.root_type, tp); 
            tp.root_type = "";
            tp.params = {}; 
        }
    }
}

/*
VISITOR FUNCTIONS 
*/

BlsObject DagGen::visit(AstNode::Source& ast) {
    std::cout<<"Source node"<<std::endl; 
    for(auto& tsk : ast.tasks){
        tsk->accept(*this); 
    }
    return true; 
}

BlsObject DagGen::visit(AstNode::Setup& ast) {
    std::cout<<"Setup Node"<<std::endl; 
   return true; 
}

BlsObject DagGen::visit(AstNode::Function::Task& ast){
    std::cout<<"Task Function Node"<<std::endl; 
    TaskContext ctx; 
    ctx.task_name = ast.name; 
    this->curr_task_ctx = ctx; 

    for(auto& name : ast.parameters){
        this->curr_task_ctx.param_names.insert(name); 
    }
    
    auto& tp_stk = this->curr_task_ctx.type_stk; 


    for(int i = 0; i < ast.parameterTypes.size(); i++){
        auto& param_type = ast.parameterTypes.at(i); 
        param_type->accept(*this); 

        if(tp_stk.size() != 1){
            DEBUG_print_type_stk(); 
            throw std::runtime_error("Function Task - Types Stack has more than one value"); 
        }

        auto& var_name = ast.parameters.at(i); 
        TypeContainer tc = tp_stk.top();
        if(this->device_map.contains(tc.root_type)){
            tc.child_types = this->device_map.at(tc.root_type).params; 
            assign_score(tc); 
            SymbolDescriptor sd(
                counter++, 
                0,
                0,
                SymbolGraph::EXPR_TYPE::DEVICE_DECLARATION
            );
            sd.set_name(var_name); 
            sd.set_type(tc); 
            this->curr_task_ctx.dag.add_symbol(sd); 
        }
        tp_stk.pop(); 
    }
    

    this->curr_task_ctx.dag.push_symbol_frame(); 
    for(auto& stmt : ast.statements){
        stmt->accept(*this);    
    }
    this->curr_task_ctx.dag.pop_symbol_frame(); 

    this->curr_task_ctx.dag.DEBUG_print_descriptor_map(); 
    this->task_to_ctx.emplace(ast.name, ctx); 
 
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Declaration& ast) {
    std::cout<<"Statement Declaration Node"<<std::endl; 
   

    auto& task_ctx = this->curr_task_ctx;

    if(ast.value.has_value()){
        ast.value.value()->accept(*this); 
    }


    ast.type->accept(*this); 
    if(task_ctx.type_stk.size() == 1){
        auto top_type = task_ctx.type_stk.top();  
        task_ctx.type_stk.pop(); 
        assign_score(top_type); 

        auto new_symbol = SymbolDescriptor(
            counter++, 
            ast.bytecodeStart, 
            ast.bytecodeEnd,
            SymbolGraph::EXPR_TYPE::DECLARATION
        ); 
        new_symbol.set_type(top_type); 
        new_symbol.set_name(ast.name); 
        task_ctx.dag.add_symbol(new_symbol); 

        task_ctx.dag.complete_declare_statement();
    }
    else{
        DEBUG_print_type_stk(); 
        int stk_sz = task_ctx.type_stk.size(); 
        throw std::runtime_error("Declaration Stack is not of size one: size: " + std::to_string(stk_sz)); 
    }
   
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Expression &ast){
    std::cout<<"Statement Expression Node"<<std::endl; 

    ast.expression->accept(*this); 
    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Binary &ast){
    std::cout<<"Expression Binary Node"<<std::endl;  
    ast.left->accept(*this);
    ast.right->accept(*this);
    
    SymbolGraph::EXPR_TYPE type = this->get_bin_expr_type(ast.op); 

    SymbolDescriptor sd(
        counter++,
        ast.bytecodeStart, 
        ast.bytecodeEnd, 
        type
    ); 

    sd.set_name(ast.op); 
    this->curr_task_ctx.dag.add_symbol(sd); 
    this->curr_task_ctx.dag.complete_binary_statement(); 
    
    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Function& ast) {
   std::cout<<"Expression Function Node"<<std::endl; 
   

    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Access& ast) {
    std::cout<<"Expression Access Node"<<std::endl; 
    auto& task = this->curr_task_ctx; 
    auto obj_name = ast.object; 
    
    if(task.param_names.contains(obj_name)){

        SymbolDescriptor sd(
            counter++,
            ast.bytecodeStart, 
            ast.bytecodeEnd, 
            SymbolGraph::EXPR_TYPE::DEVICE_ACCESS
        );

        std::string name = ast.object; 
        if(ast.member.has_value()){
            name += "." + ast.member.value(); 
        }   
        
        sd.set_name(name); 
        task.dag.add_symbol(sd); 
    }
    else{

        if(ast.subscript.has_value()){
            ast.subscript->get()->accept(*this); 
            SymbolDescriptor sd{
                counter++, 
                ast.bytecodeStart,
                ast.bytecodeEnd,
                SymbolGraph::EXPR_TYPE::SUBSCRIPT
            };
            auto var = obj_name + "%index"; 
            sd.set_name(var); 
            task.dag.add_symbol(sd); 
        }

        SymbolDescriptor sd(
            counter++, 
            ast.bytecodeStart, 
            ast.bytecodeEnd, 
            SymbolGraph::EXPR_TYPE::ACCESS
        );

        sd.set_name(obj_name); 
        task.dag.add_symbol(sd); 
    }

    task.dag.complete_access_statement(); 

    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Literal& ast) {
    std::cout<<"Expresion Literal Node"<<std::endl; 
    auto& task = this->curr_task_ctx; 
    SymbolDescriptor sd(
        counter++, 
        ast.bytecodeStart,
        ast.bytecodeEnd,
        SymbolGraph::EXPR_TYPE::LITERAL
    ); 

    std::string lit;
    std::string literal_name = "LITERAL"; 
    if(std::holds_alternative<std::string>(ast.literal)){
        lit = "string"; 
    }
    else if(std::holds_alternative<double>(ast.literal)){
        lit = "float";
    }
    else if(std::holds_alternative<bool>(ast.literal)){
        lit = "bool";
    }
    else{
        lit = "int"; 
    }
    
    sd.set_name(literal_name); 
    sd.types.child_types = {};
    sd.types.send_cost = 0;
    sd.types.root_type = lit; 
    task.dag.add_symbol(sd);
    
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::If& ast) {
    std::cout<<"Statement If Node"<<std::endl; 
    auto& task_dag = this->curr_task_ctx.dag; 
    ast.condition->accept(*this); 
    const std::string cond_name = "cond%" + std::to_string(this->counter); 
    SymbolDescriptor sd(
        counter++, 
        ast.bytecodeStart, 
        ast.bytecodeEnd, 
        SymbolGraph::EXPR_TYPE::COND_JUNC
    ); 
    
    sd.set_name(cond_name); 
    task_dag.add_symbol(sd); 
    for(auto& visit : ast.block){
        visit->accept(*this); 
    }

    for(auto& else_if : ast.elseIfStatements){
        const std::string cond_name = "cond%" + std::to_string(this->counter); 
        else_if->condition->accept(*this); 
        SymbolDescriptor sd(
            counter++, 
            ast.bytecodeStart, 
            ast.bytecodeEnd,
            SymbolGraph::EXPR_TYPE::COND_JUNC_ELIF
        ); 
        sd.set_name(cond_name); 
        task_dag.add_symbol(sd); 

        for(auto& stmt : else_if->block){
            stmt->accept(*this); 
        }

        task_dag.complete_if_statement(); 
    }
    
    if(!ast.elseBlock.empty()){
        const std::string cond_name = "cond%" + std::to_string(this->counter); 
        SymbolDescriptor sd(
            counter++,
            0,
            0,
            SymbolGraph::EXPR_TYPE::COND_JUNC_ELSE
        );
        sd.set_name(cond_name); 
        sd.types.send_cost = 1;
        sd.types.root_type = "bool";
        task_dag.add_symbol(sd); 
        for(auto& stmt : ast.elseBlock){
            stmt->accept(*this); 
        }

        task_dag.complete_if_statement(); 
    }

    task_dag.complete_if_statement();   

    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::For& ast) {
    std::cout<<"Statement For Node"<<std::endl; 

  
    return true; 
    
}

BlsObject DagGen::visit(AstNode::Statement::While& ast) {
    std::cout<<"Statement While Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Group& ast) {
    std::cout<<"Expression Group Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Unary& ast){
    std::cout<<"Expression Unary Node"<<std::endl; 
    ast.expression->accept(*this); 


    return true; 
}



BlsObject DagGen::visit(AstNode::Function::Procedure& ast) {
    std::cout<<"Function Procedure Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Statement::Return& ast) {
    std::cout<<"Statement Return Node"<<std::endl; 


 
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Continue& ast) {
    std::cout<<"Statement Continue Node"<<std::endl;  


    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Break& ast) {
std::cout<<"Statement Break Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Method& ast) {
    std::cout<<"Expression Method Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::List& ast) {
    std::cout<<"Expression List Literal Node"<<std::endl; 

    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Set& ast) {
   std::cout<<"Expression Set Literal Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Map& ast) {
    std::cout<<"Expression Map Node"<<std::endl; 


    return true; 
}


// Recursively construct the curr symbol types
BlsObject DagGen::visit(AstNode::Specifier::Type& ast) {
    std::cout<<"Specifier Type Node"<<std::endl; 
    TypeContainer tc; 
    tc.root_type = ast.name; 
    auto& tstk = this->curr_task_ctx.type_stk; 
    tstk.push(tc); 
    for(auto& item : ast.typeArgs){
        item->accept(*this); 
    }

    // Fill for basic types
    if(ast.name == "list"){ 
        try{ 
            auto contained_type = tstk.top(); 
            this->curr_task_ctx.type_stk.pop(); 
            auto& root_type = tstk.top(); 
            root_type.child_types.emplace("%access", contained_type); 
        }
        catch(std::exception e){
            std::cout<<"List Create Failed!"<<std::endl; 
            DEBUG_print_type_stk();  
        }
    }
    else if(ast.name == "map"){
        try{ 
            auto value_type = tstk.top(); 
            tstk.pop();
            auto key_type = tstk.top(); 
            tstk.pop(); 
            auto& root_type = tstk.top(); 
            root_type.child_types.emplace("key", key_type); 
            root_type.child_types.emplace("value", value_type);  
        }
        catch(std::exception e){
            std::cout<<"Map Create Failed!"<<std::endl;
            DEBUG_print_type_stk();  
        }
    }
    
    return true; 
}

BlsObject DagGen::visit(AstNode::Initializer::Task &ast){
    std::cout<<"Specifier Type Node"<<std::endl; 
    

    
    return true; 
}








