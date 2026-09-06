#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/conditional_effect_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct Serializer<formalism::planning::ActionView<T>>
{
    static std::string name() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "Action"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::ActionView<T>& value)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("name", value.get_name());
            ar.field("original_name", value.get_original_name());
            ar.field("original_arity", value.get_original_arity());
            ar.field("variables", value.get_variables());
        }
        else
        {
            ar.field("binding", value.get_row());
        }
        ar.field("condition", value.get_condition());
        ar.field("effects", value.get_effects());
    }
};

}

#endif
