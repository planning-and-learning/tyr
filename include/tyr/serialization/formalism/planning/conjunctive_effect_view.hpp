#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_operator_view.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<formalism::planning::ConjunctiveEffectView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "ConjunctiveEffect"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::ConjunctiveEffectView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("literals", value.get_literals());
        }
        else
        {
            ar.field("add_facts", value.template get_facts<formalism::PositiveTag>());
            ar.field("del_facts", value.template get_facts<formalism::NegativeTag>());
        }
        ar.field("numeric_effects", value.get_numeric_effects());
        ar.field("auxiliary_numeric_effect", value.get_auxiliary_numeric_effect());
    });
}

}

#endif
