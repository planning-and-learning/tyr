#ifndef TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_
#define TYR_SERIALIZATION_FORMALISM_ENUMS_HPP_

#include "tyr/formalism/enums.hpp"
#include "tyr/serialization/dictionaries.hpp"

namespace tyr::serialization
{

template<>
struct EnumTraits<::tyr::formalism::BooleanOperatorKind>
{
    static std::string name() { return "BooleanOperatorKind"; }
    static std::string label(::tyr::formalism::BooleanOperatorKind value) { return std::string(::tyr::formalism::to_string(value)); }
};

template<>
struct EnumTraits<::tyr::formalism::ArithmeticOperatorKind>
{
    static std::string name() { return "ArithmeticOperatorKind"; }
    static std::string label(::tyr::formalism::ArithmeticOperatorKind value) { return std::string(::tyr::formalism::to_string(value)); }
};

template<>
struct EnumTraits<::tyr::formalism::NumericEffectOperatorKind>
{
    static std::string name() { return "NumericEffectOperatorKind"; }
    static std::string label(::tyr::formalism::NumericEffectOperatorKind value) { return std::string(::tyr::formalism::to_string(value)); }
};

template<>
struct EnumTraits<::tyr::formalism::OptimizationDirection>
{
    static std::string name() { return "OptimizationDirection"; }
    static std::string label(::tyr::formalism::OptimizationDirection value) { return std::string(::tyr::formalism::to_string(value)); }
};

}

#endif
