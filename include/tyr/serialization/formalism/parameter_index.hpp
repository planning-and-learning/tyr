#ifndef TYR_SERIALIZATION_FORMALISM_PARAMETER_INDEX_HPP_
#define TYR_SERIALIZATION_FORMALISM_PARAMETER_INDEX_HPP_

#include "tyr/formalism/parameter_index.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::ParameterIndex>
{
    static std::string get() { return "ParameterIndex"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::ParameterIndex& value, Dictionaries*)
{
    result = ygg::uint_t(value);
}

}

#endif
