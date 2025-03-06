#pragma once
#include "error_types.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <vector>

namespace BlsLang {

  enum class TYPE {
      void_t
    , bool_t
    , int_t
    , float_t
    , string_t
    , PRIMITIVE_COUNT

    #define CONTAINER_BEGIN(name, ...) \
    , name##_t
    #define METHOD(...)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
    , CONTAINER_COUNT

    #define DEVTYPE_BEGIN(name) \
    , name
    #define ATTRIBUTE(...)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    , DEVTYPE_COUNT

    , ANY
    , COUNT
  };

  enum class DEVTYPE {
    #define DEVTYPE_BEGIN(name) \
    name = static_cast<int>(TYPE::name),
    #define ATTRIBUTE(...)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
  };

  struct TypeIdentifier {
    TYPE name;
    std::vector<TypeIdentifier> args;
  };

  class HeapDescriptor;

  struct BlsType : std::variant<std::monostate, bool, int64_t, double, std::string, std::shared_ptr<HeapDescriptor>> {
    using std::variant<std::monostate, bool, int64_t, double, std::string, std::shared_ptr<HeapDescriptor>>::variant;
    explicit operator bool() const;
    friend BlsType operator-(const BlsType& operand);
    friend BlsType operator<(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator<=(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator>(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator>=(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator!=(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator==(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator+(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator-(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator*(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator/(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator%(const BlsType& lhs, const BlsType& rhs);
    friend BlsType operator^(const BlsType& lhs, const BlsType& rhs);
    friend bool typeCompatible(const BlsType& lhs, const BlsType& rhs);
  };

  class HeapDescriptor {
    protected: 
      TYPE objType = TYPE::ANY; 
      TYPE keyType = TYPE::ANY;
      TYPE contType = TYPE::ANY; 

    public:
      HeapDescriptor() = default; 
      virtual ~HeapDescriptor() = default; 
      TYPE getKey() { return this->keyType; }
      TYPE getCont() { return this->contType; }
      TYPE getType() { return this->objType; }
      virtual BlsType& access(BlsType &obj) = 0;
      BlsType& access(BlsType &&obj) { return access(obj); };
      virtual int getSize() { return 1; }

  };

  class MapDescriptor : public HeapDescriptor{
    protected: 
      std::shared_ptr<std::unordered_map<std::string, BlsType>> map;    
      std::mutex mux;

    public:
      friend class DynamicMessage; 
      friend class VM; 

      MapDescriptor(std::string cont_code);
      MapDescriptor(TYPE objType, TYPE keyType, TYPE contType);
      // Add non-string handling later:
      BlsType& access(BlsType &obj) override;
      // WE assume for now that all objects are of type string 
      void emplace(BlsType& obj, BlsType& newDesc);
      // Also only used for debugging
      std::unordered_map<std::string, BlsType> getMap() { return *this->map; }
      int getSize() override { return this->map->size(); }

  };

  class VectorDescriptor : public HeapDescriptor, std::enable_shared_from_this<VectorDescriptor>{
    protected: 
      std::shared_ptr<std::vector<BlsType>> vector; 
      std::mutex mux; 

    public: 
      friend class DynamicMessage; 
      friend class VM; 

      VectorDescriptor(std::string cont_code);
      VectorDescriptor(TYPE contType);
      static std::shared_ptr<VectorDescriptor> create(std::string cont_code) {
        return std::shared_ptr<VectorDescriptor>(new VectorDescriptor(cont_code));
      }
      BlsType& access(BlsType &int_acc) override;
      void append(BlsType& newObj);
      // DEBUG FUNCTION ONLY (for now maybe): 
      std::vector<BlsType>& getVector() { return *this->vector; }
      int getSize() override { return this->vector->size(); }

  };


  class PrimDescriptor : public HeapDescriptor{
      
    public:
      PrimDescriptor(BlsType cpy) : variant(cpy) {}; 
      ~PrimDescriptor() override = default;
      int getSize() override;
      
      BlsType variant;

  };

  constexpr TYPE getTypeEnum(const std::string& type) {
    if (type == "void") return TYPE::void_t;
    if (type == "bool") return TYPE::bool_t;
    if (type == "int") return TYPE::int_t;
    if (type == "float") return TYPE::float_t;
    if (type == "string") return TYPE::string_t;
    #define CONTAINER_BEGIN(name, ...) \
    if (type == #name) return TYPE::name##_t;
    #define METHOD(...)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
    #define DEVTYPE_BEGIN(name) \
    if (type == #name) return TYPE::name;
    #define ATTRIBUTE(...)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    if (type == "ANY") return TYPE::ANY;
    return TYPE::COUNT;
  }
  TYPE getTypeEnum(const BlsType& obj);
  std::string stringify(const BlsType& value);

}