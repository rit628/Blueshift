#pragma once
#include <concepts>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace TypeDef {

    template<typename T>
    concept Void = std::same_as<T, void> || std::same_as<T, std::monostate>;

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
    concept BlueshiftBuiltin = Void<T> || Boolean<T> || Integer<T> || Float<T> || String<T> || List<T> || Map<T>;

    // primary template; retain type
    template<BlueshiftBuiltin T>
    struct Convert { using type = T; };

    // partial specialization for void types; convert to std::monostate
    template<Void T>
    struct Convert<T> { using type = std::monostate; };

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
    
    #define DEVTYPE_BEGIN(name, ...) \
    struct name {
    #define ATTRIBUTE(name, type...) \
        using name##_t = converted_t<type>; \
        name##_t name = {};
    #define DEVTYPE_END \
    };
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END

    template<typename T, typename... U>
    concept OneOf = (std::same_as<T, U> || ...);

    template<typename T>
    concept DEVTYPE = OneOf<T
    #define DEVTYPE_BEGIN(name, ...) \
    , name
    #define ATTRIBUTE(...)
    #define DEVTYPE_END
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    >;

    template<typename T>
    concept BlueshiftType = BlueshiftBuiltin<T> || DEVTYPE<T>;

}