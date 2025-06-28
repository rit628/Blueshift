#pragma once
#include "typedefs.hpp"
#include <cmath>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <cstdint>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

/* define reserved words for types in lang */
namespace BlsLang {

  constexpr auto PRIMITIVE_VOID                       ("void");
  constexpr auto PRIMITIVE_BOOL                       ("bool");
  constexpr auto PRIMITIVE_INT                        ("int");
  constexpr auto PRIMITIVE_FLOAT                      ("float");
  constexpr auto PRIMITIVE_STRING                     ("string");

  constexpr auto CONTAINER_LIST                       ("list");
  constexpr auto CONTAINER_MAP                        ("map");

  #define DEVTYPE_BEGIN(name) \
  constexpr auto DEVTYPE_##name                                          (#name);
  #define ATTRIBUTE(...)
  #define DEVTYPE_END
  #include "DEVTYPES.LIST"
  #undef DEVTYPE_BEGIN
  #undef ATTRIBUTE
  #undef DEVTYPE_END

};

enum class TYPE : uint32_t {
    void_t
  , bool_t
  , int_t
  , float_t
  , string_t
  , PRIMITIVES_END

  , list_t
  , map_t
  , CONTAINERS_END

  #define DEVTYPE_BEGIN(name) \
  , name
  #define ATTRIBUTE(...)
  #define DEVTYPE_END 
  #include "DEVTYPES.LIST"
  #undef DEVTYPE_BEGIN
  #undef ATTRIBUTE
  #undef DEVTYPE_END
  , DEVTYPES_END

  , ANY
  , NONE
  , COUNT
};

struct TypeIdentifier {
  TYPE name;
  std::vector<TypeIdentifier> args;
};

class HeapDescriptor;
class VectorDescriptor;
class MapDescriptor;

struct BlsType : std::variant<std::monostate, bool, int64_t, double, std::string, std::shared_ptr<HeapDescriptor>> {
  using std::variant<std::monostate, bool, int64_t, double, std::string, std::shared_ptr<HeapDescriptor>>::variant;
  explicit operator bool() const;
  explicit operator double() const;
  explicit operator int64_t() const;
  explicit operator std::string() const;
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

namespace TypeDef {
  template<typename T>
  concept BlueshiftConstructible = BlueshiftType<T> || std::same_as<std::remove_cv_t<std::remove_reference_t<T>>, BlsType>;

  template<BlueshiftConstructible T>
  struct Resolve {
    using type = T;
  };

  template<BlueshiftType T>
  struct Resolve<T> {
    using type = converted_t<T>;
  };

  template<List T>
  struct Resolve<T> {
    using type = std::shared_ptr<VectorDescriptor>;
  };

  template<Map T>
  struct Resolve<T> {
    using type = std::shared_ptr<MapDescriptor>;
  };

  template<typename T>
  using resolved_t = Resolve<T>::type;

}

template<>
struct std::hash<BlsType> {
    size_t operator()(const BlsType& obj) const;
};

class HeapDescriptor {
  protected: 
    TYPE objType = TYPE::ANY;
    TYPE contType = TYPE::ANY;
    std::vector<BlsType> sampleElement = {};


  public:
    enum class METHODNUM : uint16_t {
      #define METHOD_BEGIN(name, objType, ...) \
      objType##_##name,
      #define ARGUMENT(...)
      #define METHOD_END
      #include "include/LIST_METHODS.LIST"
      #include "include/MAP_METHODS.LIST"
      #undef METHOD_BEGIN
      #undef ARGUMENT
      #undef METHOD_END
      COUNT
    };

    HeapDescriptor() = default;
    virtual ~HeapDescriptor() = default;
    void setCont(TYPE contType) { this->contType = contType; }
    void setType(TYPE objType) { this->objType = objType; }
    TYPE getCont() { return this->contType; }
    TYPE getType() { return this->objType; }
    // only used for type checking
    auto& getSampleElement() { return this->sampleElement; }
    virtual BlsType& access(BlsType &obj) = 0;
    BlsType& access(BlsType &&obj) { return access(obj); };

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
    MapDescriptor(std::initializer_list<std::pair<std::string, BlsType>> elements);
    // Add non-string handling later:
    BlsType& access(BlsType &obj) override;

    #define METHOD_BEGIN(name, objType, typeArgIdx, returnType...) \
    TypeDef::resolved_t<returnType> name(
    #define ARGUMENT(name, typeArgIdx, type...) \
    TypeDef::resolved_t<type> name,
    #define METHOD_END \
    int = 0);
    #include "include/MAP_METHODS.LIST"
    #undef METHOD_BEGIN
    #undef ARGUMENT
    #undef METHOD_END

    // Also only used for debugging
    std::unordered_map<std::string, BlsType>& getMap() { return *this->map; }

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
    VectorDescriptor(std::initializer_list<BlsType> elements);
    static std::shared_ptr<VectorDescriptor> create(std::string cont_code) {
      return std::shared_ptr<VectorDescriptor>(new VectorDescriptor(cont_code));
    }
    BlsType& access(BlsType &int_acc) override;

    #define METHOD_BEGIN(name, objType, typeArgIdx, returnType...) \
    TypeDef::resolved_t<returnType> name(
    #define ARGUMENT(name, typeArgIdx, type...) \
    TypeDef::resolved_t<type> name,
    #define METHOD_END \
    int = 0);
    #include "include/LIST_METHODS.LIST"
    #undef METHOD_BEGIN
    #undef ARGUMENT
    #undef METHOD_END

    // DEBUG FUNCTION ONLY (for now maybe): 
    std::vector<BlsType>& getVector() { return *this->vector; }

    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version);
};

template<TypeDef::BlueshiftConstructible T>
BlsType createBlsType(const T& value) {
  using namespace TypeDef;
  if constexpr (Void<T>) {
    return std::monostate();
  }
  else if constexpr (Boolean<T>) {
    return bool(value);
  }
  else if constexpr (Integer<T>) {
    return int64_t(value);
  }
  else if constexpr (Float<T>) {
    return double(value);
  }
  else if constexpr (String<T>) {
    return std::string(value);
  }
  else if constexpr (List<T>) {
    auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
    typename T::value_type sampleElement;
    list->getSampleElement().assign({createBlsType(sampleElement)});
    for (auto&& element : value) {
      list->append(createBlsType(element));
    }
    return list;
  }
  else if constexpr (Map<T>) {
    auto map = std::make_shared<MapDescriptor>(TYPE::ANY);
    typename T::key_type sampleKey;
    typename T::value_type sampleElement;
    map->getSampleElement().assign({createBlsType(sampleKey), createBlsType(sampleElement)});
    for (auto&& [key, element] : value) {
      map->add(createBlsType(key), createBlsType(element));
    }
    return map;
  }
  #define DEVTYPE_BEGIN(name) \
  else if constexpr (std::same_as<T, name>) {  \
      auto devtype = std::make_shared<MapDescriptor>(TYPE::ANY);
  #define ATTRIBUTE(name, ...) \
      BlsType name##_key = #name; \
      BlsType name##_val = value.name; \
      devtype->add(name##_key, name##_val);
  #define DEVTYPE_END \
    return devtype; \
  }
  #include "DEVTYPES.LIST"
  #undef DEVTYPE_BEGIN
  #undef ATTRIBUTE
  #undef DEVTYPE_END
  else {
    return value;
  }
}

template<TypeDef::BlueshiftConstructible T>
T createFromBlsType(const BlsType& value) {
  using namespace TypeDef;
  if constexpr (Void<T> || Boolean<T> || Integer<T> || Float<T> || String<T>) {
    return std::get<converted_t<T>>(value);
  }
  else if constexpr (List<T>) {
    T result;
    auto& list = std::dynamic_pointer_cast<VectorDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(value))->getVector();
    for (auto&& element : list) {
      result.push_back(createFromBlsType<T::value_type>(element));
    }
    return result;
  }
  else if constexpr (Map<T>) {
    T result;
    auto& map = std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(value))->getMap();
    for (auto&& [key, value] : map) {
      result.emplace({createFromBlsType<T::key_type>(key), createFromBlsType<T::value_type>(value)});
    }
    return result;
  }
  #define DEVTYPE_BEGIN(name) \
  else if constexpr (std::same_as<T, name>) {  \
    T devtype; \
    auto& map = std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(value))->getMap();
    #define ATTRIBUTE(name, ...) \
    devtype.name = map.at(#name);
    #define DEVTYPE_END \
    return devtype; \
  }
  #include "DEVTYPES.LIST"
  #undef DEVTYPE_BEGIN
  #undef ATTRIBUTE
  #undef DEVTYPE_END
  else {
    return value;
  }
}

template<TypeDef::BlueshiftConstructible T>
constexpr T resolveBlsType(const BlsType& value) {
  using namespace TypeDef;
  if constexpr (Void<T> || Boolean<T> || Integer<T> || Float<T> || String<T>) {
    return std::get<resolved_t<T>>(value);
  }
  else if constexpr (List<T> || Map<T>) {
    return std::dynamic_pointer_cast<resolved_t<T>>(std::get<std::shared_ptr<HeapDescriptor>>(value));
  }
  else {
    return value;
  }
}

constexpr TYPE getTypeFromName(const std::string& type) {
  if (type == BlsLang::PRIMITIVE_VOID) return TYPE::void_t;
  if (type == BlsLang::PRIMITIVE_BOOL) return TYPE::bool_t;
  if (type == BlsLang::PRIMITIVE_INT) return TYPE::int_t;
  if (type == BlsLang::PRIMITIVE_FLOAT) return TYPE::float_t;
  if (type == BlsLang::PRIMITIVE_STRING) return TYPE::string_t;

  if (type == BlsLang::CONTAINER_LIST) return TYPE::list_t;
  if (type == BlsLang::CONTAINER_MAP) return TYPE::map_t;

  #define DEVTYPE_BEGIN(name) \
  if (type == BlsLang::DEVTYPE_##name) return TYPE::name;
  #define ATTRIBUTE(...)
  #define DEVTYPE_END 
  #include "DEVTYPES.LIST"
  #undef DEVTYPE_BEGIN
  #undef ATTRIBUTE
  #undef DEVTYPE_END

  if (type == "ANY") return TYPE::ANY;
  return TYPE::COUNT;
}

constexpr const std::string getTypeName(TYPE type) {
  switch (type) {
    case TYPE::void_t:
      return BlsLang::PRIMITIVE_VOID;
    break;
    case TYPE::bool_t:
      return BlsLang::PRIMITIVE_BOOL;
    break;
    case TYPE::int_t:
      return BlsLang::PRIMITIVE_INT;
    break;
    case TYPE::float_t:
      return BlsLang::PRIMITIVE_FLOAT;
    break;
    case TYPE::string_t:
      return BlsLang::PRIMITIVE_STRING;
    break;
    case TYPE::list_t:
      return BlsLang::CONTAINER_LIST;
    break;
    case TYPE::map_t:
      return BlsLang::CONTAINER_MAP;
    break;
    
    #define DEVTYPE_BEGIN(name) \
    case TYPE::name: \
      return BlsLang::DEVTYPE_##name; \
    break;
    #define ATTRIBUTE(...)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END

    default:
      return "ANY";
    break;
  }
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
        *this = std::make_shared<VectorDescriptor>(TYPE::ANY);
      }
      auto heapDesc = std::get<std::shared_ptr<HeapDescriptor>>(*this);
      auto resolvedDesc = std::dynamic_pointer_cast<VectorDescriptor>(heapDesc);
      ar & *resolvedDesc.get();
    }
    break;

    case TYPE::map_t: {
      if (std::holds_alternative<std::monostate>(*this)) {
        *this = std::make_shared<MapDescriptor>(TYPE::ANY);
      }
      auto heapDesc = std::get<std::shared_ptr<HeapDescriptor>>(*this);
      auto resolvedDesc = std::dynamic_pointer_cast<MapDescriptor>(heapDesc);
      ar & *resolvedDesc.get();
    }
    break;

    default:
      throw std::runtime_error("Non serializable type");
    break;
  }
}

template<typename Archive>
void HeapDescriptor::serialize(Archive& ar, const unsigned int version) {
  ar & objType;
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