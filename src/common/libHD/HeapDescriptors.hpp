#pragma once

#include <memory>
#include <variant> 
#include <stack> 
#include <iostream>
#include <unordered_map>
#include <variant> 
#include <vector> 
#include <mutex>

// Forward Declaration of Dynamic Message class 
class HeapDescriptor; 

#define PRIMATIVES int, float, std::string, bool, std::shared_ptr<HeapDescriptor>
using Prim = std::variant<PRIMATIVES>; 

enum class ObjType{
    PRIMATIVE, 
    MAP, 
    VECTOR 
}; 

enum class Desctype : uint8_t{
    VECTOR, 
    MAP, 
    UINT32, 
    FLOAT, 
    STRING, 
    CHAR, 
    UCHAR,
    BOOL, 
    DEVTYPE, 
    NONE, 
    // Used for the storage of constant size, structs
    ANY, 
}; 

static std::string stringify(const Prim &value){
    if(std::holds_alternative<bool>(value)){
        return std::to_string(std::get<bool>(value)); 
    }
    else if(std::holds_alternative<int>(value)){
        return std::to_string(std::get<int>(value)); 
    }
    else if(std::holds_alternative<float>(value)){
        return std::to_string(std::get<float>(value)); 
    }
    else if(std::holds_alternative<std::string>(value)){
        return std::get<std::string>(value); 
    }
    else{
        throw std::runtime_error("Heap Descriptor Stringification not implemented!"); 
    }
}


class HeapDescriptor{
    public: 
        Desctype contType = Desctype::ANY; 

        HeapDescriptor() = default; 
 
        virtual Desctype getCont(){
            return this->contType; 
        }

        virtual std::shared_ptr<HeapDescriptor> access(Prim &obj){
            std::cout<<"Based Object "<<std::endl; 
            return nullptr; 
        }

        virtual int getSize(){
            return 1; 
        }

        // Constructors
        virtual ~HeapDescriptor() = default; 
    
        virtual ObjType getType(){
            return ObjType::PRIMATIVE; 
        }


        
};


class MapDescriptor : public HeapDescriptor{
    private: 
        std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<HeapDescriptor>>> map;    
        std::mutex mux; 

    public:

        friend class DynamicMessage; 
        friend class VM; 

        MapDescriptor(Desctype cont_code){
            this->contType = cont_code; 
            this->map = std::make_shared<std::unordered_map<std::string, std::shared_ptr<HeapDescriptor>>>(); 
        }
  
        ObjType getType() override{
            return ObjType::MAP; 
        }

        // Add non-string handling later: 
        std::shared_ptr<HeapDescriptor> access(Prim &obj) override{
            std::scoped_lock bob(mux); 

            std::string accessor = stringify(obj); 
            if(this->map->find(accessor) != this->map->end()){
                return this->map->operator[](accessor); 
            }
            else{
                throw std::invalid_argument("Could not find the object of name: " + accessor); 
            } 
        }

        // WE assume for now that all objects are of type string 
        void emplace(Prim obj, std::shared_ptr<HeapDescriptor> newDesc){
            std::scoped_lock bob(mux); 
            std::string newKey = stringify(obj); 
            this->map->operator[](newKey) = newDesc; 
        }


        // Also only used for debugging
        std::unordered_map<std::string, std::shared_ptr<HeapDescriptor>> getMap(){
            return *this->map; 
        }

        int getSize() override{
            return this->map->size(); 
        }
};


class VectorDescriptor : public HeapDescriptor, std::enable_shared_from_this<VectorDescriptor>{
    private: 
        std::shared_ptr<std::vector<std::shared_ptr<HeapDescriptor>>> vector; 
        std::mutex mux; 

    public: 
        friend class DynamicMessage; 
        friend class VM; 

        VectorDescriptor(Desctype cont_code){
            this->contType = cont_code; 
            vector = std::make_shared<std::vector<std::shared_ptr<HeapDescriptor>>>(); 
        } 

        ObjType getType() override{
            return ObjType::VECTOR; 
        }

        static std::shared_ptr<VectorDescriptor> create(Desctype cont_code) {
            return std::shared_ptr<VectorDescriptor>(new VectorDescriptor(cont_code));
        }


        std::shared_ptr<HeapDescriptor> access(Prim &int_acc) override{
            std::scoped_lock bob(mux); 

            if(std::holds_alternative<int>(int_acc)){
                int index = std::get<int>(int_acc); 
                return this->vector->at(index); 
            }
            else{
                throw std::runtime_error("Cannot index a list with a non-integer"); 
            }
        }

        void append(std::shared_ptr<HeapDescriptor> newObj){
            std::scoped_lock bob(mux);  
            this->vector->push_back(newObj);         
        }

        // DEBUG FUNCTION ONLY (for now maybe): 
        std::vector<std::shared_ptr<HeapDescriptor>>& getVector(){
            return *this->vector; 
        }

        int getSize() override{
            return this->vector->size(); 
        }
};


class PrimDescriptor : public HeapDescriptor{
    public: 

        ~PrimDescriptor() override = default;

        Prim variant; 

        int getSize() override{
            if(std::holds_alternative<std::string>(this->variant)){
                auto str = std::get<std::string>(this->variant); 
                return str.size(); 
            }
            else{
                return 1; 
            }
        }

        PrimDescriptor(Prim cpy) : variant(cpy) {}; 

        ObjType getType() override{
            return ObjType::PRIMATIVE; 
        }
};
