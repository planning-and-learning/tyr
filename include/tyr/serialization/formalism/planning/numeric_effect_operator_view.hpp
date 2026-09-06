#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::NumericEffectOperatorView<T>>
{
    static std::string name() { return std::string(T::name) + "NumericEffectOperator"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::NumericEffectOperatorView<T>& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
