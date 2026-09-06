#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_CONDITIONAL_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_CONDITIONAL_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/ground_conditional_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/ground_conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/ground_conjunctive_effect_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::GroundConditionalEffectView>
{
    static std::string name() { return "GroundConditionalEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundConditionalEffectView& value)
    {
        ar.field("condition", value.get_condition());
        ar.field("effect", value.get_effect());
    }
};

}

#endif
