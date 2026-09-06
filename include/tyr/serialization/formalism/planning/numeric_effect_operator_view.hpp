#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct Serializer<::tyr::formalism::planning::NumericEffectOperatorView<T, F>>
{
    static std::string name() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "NumericEffectOperator"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::NumericEffectOperatorView<T, F>& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
