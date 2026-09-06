#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<formalism::planning::ConditionalEffectView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "ConditionalEffect"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::ConditionalEffectView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("variables", value.get_variables());
        }
        ar.field("condition", value.get_condition());
        ar.field("effect", value.get_effect());
    });
}

}

#endif
