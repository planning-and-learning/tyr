#ifndef TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_
#define TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_

#include "tyr/formalism/enums.hpp"
#include "tyr/serialization/dictionaries.hpp"

#include <type_traits>

namespace tyr::serialization
{

template<>
struct TypeName<formalism::BooleanOperatorKind>
{
    static std::string get() { return "BooleanOperatorKind"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::BooleanOperatorKind& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<formalism::BooleanOperatorKind>>(value);
    dictionaries->add_kind(TypeName<formalism::BooleanOperatorKind>::get(), id, std::string(formalism::to_string(value)));
    result = id;
}

template<>
struct TypeName<formalism::ArithmeticOperatorKind>
{
    static std::string get() { return "ArithmeticOperatorKind"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::ArithmeticOperatorKind& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<formalism::ArithmeticOperatorKind>>(value);
    dictionaries->add_kind(TypeName<formalism::ArithmeticOperatorKind>::get(), id, std::string(formalism::to_string(value)));
    result = id;
}

template<>
struct TypeName<formalism::NumericEffectOperatorKind>
{
    static std::string get() { return "NumericEffectOperatorKind"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::NumericEffectOperatorKind& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<formalism::NumericEffectOperatorKind>>(value);
    dictionaries->add_kind(TypeName<formalism::NumericEffectOperatorKind>::get(), id, std::string(formalism::to_string(value)));
    result = id;
}

template<>
struct TypeName<formalism::OptimizationDirection>
{
    static std::string get() { return "OptimizationDirection"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::OptimizationDirection& value, Dictionaries* dictionaries)
{
    const auto id = static_cast<std::underlying_type_t<formalism::OptimizationDirection>>(value);
    dictionaries->add_kind(TypeName<formalism::OptimizationDirection>::get(), id, std::string(formalism::to_string(value)));
    result = id;
}

}

#endif
