#ifndef TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_
#define TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_

#include "tyr/formalism/enums.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

#include <type_traits>

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::BooleanOperatorKind>
{
    static std::string get() { return "BooleanOperatorKind"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::BooleanOperatorKind& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<::tyr::formalism::BooleanOperatorKind>>(value);
    result = dictionaries->add_kind(TypeName<::tyr::formalism::BooleanOperatorKind>::get(), id, std::string(::tyr::formalism::to_string(value)));
}

template<>
struct TypeName<::tyr::formalism::ArithmeticOperatorKind>
{
    static std::string get() { return "ArithmeticOperatorKind"; }
};

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::ArithmeticOperatorKind& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<::tyr::formalism::ArithmeticOperatorKind>>(value);
    result = dictionaries->add_kind(TypeName<::tyr::formalism::ArithmeticOperatorKind>::get(), id, std::string(::tyr::formalism::to_string(value)));
}

template<>
struct TypeName<::tyr::formalism::NumericEffectOperatorKind>
{
    static std::string get() { return "NumericEffectOperatorKind"; }
};

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::NumericEffectOperatorKind& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<::tyr::formalism::NumericEffectOperatorKind>>(value);
    result = dictionaries->add_kind(TypeName<::tyr::formalism::NumericEffectOperatorKind>::get(), id, std::string(::tyr::formalism::to_string(value)));
}

template<>
struct TypeName<::tyr::formalism::OptimizationDirection>
{
    static std::string get() { return "OptimizationDirection"; }
};

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::OptimizationDirection& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<::tyr::formalism::OptimizationDirection>>(value);
    result = dictionaries->add_kind(TypeName<::tyr::formalism::OptimizationDirection>::get(), id, std::string(::tyr::formalism::to_string(value)));
}

}

#endif
