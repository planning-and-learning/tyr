#ifndef TYR_SERIALIZATION_CONVERSION_HPP_
#define TYR_SERIALIZATION_CONVERSION_HPP_

#include <boost/json.hpp>
#include <cista/containers/string.h>
#include <ranges>
#include <string>
#include <type_traits>
#include <yggdrasil/containers/optional.hpp>

namespace boost::json
{

template<>
struct is_string_like<cista::offset::string> : std::true_type {};

// Read-only optional views have no reset(), which Boost's automatic detection requires.
template<typename T, typename C>
struct is_optional_like<ygg::View<cista::optional<T>, C>> : std::true_type {};

}

namespace tyr::serialization
{

class Dictionaries;

// Boost's sequence conversion requires legacy iterator traits, which some C++20 input views lack.
template<std::ranges::view T>
    requires std::ranges::input_range<const T> && (!boost::json::is_sequence_like<T>::value)
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const T& value, Dictionaries* dictionaries)
{
    auto& array = result.emplace_array();
    for (const auto& item : value)
        array.push_back(boost::json::value_from(item, dictionaries, array.storage()));
}

template<typename T>
struct TypeName;

}

#endif
