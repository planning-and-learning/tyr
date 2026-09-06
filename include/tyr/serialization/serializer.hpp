#ifndef TYR_SERIALIZATION_SERIALIZER_HPP_
#define TYR_SERIALIZATION_SERIALIZER_HPP_

#include <concepts>
#include <string>
#include <type_traits>

namespace tyr::serialization
{

template<typename T>
struct Serializer;

template<typename T>
struct PrimitiveSerializer;

template<typename T>
concept PrimitiveSerializable = requires(const T& value) { PrimitiveSerializer<T>::name(); PrimitiveSerializer<T>::value(value); };

template<typename T>
concept Serializable = requires { { Serializer<std::remove_cvref_t<T>>::name() } -> std::convertible_to<std::string>; };

template<typename T>
concept DictionaryValue = Serializable<T> &&
    (requires(const T& value) { value.get_handle(); value.get_context(); }
     || requires(const T& value) { value.get_state_repository(); value.identifying_members(); });

template<typename T>
std::string type_name()
{
    if constexpr (Serializable<T>)
        return Serializer<std::remove_cvref_t<T>>::name();
    else if constexpr (PrimitiveSerializable<T>)
        return PrimitiveSerializer<T>::name();
    else if constexpr (std::is_arithmetic_v<T>)
        return "constant";
    else
        static_assert(sizeof(T) == 0, "Missing serialization type name");
}

}

#endif
