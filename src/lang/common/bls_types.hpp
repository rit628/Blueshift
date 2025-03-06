#pragma once
#include "libHD/HeapDescriptors.hpp"
#include <cmath>
#include <memory>
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
  concept String = std::same_as<std::remove_cv_t<std::remove_reference_t<T>>, std::string>;
  
  template <typename T>
  struct is_vector : std::false_type {};
  
  template <typename T>
  struct is_vector<std::vector<T>> : std::true_type {};
  
  template <typename T>
  concept List = is_vector<T>::value;
  
  template <typename T>
  struct is_map : std::false_type {};
  
  template <typename T, typename U>
  struct is_map<std::unordered_map<T, U>> : std::true_type {};
  
  template <typename T>
  concept Map = is_map<T>::value;
  
  template<typename T>
  concept BlueshiftType = Boolean<T> || Integer<T> || Float<T> || String<T> || List<T> || Map<T>;

  // primary template; retain type
  template<BlueshiftType T>
  struct Convert { using type = T; };

  // partial specialization for integer types; convert to int64_t
  template<Integer T>
  struct Convert<T> { using type = int64_t; };

  // partial specialization for floating point types; convert to double
  template<Float T>
  struct Convert<T> { using type = double; };

  // partial specialization for vector<T>; convert T to Tp and hence vector<T> to vector<Tp>
  template<typename T>
  struct Convert<std::vector<T>> {
      using Tp = Convert<T>::type;
      using type = std::vector<Tp>;
  };

  // partial specialization for unordered_map<K, V>; convert K to Kp and V to Vp and hence unordered_map<K, V> to unordered_map<Kp, Vp>
  template<typename K, typename V>
  struct Convert<std::unordered_map<K, V>> {
    using Kp = Convert<K>::type;
    using Vp = Convert<V>::type;
    using type = std::unordered_map<Kp, Vp>;
  };

  // alias template for convenience
  template<typename T>
  using converted_t = Convert<T>::type;

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

  namespace TypeDef {
    /* string aliases */
    using string = std::string;

    /* vector aliases */
    template<typename T>
    using list = std::vector<T>;
    template<typename T>
    using vector = std::vector<T>;

    /* map aliases */
    template<typename K, typename V>
    using map = std::unordered_map<K, V>;
    template<typename K, typename V>
    using unordered_map = std::unordered_map<K, V>;
  
    #define DEVTYPE_BEGIN(name) \
    struct name { 
    #define ATTRIBUTE(name, type...) \
      using name##_t = converted_t<type>; \
      name##_t name;
    #define DEVTYPE_END \
    };
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END

  }

  enum class TYPE {
      void_t
    , bool_t
    , int_t
    , float_t
    , string_t

    #define CONTAINER_BEGIN(name, _) \
    , name##_t
    #define METHOD(...)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END

    #define DEVTYPE_BEGIN(name) \
    , name
    #define ATTRIBUTE(...)
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
    #define METHOD(...)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
  };

  inline constexpr TYPE getTypeEnum(const std::string& type) {
    if (type == "void") { return TYPE::void_t; }
    if (type == "bool") { return TYPE::bool_t; }
    if (type == "int") { return TYPE::int_t; }
    if (type == "float") { return TYPE::float_t; }
    if (type == "string") { return TYPE::string_t; }
    #define CONTAINER_BEGIN(name, _) \
    if (type == #name) { return TYPE::name##_t; }
    #define METHOD(...)
    #define CONTAINER_END
    #include "CONTAINER_TYPES.LIST"
    #undef CONTAINER_BEGIN
    #undef METHOD
    #undef CONTAINER_END
    #define DEVTYPE_BEGIN(name) \
    if (type == #name) { return TYPE::name; }
    #define ATTRIBUTE(...)
    #define DEVTYPE_END 
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    return TYPE::COUNT;
  }

  struct TypeIdentifier {
    TYPE name;
    std::vector<TypeIdentifier> args;
  };

  using BlsType = std::variant<
      std::monostate
    , bool
    , int
    , float
    , std::string
    , std::shared_ptr<HeapDescriptor>
  >;

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
        if constexpr (Numeric<decltype(b)>) {
          if (b == 0) {
            throw std::runtime_error("Error: dividing by zero.");
          }
        }
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
        if (b == 0) {
          throw std::runtime_error("Error: dividing by zero.");
        }
        return a % b;
      }
      else if constexpr (Numeric<decltype(a)> && Numeric<decltype(b)>) {
        if (b == 0) {
          throw std::runtime_error("Error: dividing by zero.");
        }
        return (float)std::fmod(a, b);
      }
      else {
        throw std::runtime_error("Lhs and Rhs must be integer or float types.");
      }
    }, lhs, rhs);
  }

  inline BlsType operator^(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
      if constexpr (Numeric<decltype(a)> && Numeric<decltype(b)>) {
        return (float)std::pow(a, b);
      }
      else {
        throw std::runtime_error("Lhs and Rhs must be integer or float types.");
      }
    }, lhs, rhs);
  }

}