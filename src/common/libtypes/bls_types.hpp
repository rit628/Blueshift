#pragma once
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

enum class TYPE : uint32_t {
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
  , NONE
  , COUNT
};

enum class DEVTYPE : uint32_t {
  #define DEVTYPE_BEGIN(name) \
  name = static_cast<uint32_t>(TYPE::name),
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
  friend bool operator<(const BlsType& lhs, const BlsType& rhs);
  friend bool operator<=(const BlsType& lhs, const BlsType& rhs);
  friend bool operator>(const BlsType& lhs, const BlsType& rhs);
  friend bool operator>=(const BlsType& lhs, const BlsType& rhs);
  friend bool operator!=(const BlsType& lhs, const BlsType& rhs);
  friend bool operator==(const BlsType& lhs, const BlsType& rhs);
  friend BlsType operator+(const BlsType& lhs, const BlsType& rhs);
  friend BlsType operator-(const BlsType& lhs, const BlsType& rhs);
  friend BlsType operator*(const BlsType& lhs, const BlsType& rhs);
  friend BlsType operator/(const BlsType& lhs, const BlsType& rhs);
  friend BlsType operator%(const BlsType& lhs, const BlsType& rhs);
  friend BlsType operator^(const BlsType& lhs, const BlsType& rhs);
  friend bool typeCompatible(const BlsType& lhs, const BlsType& rhs);
  friend std::ostream& operator<<(std::ostream& os, const BlsType& obj);
  template<typename Archive>
  void serialize(Archive& ar, const unsigned int version);
};

template<>
struct std::hash<BlsType> {
    size_t operator()(const BlsType& obj) const;
};

class HeapDescriptor {
  protected: 
    TYPE objType = TYPE::ANY;
    TYPE keyType = TYPE::ANY;
    TYPE contType = TYPE::ANY;
    BlsType sampleElement = std::monostate();

  public:
    HeapDescriptor() = default;
    virtual ~HeapDescriptor() = default;
    void setKey(TYPE keyType) { this->keyType = keyType; }
    void setCont(TYPE contType) { this->contType = contType; }
    void setType(TYPE objType) { this->objType = objType; }
    TYPE getKey() { return this->keyType; }
    TYPE getCont() { return this->contType; }
    TYPE getType() { return this->objType; }
    BlsType& getSampleElement() { return this->sampleElement; }
    virtual BlsType& access(BlsType &obj) = 0;
    BlsType& access(BlsType &&obj) { return access(obj); };
    virtual int getSize() { return 1; }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version);
};

class MapDescriptor : public HeapDescriptor{
  protected: 
    std::shared_ptr<std::unordered_map<std::string, BlsType>> map;    
    std::mutex mux;

  public:
    friend class DynamicMessage; 

    MapDescriptor(TYPE contType);
    MapDescriptor(TYPE objType, TYPE keyType, TYPE contType);
    // Add non-string handling later:
    BlsType& access(BlsType &obj) override;
    // WE assume for now that all objects are of type string 
    void emplace(BlsType& obj, BlsType& newDesc);
    // Also only used for debugging
    std::unordered_map<std::string, BlsType>& getMap() { return *this->map; }
    int getSize() override { return this->map->size(); }

    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version);
};

class VectorDescriptor : public HeapDescriptor, std::enable_shared_from_this<VectorDescriptor>{
  protected: 
    std::shared_ptr<std::vector<BlsType>> vector; 
    std::mutex mux; 

  public: 
    friend class DynamicMessage; 

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

    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version);
};

constexpr TYPE getTypeFromName(const std::string& type) {
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
TYPE getType(const BlsType& obj);
std::string stringify(const BlsType& value);

template<typename Archive>
inline void BlsType::serialize(Archive& ar, const unsigned int version) {
  auto type = getType(*this);
  ar & type;
  switch (type) {
    case TYPE::void_t:
      *this = std::monostate();
    break;
    case TYPE::bool_t:
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = bool();
      }
      ar & std::get<bool>(*this);
    break;
    case TYPE::int_t:
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = int64_t();
      }
      ar & std::get<int64_t>(*this);
    break;
    case TYPE::float_t:
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = double();
      }
      ar & std::get<double>(*this);
    break;
    case TYPE::string_t:
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = std::string();
      }
      ar & std::get<std::string>(*this);
    break;
    case TYPE::list_t: {
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = std::make_shared<VectorDescriptor>(TYPE::ANY);;
      }
      auto heapDesc = std::get<std::shared_ptr<HeapDescriptor>>(*this);
      auto resolvedDesc = std::dynamic_pointer_cast<VectorDescriptor>(heapDesc);
      ar & *resolvedDesc.get();
      break;
    }
    case TYPE::map_t: {
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = std::make_shared<MapDescriptor>(TYPE::ANY);;
      }
      auto heapDesc = std::get<std::shared_ptr<HeapDescriptor>>(*this);
      auto resolvedDesc = std::dynamic_pointer_cast<MapDescriptor>(heapDesc);
      ar & *resolvedDesc.get();
      break;
    }
    default:
      throw std::runtime_error("Non serializable type");
    break;
  }
}

template<typename Archive>
void HeapDescriptor::serialize(Archive& ar, const unsigned int version) {
  ar & objType;
  ar & keyType;
  ar & contType;
}

template<typename Archive>
void MapDescriptor::serialize(Archive & ar, const unsigned int version) {
  // serialize base class information
  ar & boost::serialization::base_object<HeapDescriptor>(*this);
  ar & *map.get();
}

template<typename Archive>
void VectorDescriptor::serialize(Archive & ar, const unsigned int version) {
  ar & boost::serialization::base_object<HeapDescriptor>(*this);
  ar & *vector.get();
}