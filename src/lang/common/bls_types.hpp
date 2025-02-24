#pragma once
// #include "heap_descriptors.hpp"
#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <concepts>
#include <vector>

namespace BlsLang {

  template<typename T>
  concept Boolean = std::same_as<std::remove_cv_t<std::remove_reference_t<T>>, bool>;

  template<typename T>
  concept Integer = !Boolean<T> && std::is_integral_v<std::remove_cv_t<std::remove_reference_t<T>>>;

  template<typename T>
  concept Float = std::is_floating_point_v<std::remove_cv_t<std::remove_reference_t<T>>>;

  template<typename T>
  concept Numeric = Integer<T> || Float<T>;

  template<typename T, typename U>
  concept BinaryOperable = !(Boolean<T> || Boolean<U>);

  template<typename T, typename U>
  concept WeaklyComparable = requires(T a, U b) {
    { a == b };
    { a != b };
  };

  template<typename T, typename U>
  concept Comparable = requires(T a, U b) {
    { a < b };
    { a <= b };
    { a > b };
    { a >= b };
  }
  && WeaklyComparable<T, U>
  && BinaryOperable<T, U>;

  template<typename T, typename U>
  concept Addable = requires(T a, U b) {
    { a + b };
  }
  && BinaryOperable<T, U>;

  template<typename T, typename U>
  concept Subtractable = requires(T a, U b) {
    { a - b };
  }
  && BinaryOperable<T, U>;

  template<typename T, typename U>
  concept Multiplicable = requires(T a, U b) {
    { a * b };
  }
  && BinaryOperable<T, U>;

  template<typename T, typename U>
  concept Divisible = requires(T a, U b) {
    { a / b };
  }
  && BinaryOperable<T, U>;

  class HeapDescriptor;

  #define DEVTYPE_BEGIN(name) \
  struct name { 
  #define ATTRIBUTE(name, type) \
  type name;
  #define DEVTYPE_END \
  };
  #include "DEVTYPES.LIST"
  #undef DEVTYPE_BEGIN
  #undef ATTRIBUTE
  #undef DEVTYPE_END

  enum class TYPE {
      void_t
    , bool_t
    , int_t
    , float_t
    , string_t

    #define CONTAINER_BEGIN(name, _) \
    , name##_t
    #define METHOD(_, __)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END

    #define DEVTYPE_BEGIN(name) \
    , name
    #define ATTRIBUTE(_, __)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    , COUNT
  };

  enum class CONTAINER_TYPE {
    #define CONTAINER_BEGIN(name, _) \
    name = static_cast<int>(TYPE::name##_t),
    #define METHOD(_, __)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
  };

  enum class DEVTYPE {
    #define DEVTYPE_BEGIN(name) \
    name = static_cast<int>(TYPE::name),
    #define ATTRIBUTE(_, __)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
  };

  inline constexpr TYPE getTypeEnum(const std::string& type) {
    if (type == "void") { return TYPE::void_t; }
    if (type == "bool") { return TYPE::bool_t; }
    if (type == "int") { return TYPE::int_t; }
    if (type == "float") { return TYPE::float_t; }
    if (type == "string") { return TYPE::string_t; }
    #define CONTAINER_BEGIN(name, _) \
    if (type == #name) { return TYPE::name##_t; }
    #define METHOD(_, __)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
    #define DEVTYPE_BEGIN(name) \
    if (type == #name) { return TYPE::name; }
    #define ATTRIBUTE(_, __)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    return TYPE::COUNT;
  }

  struct TypeIdenfier {
    TYPE name;
    std::vector<TypeIdenfier> args;
  };

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

  inline BlsType operator!(const BlsType& operand) {
    return std::visit([](const auto& a) -> BlsType {
      if constexpr (Boolean<decltype(a)>) {
        return !a;
      }
      else {
        throw std::runtime_error("Operand of '!' must have boolean type.");
      }
    }, operand);
  }

  inline BlsType operator-(const BlsType& operand) {
    return std::visit([](const auto& a) -> BlsType {
      if constexpr (Numeric<decltype(a)>) {
        return -a;
      }
      else {
        throw std::runtime_error("Operand of '-' must have integer or float type.");
      }
    }, operand);
  }

  inline BlsType operator||(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (Boolean<decltype(a)> && Boolean<decltype(b)>) {
          return a || b;
        }
        else {
          throw std::runtime_error("Lhs and Rhs of logical expression must have boolean type.");
        }
    }, lhs, rhs);
  }

  inline BlsType operator&&(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (Boolean<decltype(a)> && Boolean<decltype(b)>) {
          return a && b;
        }
        else {
          throw std::runtime_error("Lhs and Rhs of logical expression must have boolean type.");
        }
    }, lhs, rhs);
  }

  inline BlsType operator<(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Comparable<decltype(a), decltype(b)>) {
        return a < b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not orderable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator<=(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Comparable<decltype(a), decltype(b)>) {
        return a <= b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not orderable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator>(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Comparable<decltype(a), decltype(b)>) {
        return a > b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not orderable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator>=(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Comparable<decltype(a), decltype(b)>) {
        return a >= b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not orderable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator!=(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (WeaklyComparable<decltype(a), decltype(b)>) {
        return a != b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not comparable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator==(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (WeaklyComparable<decltype(a), decltype(b)>) {
        return a == b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not comparable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator+(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Addable<decltype(a), decltype(b)>) {
        return a + b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not addable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator-(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Subtractable<decltype(a), decltype(b)>) {
        return a - b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not subtractable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator*(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Multiplicable<decltype(a), decltype(b)>) {
        return a * b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not multiplicable.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator/(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Divisible<decltype(a), decltype(b)>) {
        return a / b;
      }
      else {
        throw std::runtime_error("Lhs and Rhs are not divisible.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator%(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Integer<decltype(a)> && Integer<decltype(b)>) {
        return a % b;
      }
      else if constexpr (Numeric<decltype(a)> && Numeric<decltype(b)>) {
        return std::fmod(a, b);
      }
      else {
        throw std::runtime_error("Lhs and Rhs must be integer or float types.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator^(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Numeric<decltype(a)> && Numeric<decltype(b)>) {
        return std::pow(a, b);
      }
      else {
        throw std::runtime_error("Lhs and Rhs must be integer or float types.");
      }
    }, lhs, rhs);
  }

}