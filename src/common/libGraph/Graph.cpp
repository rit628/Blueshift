#include "Graph.hpp"
#include "Serialization.hpp"
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>


using namespace SymbolGraph; 


std::pair<std::string, int> DAG::get_last_ident(std::string &pure_name){
    auto& stk = this->variable_stk;
    if(stk.empty()){
        throw std::runtime_error("Cannot get variable: Stack is empty");
    }
    std::string mod_name = ""; 
    int sym_desc_id; 

    for(auto iter = stk.rbegin(); iter != stk.rend(); iter++){
        if(iter->sym_ref_cnt.contains(pure_name)){
            int last_ref = iter->sym_ref_cnt.at(pure_name); 
            mod_name = pure_name + "%" + std::to_string(last_ref); 
            sym_desc_id = iter->named_descs.at(mod_name); 
            break; 
        }
    }

    if(mod_name != ""){
        return {mod_name, sym_desc_id}; 
    }
    else{
        throw std::runtime_error("Cannot find symbol in scope: " + pure_name); 
    }
}

void DAG::push_symbol_frame(){
    VariableContainer vc; 
    this->variable_stk.push_back(vc);
}

void DAG::pop_symbol_frame(){
    this->variable_stk.pop_back(); 
}

void DAG::add_variable(std::string &pure_name, int new_sym_id, EXPR_TYPE expr){
    if(this->variable_stk.empty()){
        throw std::runtime_error("Cannot add variable: Variable stack is empty!"); 
    }

    auto& var = this->variable_stk.back(); 
    auto& syms = var.sym_ref_cnt; 
    int new_cnt = 0; 
    if(syms.contains(pure_name)){   
        new_cnt = ++syms.at(pure_name); 
    }
    else{
        syms.emplace(pure_name, new_cnt);  
    }
    
    std::string sym_name = pure_name + "%" + std::to_string(new_cnt);
    auto& new_var = this->descriptor_map.at(new_sym_id);
    new_var.set_name(sym_name);
    new_var.expr_type = expr; 
    var.named_descs.emplace(sym_name, new_sym_id);
}

void DAG::add_symbol(SymbolDescriptor &new_item){

    this->descriptor_map.emplace(new_item.id, new_item); 
    if(!this->conditional_dep_stack.empty()){
        auto val = this->conditional_dep_stack.top(); 
        link_edge(val, new_item.id);
    }

    auto& sd = this->descriptor_map.at(new_item.id);

    switch(sd.expr_type){
        case EXPR_TYPE::DECLARATION: {
            add_variable(sd.symbol_name, sd.id, EXPR_TYPE::DECLARATION); 
            break;
        }

        case EXPR_TYPE::DEVICE_DECLARATION: {
            this->declared_devices.emplace(sd.symbol_name, sd); 
            break; 
        }
        case EXPR_TYPE::DEVICE_ACCESS: {
            auto ind = sd.symbol_name.find('.'); 
            
            auto dev_alias = sd.symbol_name.substr(0, ind); 
            auto first_member = sd.symbol_name.substr(ind + 1, sd.symbol_name.length());

            auto device_tc = this->declared_devices.at(dev_alias).types;
            sd.types = device_tc.child_types.at(first_member); 
            break; 
        }
        case EXPR_TYPE::ACCESS: {
            auto [mod_name, idx]  = get_last_ident(sd.symbol_name);
            auto& chosen_symbol = this->descriptor_map.at(idx); 
            
            sd.symbol_name = mod_name; 
            sd.types = chosen_symbol.types;

            link_edge(idx, sd.id);
            break; 
        }
        case EXPR_TYPE::COND_JUNC: 
        case EXPR_TYPE::COND_JUNC_ELIF:
        {
            this->push_conditional_dep(sd.id);
            auto bin_calc = this->symbol_desc.top(); 
            this->symbol_desc.pop(); 
            sd.types.root_type = "bool";  
            sd.types.send_cost = 1; 

            inherit_bcrange(bin_calc, sd.id); 
            link_edge(bin_calc, sd.id);
            break; 
        }
        case SymbolGraph::EXPR_TYPE::COND_JUNC_ELSE:{
            this->push_conditional_dep(sd.id); 
            break; 
        }
        default: 
            
            break; 
    }   

    this->symbol_desc.push(new_item.id); 
}

void DAG::inherit_bcrange(SymbolID_t parent_id, SymbolID_t child_id){
    const auto& content_symbol = this->descriptor_map.at(parent_id); 
    auto& inherit_symbol = this->descriptor_map.at(child_id); 
    inherit_symbol.bytecode_start = content_symbol.bytecode_start;
    inherit_symbol.bytecode_end = content_symbol.bytecode_end; 
}

void DAG::link_edge(SymbolID_t src_id, SymbolID_t dest_id){
    auto& src_sym = this->descriptor_map.at(src_id); 
    auto& dest_sym = this->descriptor_map.at(dest_id); 
    
    dest_sym.symbolic_depend.push_back(src_id); 
    src_sym.symbolic_fwd.push_back(dest_id);
}

void DAG::unlink_edge(SymbolID_t src_id, SymbolID_t dest_id){
    auto& src_sym = this->descriptor_map.at(src_id); 
    auto& dest_sym = this->descriptor_map.at(dest_id); 

    auto& src_vct = src_sym.symbolic_fwd; 
    auto vct_it = find(src_vct.begin(), src_vct.end(), dest_id);

    if(vct_it == src_vct.end()){
        throw std::runtime_error("failed to unlink element"); 
    }
    else{
        src_vct.erase(vct_it); 
    }


    auto& dest_vect = dest_sym.symbolic_depend;
    auto dst_vct_it = find(dest_vect.begin(), dest_vect.end(), src_id);

    if(dst_vct_it == dest_vect.end()){
        throw std::runtime_error("failed to unlink element"); 
    }
    else{
        dest_vect.erase(dst_vct_it); 
    }
}

void DAG::complete_binary_statement(){
    auto& carrier = this->descriptor_map.at(this->symbol_desc.top()); 
    this->symbol_desc.pop(); 
    auto& rhs = this->descriptor_map.at(this->symbol_desc.top());
    this->symbol_desc.pop(); 
    auto& lhs = this->descriptor_map.at(this->symbol_desc.top());
    this->symbol_desc.pop(); 

    SymbolID_t carrier_id = carrier.id; 
    auto rhs_deps = rhs.symbolic_depend;
    carrier.set_type(this->get_binary_type(lhs, rhs, carrier.symbol_name)); 

    // Transfer the arguments if commutative
    if(carrier.expr_type == EXPR_TYPE::BINARY_COM){
        if(rhs.symbol_name == carrier.symbol_name){
            SymbolID_t home_id = rhs.id; 

            for(auto sym : rhs_deps){
                unlink_edge(sym, home_id); 
                link_edge(sym,  carrier_id); 
            }
            this->descriptor_map.erase(home_id);
        }
        else{
            link_edge(rhs.id,  carrier_id); 
        }

        if(lhs.symbol_name == carrier.symbol_name){
            SymbolID_t home_id = lhs.id; 
            auto lhs_deps = lhs.symbolic_depend; 

            for(auto sym : lhs_deps){
                unlink_edge(sym, home_id); 
                link_edge(sym,  carrier_id); 
            }
            this->descriptor_map.erase(home_id);
        }
        else{
            link_edge(lhs.id, carrier_id); 
        }
        
        this->symbol_desc.push(carrier.id); 
    }
    else if(carrier.symbol_name == "="){
        // Assignment value
        auto &root_name = lhs.symbol_name; 
        int find_str = root_name.find('%'); 
        if(find_str != root_name.npos){
            root_name = root_name.substr(0, find_str); 
        }

        if(lhs.expr_type == EXPR_TYPE::ACCESS){
            this->add_variable(root_name, lhs.id, EXPR_TYPE::ASSIGNMENT); 
        }
        else if(lhs.expr_type == EXPR_TYPE::DEVICE_ACCESS){
            this->add_variable(root_name, lhs.id, EXPR_TYPE::DEVICE_WRITE); 
            this->dev_writes.push_back(lhs.id); 
        }

        link_edge(rhs.id, lhs.id); 
        this->descriptor_map.erase(carrier.id); 
    }
    else{
        link_edge(lhs.id, carrier_id); 
        link_edge(rhs.id, carrier_id); 
        this->symbol_desc.push(carrier.id); 
    }
    
}

void DAG::complete_declare_statement(){
    auto dcl = this->symbol_desc.top(); 
    this->symbol_desc.pop(); 

    if(!this->symbol_desc.empty()){
            auto val = this->symbol_desc.top(); 
            this->symbol_desc.pop(); 
            link_edge(val, dcl); 
    }
}

void DAG::complete_access_statement(){
    auto acs_root = this->symbol_desc.top(); 
    this->symbol_desc.pop(); 
    if(!this->symbol_desc.empty()){
        auto subscript_root = this->symbol_desc.top(); 

        auto& subscript_sym = this->descriptor_map.at(subscript_root); 
        if(subscript_sym.expr_type == EXPR_TYPE::SUBSCRIPT){
            // Add Code to derive the sub_type
        
            this->symbol_desc.pop(); 
            auto subscript_content = this->symbol_desc.top(); 
            this->symbol_desc.pop(); 
            link_edge(subscript_content, subscript_root);
            link_edge(subscript_root, acs_root); 
            inherit_bcrange(subscript_content, subscript_root); 
            
            auto& root_type = this->descriptor_map.at(subscript_root);
            auto content_type = this->descriptor_map.at(subscript_content);
            root_type.types = content_type.types;  

            // Capture the ACS root
            auto& acc_sym = this->descriptor_map.at(acs_root); 
            auto type = acc_sym.types; 
            auto mod_name = acc_sym.symbol_name + "%access"; 
            acc_sym.set_name(mod_name);

            if(type.root_type == "map"){
                acc_sym.types = type.child_types.at("value");
            }
            else if(type.root_type == "list"){
                acc_sym.types = type.child_types.at("%access");
            }
            else{
                std::cout<<"WHAT IS THE TYPE TO BE SUBSCRIPTED: "<<acc_sym.types.root_type<<std::endl; 
            }

        }
    }
    this->symbol_desc.push(acs_root); 
}



void DAG::push_conditional_dep(SymbolID_t cond_type){
    this->conditional_dep_stack.push(cond_type); 
}

void DAG::pop_conditional_dep(){
    if(this->conditional_dep_stack.empty()){
        throw std::runtime_error("Attempting to pop from an empty conditional stack"); 
    }
    else{
        this->conditional_dep_stack.pop(); 
    }
}

void DAG::complete_if_statement(){
    this->pop_conditional_dep(); 
}

bool DAG::is_conditional(std::string& op){
    const std::unordered_set<std::string> op_list = {"==", "<", ">", "<=", ">=", "&&", "||"}; 
    for(auto& str : op_list){
        if(str == op){
            return true; 
        }
    }
    return false; 
}

TypeContainer DAG::get_binary_type(SymbolDescriptor &lhs, SymbolDescriptor &rhs, std::string& op){
    std::unordered_set<std::string> arg = {lhs.types.root_type, rhs.types.root_type}; 
    
    if(arg.size() == 1){
        if(this->is_conditional(op)){
            return TypeContainer{ .root_type = "bool" , .child_types = {}, .send_cost = 1}; 
        }
        return lhs.types; 
    }
    else{
        throw std::runtime_error("Data of different types to be implemented: " + lhs.types.root_type + " " + rhs.types.root_type); 
    }
}


/*
    Conversion of the DAG from the first state to the 
    second state and reverse dep
*/


void DAG::finalize_branch(){
    for(auto& [id, symbol] : this->descriptor_map){
        if(symbol.expr_type == EXPR_TYPE::COND_JUNC){
            

        }
    }
}


void DAG::finalize_access(){


}























std::string DAG::DEBUG_print_sym_type(EXPR_TYPE type){
    switch(type){
        case EXPR_TYPE::ACCESS: 
            return "ACCESS"; 
        case EXPR_TYPE::DEVICE_ACCESS: 
            return "DEVICE_ACCESS";
        case EXPR_TYPE::DECLARATION:
            return "DECLARATION"; 
        case EXPR_TYPE::BINARY_COM:
            return "BINARY_COM"; 
        case EXPR_TYPE::BINARY_NO_COM:
            return "BINARY_NO_COM";
        case EXPR_TYPE::DEVICE_WRITE:
            return "DEVICE_WRITE"; 
        case EXPR_TYPE::ASSIGNMENT:
            return "ASSIGNMENT"; 
        case EXPR_TYPE::LITERAL: 
            return "LITERAL"; 
        case EXPR_TYPE::SUBSCRIPT: 
            return "SUBSCRIPT"; 
        case EXPR_TYPE::COND_JUNC:
            return "COND_JUNC"; 
        case EXPR_TYPE::COND_JUNC_ELIF:
            return "COND_JUNC_ELIF"; 
        case EXPR_TYPE::COND_JUNC_ELSE:
            return "COND_JUNC_ELSE"; 
        default: 
            return "UNADDED_EXPR_TYPE"; 
    }
}


void DAG::DEBUG_print_symbol(const SymbolDescriptor& sdesc){
    std::cout<<"Symbol Id: "<<sdesc.id<<std::endl; 
    std::cout<<"Symbol Name: "<<sdesc.symbol_name<<std::endl; 
    std::cout<<"Type: "<<this->DEBUG_print_sym_type(sdesc.expr_type)<<std::endl;
    std::cout<<"Bytecode Range: "<<sdesc.bytecode_start<<" => "<<sdesc.bytecode_end<<std::endl; 
    
    if(!sdesc.symbolic_depend.empty()){
    std::cout<<"Deps: "; 
        for(auto& item : sdesc.symbolic_depend){
            std::cout<<"\t"<<item<<" :  "<<this->descriptor_map.at(item).symbol_name<<std::endl; 
        }
    }


    /*
    if(sdesc.cond_dep != -1){
        std::cout<<"Conditional Dep";
        int cond = sdesc.cond_dep; 
        auto& sym = this->descriptor_map.at(cond); 
        std::cout<<"\t"<<cond<<" : "<<sym.symbol_name<<std::endl; 

    }
    */ 

    this->DEBUG_print_type(sdesc.types); 
    std::cout<<"---------------------------------------------"<<std::endl;
}

void DAG::DEBUG_print_descriptor_map(){
    std::cout<<"--------------------------------------------"<<std::endl; 
    for(auto& [_ ,val] : this->descriptor_map){
            this->DEBUG_print_symbol(val);
    }
}


void DAG::print_type_helper(const TypeContainer &tc, const std::string &name, int tab_cnt){
    for(int i = 0 ; i < tab_cnt ; i++){
        std::cout<<"   "; 
    }
    std::cout<<name<<" : "<<tc.root_type<<" | cost: "<<tc.send_cost<<std::endl;
    tab_cnt++;
    for(auto& [param_name, type] : tc.child_types){
        this->print_type_helper(type, param_name, tab_cnt); 
    }
}

void DAG::DEBUG_print_type(const TypeContainer& tc){
    this->print_type_helper(tc, "ROOT", 0); 
}

void DAG::DEBUG_print_declared_map(){
    
    std::cout<<"DECLARED DEVICES:\n"<<std::endl; 

    for(auto& [device_name, sd] : this->declared_devices){
        std::cout<<"Symbol name: "<<device_name<<std::endl;   
        this->DEBUG_print_type(sd.types);
        std::cout<<std::endl; 
    }
    
    std::cout<<"DECLARED SYMBOLS TO BE ADDED: \n"<<std::endl; 

    /*
        for(auto& [sym_name, cnt] : this->symbol_ref_count){
            auto new_name = sym_name + "%0"; 
            auto index = this->name_to_symid.at(new_name); 
            std::cout<<"Symbol Name: "<<new_name<<std::endl; 
            try{ 
                auto sc = this->descriptor_map.at(index); 
                this->DEBUG_print_type(sc.types); 
                std::cout<<std::endl; 
            }
            catch(std::exception  e){
                std::cout<<"Cannot find declared symbol: "<<sym_name<<" at index "<<index<<std::endl; 
            }
    }
    */ 
}




