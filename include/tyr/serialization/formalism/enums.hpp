#ifndef TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_
#define TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_

#include "tyr/formalism/enums.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::BooleanOperatorKind& value, Dictionaries*)
{
    result = std::string(::tyr::formalism::to_string(value));
}

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::ArithmeticOperatorKind& value, Dictionaries*)
{
    result = std::string(::tyr::formalism::to_string(value));
}

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::NumericEffectOperatorKind& value, Dictionaries*)
{
    result = std::string(::tyr::formalism::to_string(value));
}

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::OptimizationDirection& value, Dictionaries*)
{
    result = std::string(::tyr::formalism::to_string(value));
}

}

#endif
