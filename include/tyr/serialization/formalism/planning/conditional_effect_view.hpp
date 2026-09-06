#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::TaskKind T>
struct Serializer<::tyr::formalism::planning::ConditionalEffectView<T>>
{
    static std::string name() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "ConditionalEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::ConditionalEffectView<T>& value)
    {
        if constexpr (std::same_as<T, ::tyr::LiftedTag>)
        {
            ar.field("variables", value.get_variables());
        }
        ar.field("condition", value.get_condition());
        ar.field("effect", value.get_effect());
    }
};

}

#endif
