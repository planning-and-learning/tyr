#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/conditional_effect_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::ActionView>
{
    static std::string name() { return "Action"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::ActionView& value)
    {
        ar.field("name", value.get_name());
        ar.field("original_name", value.get_original_name());
        ar.field("original_arity", value.get_original_arity());
        ar.field("variables", value.get_variables());
        ar.field("condition", value.get_condition());
        ar.field("effects", value.get_effects());
    }
};

}

#endif
