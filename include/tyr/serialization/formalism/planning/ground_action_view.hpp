#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_ACTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_ACTION_VIEW_HPP_

#include "tyr/formalism/planning/ground_action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/ground_conditional_effect_view.hpp"
#include "tyr/serialization/formalism/planning/ground_conjunctive_condition_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::GroundActionView>
{
    static std::string name() { return "GroundAction"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundActionView& value)
    {
        ar.field("binding", value.get_row());
        ar.field("condition", value.get_condition());
        ar.field("effects", value.get_effects());
    }
};

}

#endif
