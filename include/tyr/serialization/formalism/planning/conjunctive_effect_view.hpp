#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct Serializer<formalism::planning::ConjunctiveEffectView<T>>
{
    static std::string name() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "ConjunctiveEffect"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::ConjunctiveEffectView<T>& value)
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
    }
};

}

#endif
