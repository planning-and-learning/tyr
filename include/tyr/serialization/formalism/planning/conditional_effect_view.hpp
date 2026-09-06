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

template<>
struct Serializer<::tyr::formalism::planning::ConditionalEffectView>
{
    static std::string name() { return "ConditionalEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::ConditionalEffectView& value)
    {
        ar.field("variables", value.get_variables());
        ar.field("condition", value.get_condition());
        ar.field("effect", value.get_effect());
    }
};

}

#endif
