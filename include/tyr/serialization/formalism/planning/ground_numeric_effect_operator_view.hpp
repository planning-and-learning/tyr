#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/ground_numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/ground_numeric_effect_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::GroundNumericEffectOperatorView<T>>
{
    static std::string name() { return std::string(T::name) + "GroundNumericEffectOperator"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundNumericEffectOperatorView<T>& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
