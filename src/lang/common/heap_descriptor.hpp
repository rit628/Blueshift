#pragma once
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace BlsLang {

    class HeapDescriptor;
    using BlsType = std::variant<
        std::monostate
        , bool
        , int64_t
        , double
        , std::string
        , std::shared_ptr<HeapDescriptor>
    >;

    enum class ObjType{
        PRIMATIVE, 
        MAP, 
        VECTOR 
    }; 
    
    enum class ContainerType{
        ANY, 
        INT,
        FLOAT, 
        STRING, 
        BOOL, 
        VECTOR, 
        MAP, 
    }; 
    
    
    inline std::unordered_map<std::string, ContainerType> ContMap = {
        {"ANY", ContainerType::ANY}, 
    
        {"INT", ContainerType::INT}, 
        {"FLOAT", ContainerType::FLOAT}, 
        {"STRING", ContainerType::STRING}, 
        {"BOOL", ContainerType::BOOL},
    
        {"VECTOR", ContainerType::VECTOR}, 
        {"MAP", ContainerType::MAP}, 
    }; 
    
    
    static std::string stringify(const BlsType &value){
        if(std::holds_alternative<bool>(value)){
          return std::to_string(std::get<bool>(value)); 
        }
        else if(std::holds_alternative<int64_t>(value)){
          return std::to_string(std::get<int64_t>(value)); 
        }
        else if(std::holds_alternative<double>(value)){
          return std::to_string(std::get<double>(value)); 
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
          ContainerType contType = ContainerType::ANY; 
    
          HeapDescriptor() = default; 
    
          virtual ContainerType getCont(){
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
    
          MapDescriptor(std::string cont_code){
            this->contType = ContMap[cont_code]; 
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
          void emplace(BlsType& obj, BlsType& newDesc){
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
    
          VectorDescriptor(std::string cont_code){
            this->contType = ContMap[cont_code]; 
            vector = std::make_shared<std::vector<BlsType>>(); 
          } 
    
          ObjType getType() override{
            return ObjType::VECTOR; 
          }
    
          static std::shared_ptr<VectorDescriptor> create(std::string cont_code) {
            return std::shared_ptr<VectorDescriptor>(new VectorDescriptor(cont_code));
          }
    
    
          BlsType& access(BlsType &int_acc) override{
            std::scoped_lock bob(mux); 
    
            if(std::holds_alternative<int64_t>(int_acc)){
              int index = std::get<int64_t>(int_acc); 
              return this->vector->at(index); 
            }
            else{
              throw std::runtime_error("Cannot index a list with a non-integer"); 
            }
          }
    
          void append(BlsType& newObj){
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
    
    
    class PrimDescriptor : public HeapDescriptor{
        public: 
    
        ~PrimDescriptor() override = default;
    
        BlsType variant; 
    
        int getSize() override{
          if (std::holds_alternative<std::string>(this->variant)){
            auto str = std::get<std::string>(this->variant); 
            return str.size(); 
          }
          else {
            return 1; 
          }
        }
    
        PrimDescriptor(BlsType cpy) : variant(cpy) {}; 
    
        ObjType getType() override{
          return ObjType::PRIMATIVE; 
        }
    };

}

