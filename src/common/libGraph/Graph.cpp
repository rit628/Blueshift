#include "Graph.hpp"
#include "Serialization.hpp"
#include <cassert>
#include <cerrno>
#include <deque>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>


using namespace SymbolGraph; 

static std::deque<std::string> split_string(std::string s, char delim){
    std::stringstream ss(s); 
    std::string holder; 
    std::deque<std::string> type_queue; 

    while(std::getline(ss, holder, delim)){
        type_queue.push_back(holder); 
    }

    return type_queue; 
}


TypeContainer DAG::extract_type_help(TypeContainer root_tc, std::deque<std::string> &str_list){
    if(str_list.empty()){
        return root_tc; 
    }
    else{
        auto item = str_list.back(); 
        TypeContainer sub_type; 
        str_list.pop_back(); 
        if(root_tc.child_types.contains(item)){
            sub_type = root_tc.child_types.at(item);
        }   
        else{
            throw std::runtime_error(root_tc.root_type + " has no sub type " + item); 
        }

        return extract_type_help(sub_type, str_list); 
    }
}



TypeContainer DAG::extract_type(TypeContainer root_tc, std::string &subtype_list){
    auto deque = split_string(subtype_list, '.'); 
    return extract_type_help(root_tc, deque); 
}


DAG::VariableContainer& DAG::get_last_assign(const std::string &pure_name){
    auto& stk = this->variable_stk; 
    for(auto iter = stk.rbegin(); iter != stk.rend(); iter++){
        if(iter->sym_ref_cnt.contains(pure_name)){
            return *iter; 
        }
    }
    return stk.back(); 
}

std::pair<std::string, std::string> DAG::get_access_pair(const std::string& obj_ref){
    std::string root_item = obj_ref;
    std::string sub_access; 
    
    if(int i = obj_ref.find("."); i != obj_ref.npos){
        root_item = obj_ref.substr(0,i); 
        sub_access = obj_ref.substr(i+1, obj_ref.size());
    }
    
    return {root_item, sub_access}; 
}



std::pair<std::string, int> DAG::get_last_ident(const std::string &pure_name){
    auto& stk = this->variable_stk;

    if(stk.empty()){
        throw std::runtime_error("Cannot get variable: Stack is empty");
    }

    if(this->declared_devices.contains(pure_name)){
        int val = this->declared_devices.at(pure_name);
        return {pure_name, val}; 
    }

    std::string mod_name = ""; 
    int sym_desc_id = -1; 

    for(auto iter = stk.rbegin(); iter != stk.rend(); iter++){
        if(iter->named_descs.contains(pure_name)){
            sym_desc_id = iter->named_descs.at(pure_name); 
            mod_name = this->descriptor_map.at(sym_desc_id).symbol_name; 
            break; 
        }
    }

    if(sym_desc_id != -1){
        return {mod_name, sym_desc_id}; 
    }
    else{
        throw std::runtime_error("Cannot find symbol in scope: " + pure_name); 
    }
}



void DAG::push_symbol_frame(){
    this->curr_stack_depth++; 
    this->variable_stk.push_back(VariableContainer{});
}

void DAG::pop_symbol_frame(){
    this->curr_stack_depth--; 
    this->variable_stk.pop_back(); 
}

void DAG::start_if_statement(){
    this->affected_variables.push({}); 
}



void DAG::add_variable(std::string pure_name, int new_sym_id, EXPR_TYPE expr){
    if(this->variable_stk.empty()){
        throw std::runtime_error("Cannot add variable: Variable stack is empty!"); 
    }

    auto& var = this->variable_stk.back(); 
    int new_cnt = 0; 

    if(!var.sym_ref_cnt.contains(pure_name)){
        var.sym_ref_cnt.emplace(pure_name, new_cnt);
    }
    else{
        new_cnt = ++var.sym_ref_cnt.at(pure_name); 
    }

    std::string sym_name; 
    if(expr == EXPR_TYPE::PHI){
        sym_name = pure_name + "%phi%" + std::to_string(new_cnt);
    }
    else if(expr == EXPR_TYPE::ITER_PHI){
        sym_name = pure_name + "%iter_phi%" + std::to_string(new_cnt);
    }   
    else{
        sym_name = pure_name + "%" + std::to_string(new_cnt);
    }


    auto& new_var = this->descriptor_map.at(new_sym_id);
    new_var.stack_depth = this->curr_stack_depth;
    new_var.set_root_symbol_name(pure_name); 
    new_var.set_name(sym_name);
    new_var.expr_type = expr; 
    var.named_descs.insert_or_assign(pure_name, new_sym_id); 
}


int DAG::add_symbol(SymbolDescriptor &new_item){

    this->descriptor_map.emplace(new_item.id, new_item); 
    if(!this->conditional_dep_stack.empty()){
        auto val = this->conditional_dep_stack.top(); 
        link_conditional_edge(val, new_item.id);
    }

    auto& sd = this->descriptor_map.at(new_item.id);

    switch(sd.expr_type){
        case EXPR_TYPE::DECLARATION: {
            add_variable(sd.symbol_name, sd.id, EXPR_TYPE::DECLARATION); 
            break;
        }
        case EXPR_TYPE::DEVICE_DECLARATION: {
            this->declared_devices.emplace(sd.symbol_name, sd.id); 
            break; 
        }
        case EXPR_TYPE::ACCESS: {
            sd.set_root_symbol_name(sd.symbol_name); 

            if(sd.id == 39){
                std::cout<<"Right Wing"<<std::endl; 
            }

            auto [mod_name, idx]  = get_last_ident(sd.symbol_name);
            auto& chosen_symbol = this->descriptor_map.at(idx); 
            
            sd.symbol_name = mod_name + "%access";
            sd.types = chosen_symbol.types;

            link_edge(idx, sd.id);
            break; 
        }
        case EXPR_TYPE::COND_JUNC_ELIF:
        case EXPR_TYPE::COND_JUNC: 
        {
            this->push_conditional_dep(sd.id);
            auto bin_calc = this->symbol_desc.top(); 
            this->symbol_desc.pop(); 
            sd.types.root_type = "bool";  
            sd.types.send_cost = 1; 

            inherit_bcrange(bin_calc, sd.id); 
            sd.conditional_symbol = bin_calc; 
            break; 
        }
        case EXPR_TYPE::COND_JUNC_ELSE:{
            this->push_conditional_dep(sd.id); 
            break; 
        }
        case EXPR_TYPE::ASSIGNMENT:{
            auto dep = this->conditional_dep_stack.top(); 
            auto& cond = this->descriptor_map.at(dep); 
            break; 
        }
        case EXPR_TYPE::FOR:{ 
            sd.types.root_type = "none"; 
            sd.types.send_cost = 0; 
            break;     
        }
        default: 
            
            break; 
    }   

    this->symbol_desc.push(new_item.id); 
    return new_item.id; 
}

void DAG::inherit_bcrange(SymbolID_t parent_id, SymbolID_t child_id){
    const auto& content_symbol = this->descriptor_map.at(parent_id); 
    auto& inherit_symbol = this->descriptor_map.at(child_id); 
    inherit_symbol.bytecode_start = content_symbol.bytecode_start;
    inherit_symbol.bytecode_end = content_symbol.bytecode_end; 
}

void DAG::link_conditional_edge(SymbolID_t parent, SymbolID_t child){
    auto parent_cond = this->descriptor_map.at(parent); 
    parent_cond.true_path.push_back(child); 
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
    carrier.binary_val.lhs_id = lhs.id; 
    carrier.binary_val.rhs_id = rhs.id; 

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
        const auto &root_name = lhs.root_symbol_name; 

        if(!this->conditional_dep_stack.empty()){
            auto& direct_dep_symbol = this->descriptor_map.at(this->conditional_dep_stack.top()); 
            direct_dep_symbol.value_under_cond.emplace(root_name, rhs.id); 
            this->affected_variables.top().push_back(root_name); 
        }
      
        if(lhs.expr_type == EXPR_TYPE::ACCESS){
            this->add_variable(root_name, lhs.id, EXPR_TYPE::ASSIGNMENT); 
        }
        else if(lhs.expr_type == EXPR_TYPE::DEVICE_ACCESS){
            this->add_variable(root_name, lhs.id, EXPR_TYPE::DEVICE_WRITE); 
            this->dev_writes.push_back(lhs.id); 
        }
        
        lhs.value_node = rhs.id; 
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
        this->descriptor_map.at(dcl).value_node = val; 
        link_edge(val, dcl); 
    }

    this->symbol_desc.push(dcl); 
}

void DAG::complete_member_statement(){
    auto member_item = this->symbol_desc.top(); 
    this->symbol_desc.pop(); 
    auto root_access = this->symbol_desc.top(); 
    this->symbol_desc.pop(); 

    auto& root_symbol = this->descriptor_map.at(root_access);
    auto& member_symbol = this->descriptor_map.at(member_item); 
    if(member_symbol.expr_type != EXPR_TYPE::MEMBER){
        throw std::runtime_error("Unexpected expression type for member statement"); 
    }

    auto sub_types = root_symbol.types.child_types.at(member_symbol.symbol_name); 

    EXPR_TYPE type = EXPR_TYPE::ACCESS; 
    if(this->declared_devices.contains(root_symbol.root_symbol_name)){
        type = EXPR_TYPE::DEVICE_ACCESS; 
    }

    SymbolDescriptor sd{
        member_item, 
        member_symbol.bytecode_start,
        member_symbol.bytecode_end, 
        type, 
    }; 

    auto combined_name = root_symbol.root_symbol_name + "." + member_symbol.root_symbol_name; 
    sd.set_name(combined_name); 
    sd.set_root_symbol_name(combined_name); 
    sd.set_type(sub_types); 
 
    this->descriptor_map.erase(member_item); 
    this->descriptor_map.erase(root_access);
    
    this->descriptor_map.emplace(sd.id, sd); 
    this->symbol_desc.push(sd.id); 
}

void DAG::complete_subscript_statement(){
    auto& subscript_item = this->descriptor_map.at(this->symbol_desc.top()); 
    this->symbol_desc.pop();
    auto& binary_item = this->descriptor_map.at(this->symbol_desc.top()); 
    this->symbol_desc.pop(); 
    auto& root_access = this->descriptor_map.at(this->symbol_desc.top());
    this->symbol_desc.pop(); 

    subscript_item.subscript_val.index_id = binary_item.id; 
    subscript_item.subscript_val.root_id = root_access.id; 

    link_edge(binary_item.id, subscript_item.id); 
    link_edge(root_access.id, subscript_item.id); 



    auto new_name = root_access.symbol_name + "%index"; 
    subscript_item.set_name(new_name);     

    // Check if the root type is subscriptable ()
    TypeContainer sub_type; 
    if(root_access.types.root_type == "map"){
        sub_type = root_access.types.child_types.at("value"); 
    }
    else if(root_access.types.root_type == "list"){
        sub_type = root_access.types.child_types.at("%access"); 
    }
   
    subscript_item.set_type(sub_type);
    this->symbol_desc.push(subscript_item.id); 
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

int DAG::construct_phi_node(const std::string& pure_name, const CondValue& cv, SymbolID_t alt_value, size_t bcode_start, size_t bcode_end, SymbolID_t &counter){
    SymbolID_t produced = counter++; 
    SymbolDescriptor sd{
        produced, 
        bcode_start,
        bcode_end,
        EXPR_TYPE::PHI
    };

    
    std::string root_name = pure_name;
    std::string object_names = ""; 
    if(auto pos = pure_name.find(".") ; pos != pure_name.npos){
        root_name = pure_name.substr(0, pos);
        object_names = pure_name.substr(pos + 1, pure_name.size()); 
    }  

    auto [_, val_int] = this->get_last_ident(root_name); 
    auto& root_obj = this->descriptor_map.at(val_int); 

    sd.set_root_symbol_name(pure_name); 
    sd.set_phi_params(cv.condition, cv.value, alt_value); 
    
    TypeContainer tc = root_obj.types;     
    if(object_names != ""){
        tc = this->extract_type(tc, object_names); 
    }

    sd.set_type(tc); 

    if(!this->conditional_dep_stack.empty()){
        auto& top_cond_symbol = this->descriptor_map.at(this->conditional_dep_stack.top()).value_under_cond; 
        sd.set_name(pure_name + "%phi%temp"); 
        top_cond_symbol.insert_or_assign(pure_name, sd.id); 
        this->descriptor_map.emplace(sd.id, sd);
    }
  
    this->descriptor_map.emplace(sd.id, sd);
    this->add_variable(pure_name, sd.id, EXPR_TYPE::PHI); 

    return produced; 
}

void DAG::coalesce_phi_nodes(std::unordered_map<std::string, CondData> &data_node, SymbolID_t &sym_count){
    
    for(auto& [name, cond_data] : data_node){
        // Extract the alt value
        SymbolID_t alt_value = cond_data.alternative;
        auto& stk = cond_data.conditional_stack;
        
        std::string obj_name = name; 
        if(auto pos = name.find(".") ; pos != name.npos){
            obj_name = name.substr(0, pos); 
        }

        auto [_, last_ident] = get_last_ident(obj_name);  
        auto ident_val = this->descriptor_map.at(last_ident).value_node; 

        if(cond_data.alternative == 0){
            alt_value = ident_val;  
        }

        assert(("Conditional Stack Cannot be empty", !(stk.empty())));

        while(!stk.empty()){
            auto& cond_option = stk.back(); 
            auto& cond_obj = this->descriptor_map.at(cond_option.condition); 
            alt_value = construct_phi_node(name, cond_option, alt_value, cond_obj.bytecode_start, cond_obj.bytecode_end, sym_count); 
            stk.pop_back(); 
        }
    }

}

void DAG::complete_if_statement(SymbolID_t &sym_count){ 

    // Prepopulate the variable phi nodes: 
    std::unordered_map<std::string, CondData> variable_phi;
    for(auto& item : this->affected_variables.top()){
        variable_phi.emplace(item, CondData{});     
    }   
    this->affected_variables.pop(); 
    
    while(true){
        auto& cond_sym = this->descriptor_map.at(this->conditional_dep_stack.top()); 
        this->conditional_dep_stack.pop(); 

        switch(cond_sym.expr_type){
            case SymbolGraph::EXPR_TYPE::COND_JUNC:  
            case SymbolGraph::EXPR_TYPE::COND_JUNC_ELIF: {
                for(auto& [name, cond_data] : variable_phi){
                    SymbolID_t value; 

                    if(cond_sym.value_under_cond.contains(name)){
                        value = cond_sym.value_under_cond.at(name);
                    }
                    else{
                        auto [_, last_ident] = get_last_ident(name);  
                        auto ident_val = this->descriptor_map.at(last_ident).value_node; 
                        value = ident_val; 
                    }

                    cond_data.conditional_stack.push_front(CondValue{.condition = cond_sym.conditional_symbol, .value = value}); 
                }

                if(cond_sym.expr_type == SymbolGraph::EXPR_TYPE::COND_JUNC){
                    goto list_create; 
                }

                break; 
            }
            case SymbolGraph::EXPR_TYPE::COND_JUNC_ELSE :{
                for(auto& [symbol_name, value] : cond_sym.value_under_cond){
                      
                    if(!variable_phi.contains(symbol_name)){
                        variable_phi.emplace(symbol_name, CondData{}); 
                    }
                    auto& cond_data = variable_phi.at(symbol_name); 
                    cond_data.alternative = value; 
                }  
                break; 
            }
            default:{
                throw std::runtime_error("Im not sure... this really shouldnt be happening"); 

            }
        }
    }    

    list_create: 

    // Perform final check on alternative value symbol
    this->coalesce_phi_nodes(variable_phi, sym_count);
}

void DAG::start_for_statement(std::array<bool, 3>& set_fields){
    auto& for_stk = this->descriptor_map.at(this->symbol_desc.top());
    this->symbol_desc.pop();  
    this->affected_variables.push({}); 

    for(int i = 0; i < 3; i++){
        if(set_fields.at(i)){
            auto& sub_obj = this->symbol_desc.top(); 
            this->symbol_desc.pop(); 

            switch(i){
                case 2: 
                    //  init_statement
                    for_stk.for_value.init_symbol = sub_obj; 
                    break; 
                case 1:
                    //  Condition Expression
                    for_stk.for_value.cond_expr = sub_obj; 
                    break; 
                case 0: 
                    //  Add incremenet expression
                    for_stk.for_value.incrementor = sub_obj; 
                    break; 
                default: 
                    throw std::runtime_error("Start For statement shouldn't be happening"); 
            }   
        }
    }

    for_stk.set_name("for"); 

    this->conditional_dep_stack.push(for_stk.id); 
}

// Add loop unrolling to remove repetitve statements. 
void DAG::complete_for_statement(SymbolID_t &counter){
    std::cout<<"Completing the for statement"<<std::endl; 
    this->affected_variables.pop(); 
    auto& for_obj = this->descriptor_map.at(this->conditional_dep_stack.top()); 
    this->conditional_dep_stack.pop(); 
    
    auto delete_frame = this->variable_stk.back(); 
    this->pop_symbol_frame(); 

    // Transfer the old index to the new index; 
    for(auto& [var_name, index]  : delete_frame.named_descs){
        SymbolDescriptor sd{
            counter++, 
            0, 
            0, 
            EXPR_TYPE::ITER_PHI, 
        }; 

        sd.set_iter_phi(for_obj.id, index); 
        auto [name, type] = this->get_last_ident(var_name); 
        sd.set_type(this->descriptor_map.at(type).types); 

        this->descriptor_map.emplace(sd.id, sd); 
        this->add_variable(var_name, sd.id, EXPR_TYPE::ITER_PHI); 
    }


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
        std::cout<<"Left Type: "<<lhs.symbol_name<<std::endl; 
        std::cout<<"Right Type: "<<rhs.symbol_name<<std::endl; 
        throw std::runtime_error("Data of different types to be implemented: " + lhs.types.root_type + " " + rhs.types.root_type); 
    }
}













std::string DAG::DEBUG_print_sym_type(EXPR_TYPE type){
    switch(type){
        case EXPR_TYPE::ACCESS: 
            return "ACCESS"; 
        case EXPR_TYPE::DEVICE_ACCESS: 
            return "DEVICE_ACCESS";
        case EXPR_TYPE::DEVICE_DECLARATION:
            return "DEVICE_DECLARATION"; 
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
        case EXPR_TYPE::PHI:
            return "PHI"; 
        case EXPR_TYPE::FOR: 
            return "FOR"; 
        case EXPR_TYPE::ITER_PHI:
            return "ITER_PHI"; 
        default: 
            return "UNADDED_EXPR_TYPE"; 
    }
}


void DAG::DEBUG_print_symbol(const SymbolDescriptor& sdesc){
    std::cout<<"Type: "<<this->DEBUG_print_sym_type(sdesc.expr_type)<<std::endl;
    std::cout<<"Symbol Id: "<<sdesc.id<<std::endl; 
    std::cout<<"Symbol Name: "<<sdesc.symbol_name<<std::endl; 
    std::cout<<"Scope Depth: "<<sdesc.stack_depth<<std::endl; 
    if(sdesc.root_symbol_name != ""){
        std::cout<<"Root Symbol Name: "<<sdesc.root_symbol_name<<std::endl; 
    }
    
    std::cout<<"Bytecode Range: "<<sdesc.bytecode_start<<" => "<<sdesc.bytecode_end<<std::endl; 
    if(!sdesc.symbolic_depend.empty()){
    std::cout<<"Deps: "; 
        for(auto& item : sdesc.symbolic_depend){
            std::cout<<"\t"<<item<<" :  "<<this->descriptor_map.at(item).symbol_name<<std::endl; 
        }
    }

    
    if(sdesc.expr_type == EXPR_TYPE::PHI){
        std::cout<<std::endl; 
        auto& cond_sym = this->descriptor_map.at(sdesc.phi_value.condition); 
        auto& true_val = this->descriptor_map.at(sdesc.phi_value.true_value); 
        auto false_sym = sdesc.phi_value.false_value; 
        std::cout<<"Conditional Symbol: "<<cond_sym.id<<" Name: "<<cond_sym.symbol_name<<std::endl; 
        std::cout<<"True Symbol: "<<true_val.id<<" Name: "<<true_val.symbol_name<<std::endl; 
        
        if(false_sym != 0){
            auto false_val = this->descriptor_map.at(sdesc.phi_value.false_value); 
            std::cout<<"False Symbol: "<<false_val.id<<" Name: "<<false_val.symbol_name<<std::endl; 
        }
        else{
            std::cout<<"False Symbol: "<<0<<" Name: __UNITITALIZED__"<<std::endl; 
        }

        std::cout<<std::endl; 
    }
    else if(sdesc.expr_type == EXPR_TYPE::FOR){
        std::cout<<std::endl;    
        auto& init_sym = this->descriptor_map.at(sdesc.for_value.init_symbol); 
        auto& cond_sym = this->descriptor_map.at(sdesc.for_value.cond_expr); 
        auto& increment_sym = this->descriptor_map.at(sdesc.for_value.incrementor); 
   
   
        std::cout<<"Intiator Symbol: "<<init_sym.id<<" Name: "<<init_sym.symbol_name<<std::endl; 
        std::cout<<"Conditional Symbol: "<<cond_sym.id<<" Name: "<<cond_sym.symbol_name<<std::endl; 
        std::cout<<"Increment Symbol: "<<increment_sym.id<<" Name: "<<increment_sym.symbol_name<<std::endl; 

    }
    else if(sdesc.expr_type == EXPR_TYPE::ITER_PHI){
        std::cout<<std::endl;    
        auto& for_symbol = this->descriptor_map.at(sdesc.iter_phi.loop_symbol); 
        auto& assignment_symbol = this->descriptor_map.at(sdesc.iter_phi.assignment_symbol); 
   
        std::cout<<"For Symbol: "<<for_symbol.id<<" | Name: "<<for_symbol.symbol_name<<std::endl; 
        std::cout<<"Assignment Symbol: "<<assignment_symbol.id<<" | Name: "<<assignment_symbol.symbol_name<<std::endl; 
    }
    else if(sdesc.expr_type == EXPR_TYPE::SUBSCRIPT){
        std::cout<<std::endl;    
        auto& root_obj = this->descriptor_map.at(sdesc.subscript_val.root_id); 
        auto& index_obj = this->descriptor_map.at(sdesc.subscript_val.index_id); 
   
        std::cout<<"Root id: "<<root_obj.id<<" | Name: "<<root_obj.symbol_name<<std::endl; 
        std::cout<<"Index id: "<<index_obj.id<<" | Name: "<<index_obj.symbol_name<<std::endl; 
    }
    else if(sdesc.expr_type == EXPR_TYPE::BINARY_NO_COM){
        std::cout<<std::endl;    
        auto& lhs = this->descriptor_map.at(sdesc.binary_val.lhs_id); 
        auto& rhs = this->descriptor_map.at(sdesc.binary_val.rhs_id); 
   
        std::cout<<"LHS id: "<<lhs.id<<" | Name: "<<lhs.symbol_name<<std::endl; 
        std::cout<<"RHS id: "<<rhs.id<<" | Name: "<<rhs.symbol_name<<std::endl; 

    }


  
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
    
    /*
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




