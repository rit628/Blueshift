#include "bls_types.hpp"
#include "typedefs.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <boost/functional/hash.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <vector>

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

template<typename T>
concept Numeric = TypeDef::Integer<T> || TypeDef::Float<T>;

template<typename T, typename U>
concept BinaryOperable = !(TypeDef::Boolean<T> || TypeDef::Boolean<U>);

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

BlsType::operator bool() const {
    return std::visit([](const auto& a) -> bool {
        if constexpr (TypeDef::Boolean<decltype(a)>) {
            return bool(a);
        }
        else {
            throw std::runtime_error("Operand does not have boolean type.");
        }
    }, *this);
}

BlsType operator-(const BlsType& operand) {
    return std::visit([](const auto& a) -> BlsType {
        if constexpr (Numeric<decltype(a)>) {
            return -a;
        }
        else {
            throw std::runtime_error("Operand of '-' must have integer or float type.");
        }
    }, operand);
}

bool operator<(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> bool {
        if constexpr (Comparable<decltype(a), decltype(b)>) {
            return a < b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not orderable.");
        }
    }, lhs, rhs);
}

bool operator<=(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> bool {
        if constexpr (Comparable<decltype(a), decltype(b)>) {
            return a <= b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not orderable.");
        }
    }, lhs, rhs);
}

bool operator>(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> bool {
        if constexpr (Comparable<decltype(a), decltype(b)>) {
            return a > b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not orderable.");
        }
    }, lhs, rhs);
}

bool operator>=(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> bool {
        if constexpr (Comparable<decltype(a), decltype(b)>) {
            return a >= b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not orderable.");
        }
    }, lhs, rhs);
}

bool operator!=(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> bool {
        if constexpr (WeaklyComparable<decltype(a), decltype(b)>) {
            return a != b;
        }
        else {
            return true; // objects are not the same; thus trivially not equal
        }
    }, lhs, rhs);
}

bool operator==(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> bool {
        if constexpr (WeaklyComparable<decltype(a), decltype(b)>) {
            return a == b;
        }
        else {
            return false; // objects are not the same; thus trivially not equal
        }
    }, lhs, rhs);
}

BlsType operator+(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (Addable<decltype(a), decltype(b)>) {
            return a + b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not addable.");
        }
    }, lhs, rhs);
}

BlsType operator-(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (Subtractable<decltype(a), decltype(b)>) {
            return a - b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not subtractable.");
        }
    }, lhs, rhs);
}

BlsType operator*(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (Multiplicable<decltype(a), decltype(b)>) {
            return a * b;
        }
        else {
            throw std::runtime_error("Lhs and Rhs are not multiplicable.");
        }
    }, lhs, rhs);
}

BlsType operator/(const BlsType& lhs, const BlsType& rhs) {
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

BlsType operator%(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (TypeDef::Integer<decltype(a)> && TypeDef::Integer<decltype(b)>) {
            if (b == 0) {
                throw std::runtime_error("Error: dividing by zero.");
            }
            return a % b;
        }
        else if constexpr (Numeric<decltype(a)> && Numeric<decltype(b)>) {
            if (b == 0) {
                throw std::runtime_error("Error: dividing by zero.");
            }
            return std::fmod(a, b);
        }
        else {
            throw std::runtime_error("Lhs and Rhs must be integer or float types.");
        }
    }, lhs, rhs);
}

BlsType operator^(const BlsType& lhs, const BlsType& rhs) {
    return std::visit([](const auto& a, const auto& b) -> BlsType {
        if constexpr (Numeric<decltype(a)> && Numeric<decltype(b)>) {
            return std::pow(a, b);
        }
        else {
            throw std::runtime_error("Lhs and Rhs must be integer or float types.");
        }
    }, lhs, rhs);
}

bool typeCompatible(const BlsType& lhs, const BlsType& rhs) {
    const static auto compareTypes = [](TYPE a, TYPE b)  {
        switch (a) {
            case TYPE::int_t:
            case TYPE::float_t:
                switch (b) {
                    case TYPE::int_t:
                    case TYPE::float_t:
                        return true;
                    break;

                    default:
                        return false;
                    break;
                }
            break;
            
            default:
                return a == b;
            break;
        }
    };

    return std::visit(overloads {
        [](const std::shared_ptr<HeapDescriptor>& a, const std::shared_ptr<HeapDescriptor>& b) {
            // container types (or devtypes) must match
            if (a->getType() != b->getType()) {
                return false;
            }
            // key types must match or be implicitly convertible
            else if (!compareTypes(a->getKey(), b->getKey())) {
                return false;
            }
            else if (!compareTypes(a->getCont(), b->getCont())) {
                return false;
            }

            BlsType aInner = a->getSampleElement(), bInner = b->getSampleElement();
            return typeCompatible(aInner, bInner);
        },
        [](const auto& a, const auto& b) {
            return compareTypes(getType(a), getType(b));
        }
    }, lhs, rhs);
}

std::ostream& operator<<(std::ostream& os, const BlsType& obj) {
    std::visit(overloads {
        [&](const std::shared_ptr<HeapDescriptor>& x) {
            switch (x->getType()) {
                case TYPE::list_t: {
                    auto list = std::dynamic_pointer_cast<VectorDescriptor>(x);
                    auto& internalVector = list->getVector();
                    os << "[";
                    if (list->getSize() > 0) {
                        os << internalVector.front();
                    }
                    for (auto&& element : boost::make_iterator_range(internalVector.begin() + 1, internalVector.end())) {
                        os << ", " << element;
                    }
                    os << "]";
                }
                break;
                case TYPE::map_t: {
                    auto map = std::dynamic_pointer_cast<MapDescriptor>(x);
                    auto& internalMap = map->getMap();
                    auto beginIt = internalMap.begin();
                    os << "{";
                    if (map->getSize() > 0) {
                        auto& [key, val] = *beginIt;
                        os << key << " : " << val;
                        beginIt++;
                    }
                    for (auto&& [key, val] : boost::make_iterator_range(beginIt, internalMap.end())) {
                        os << ", " << key << " : " << val;
                    }
                    os << "}";
                }
                break;
                default:
                break;
            }
        },
        [&](const bool& x) {
            os << ((x) ? "true" : "false");
        },
        [&](const auto& x) {
            os << x;
        }
    }, obj);
    return os;
}

size_t std::hash<BlsType>::operator()(const BlsType& obj) const {
    return std::visit(overloads {
        [](const std::shared_ptr<HeapDescriptor>& x) -> size_t {
            size_t seed;
            switch (x->getType()) {
                case TYPE::list_t:
                    boost::hash_combine(seed, std::dynamic_pointer_cast<VectorDescriptor>(x)->getVector());
                break;
                case TYPE::map_t:
                    boost::hash_combine(seed, std::dynamic_pointer_cast<MapDescriptor>(x)->getMap());
                break;
                default:
                    seed = 0;
                break;
            }
            return seed;
        },
        [](const std::monostate& x) -> size_t {
            return std::hash<std::monostate>{}(x);
        },
        [](const std::string& x) -> size_t {
            return std::hash<std::string>{}(x);
        },
        [](auto x) -> size_t {
            return std::hash<decltype(x)>{}(x);
        }
    }, obj);
}

MapDescriptor::MapDescriptor(TYPE contType) {
    this->objType = TYPE::map_t;
    this->keyType = TYPE::string_t;
    this->contType = contType;
    this->map = std::make_shared<std::unordered_map<std::string, BlsType>>(); 
}

MapDescriptor::MapDescriptor(TYPE objType, TYPE keyType, TYPE contType) {
    this->objType = objType;
    this->keyType = keyType;
    this->contType = contType;
    this->map = std::make_shared<std::unordered_map<std::string, BlsType>>(); 

    switch(this->objType){
        #define DEVTYPE_BEGIN(name) \
            case(TYPE::name) : { 
            #define ATTRIBUTE(varName, type) \
                this->map->emplace(#varName, BlsType(type())); 
            #define DEVTYPE_END \
                break; \
            }
            #include "DEVTYPES.LIST"
            #undef DEVTYPE_BEGIN
            #undef ATTRIBUTE
            #undef DEVTYPE_END
            default : {
                break; 
            }
    }
}

BlsType& MapDescriptor::access(BlsType &obj) {
    std::scoped_lock bob(mux); 

    std::string accessor = stringify(obj); 
    if(this->map->find(accessor) != this->map->end()) {
      return this->map->at(accessor); 
    }
    else{
      throw std::invalid_argument("Could not find the object of name: " + accessor); 
    } 
}

void MapDescriptor::emplace(BlsType& obj, BlsType& newDesc) {
    std::scoped_lock bob(mux); 
    std::string newKey = stringify(obj); 
    this->map->emplace(newKey, newDesc);
}

VectorDescriptor::VectorDescriptor(std::string cont_code) {
    this->objType = TYPE::list_t;
    this->contType = getTypeFromName(cont_code); 
    vector = std::make_shared<std::vector<BlsType>>(); 
}

VectorDescriptor::VectorDescriptor(TYPE contType) {
    this->objType = TYPE::list_t;
    this->contType = contType; 
    vector = std::make_shared<std::vector<BlsType>>(); 
}

BlsType& VectorDescriptor::access(BlsType &int_acc) {
    std::scoped_lock bob(mux); 

    if(std::holds_alternative<int64_t>(int_acc)){
      int index = std::get<int64_t>(int_acc); 
      return this->vector->at(index); 
    }
    else{
      throw std::runtime_error("Cannot index a list with a non-integer"); 
    }
}

void VectorDescriptor::append(BlsType& newObj){
    std::scoped_lock bob(mux);
    this->vector->push_back(newObj);
}

TYPE getType(const BlsType& obj) {
    return std::visit(overloads {
        [](std::monostate) { return TYPE::void_t; },
        [](bool) { return TYPE::bool_t; },
        [](int64_t) { return TYPE::int_t; },
        [](double) { return TYPE::float_t; },
        [](const std::string&) { return TYPE::string_t; },
        [](const std::shared_ptr<HeapDescriptor>& obj) { return obj->getType(); }
    }, obj);
}

std::string stringify(const BlsType& value) {
    return std::visit(overloads {
        [](bool value) { return std::to_string(value); },
        [](int64_t value) { return std::to_string(value); },
        [](double value) { return std::to_string(value); },
        [](const std::string& value) { return value; },
        [](const auto&) -> std::string { throw std::runtime_error("Heap Descriptor Stringification not implemented!"); }
    }, value);
}