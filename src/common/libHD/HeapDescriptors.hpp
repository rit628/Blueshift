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

using BlsType = std::variant<
      std::monostate
    , bool
    , int
    , float
    , std::string
    , std::shared_ptr<HeapDescriptor>
  >;

enum class ObjType : uint8_t {
    PRIMATIVE, 
    MAP, 
    VECTOR 
}; 

enum class Desctype : uint8_t {
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

static std::string stringify(const BlsType &value){
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

        virtual BlsType& access(BlsType &obj) = 0;

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
        std::shared_ptr<std::unordered_map<std::string, BlsType>> map;    
        std::mutex mux; 

    public:

        friend class DynamicMessage; 
        friend class VM; 

        MapDescriptor(Desctype cont_code){
            this->contType = cont_code; 
            this->map = std::make_shared<std::unordered_map<std::string, BlsType>>(); 
        }
  
        ObjType getType() override{
            return ObjType::MAP; 
        }

        // Add non-string handling later: 
        BlsType& access(BlsType &obj) override{
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
        void emplace(BlsType obj, BlsType newDesc){
            std::scoped_lock bob(mux); 
            std::string newKey = stringify(obj); 
            this->map->operator[](newKey) = newDesc; 
        }


        // Also only used for debugging
        std::unordered_map<std::string, BlsType> getMap(){
            return *this->map; 
        }

        int getSize() override{
            return this->map->size(); 
        }
};


class VectorDescriptor : public HeapDescriptor, std::enable_shared_from_this<VectorDescriptor>{
    private: 
        std::shared_ptr<std::vector<BlsType>> vector; 
        std::mutex mux; 

    public: 
        friend class DynamicMessage; 
        friend class VM; 

        VectorDescriptor(Desctype cont_code){
            this->contType = cont_code; 
            vector = std::make_shared<std::vector<BlsType>>(); 
        } 

        ObjType getType() override{
            return ObjType::VECTOR; 
        }

        static std::shared_ptr<VectorDescriptor> create(Desctype cont_code) {
            return std::shared_ptr<VectorDescriptor>(new VectorDescriptor(cont_code));
        }


        BlsType& access(BlsType &int_acc) override{
            std::scoped_lock bob(mux); 

            if(std::holds_alternative<int>(int_acc)){
                int index = std::get<int>(int_acc); 
                return this->vector->at(index); 
              }
            else{
                throw std::runtime_error("Cannot index a list with a non-integer"); 
            }
        }

        void append(BlsType newObj){
            std::scoped_lock bob(mux);  
            this->vector->push_back(newObj);         
        }

        // DEBUG FUNCTION ONLY (for now maybe): 
        std::vector<BlsType>& getVector(){
            return *this->vector; 
        }

        int getSize() override{
            return this->vector->size(); 
        }
};
