#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/ground_conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/ground_literal_view.hpp"
#include "tyr/serialization/formalism/planning/ground_numeric_effect_operator_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::GroundConjunctiveEffectView>
{
    static std::string name() { return "GroundConjunctiveEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundConjunctiveEffectView& value)
    {
        ar.field("add_facts", value.get_facts<::tyr::formalism::PositiveTag>());
        ar.field("del_facts", value.get_facts<::tyr::formalism::NegativeTag>());
        ar.field("numeric_effects", value.get_numeric_effects());
        ar.field("auxiliary_numeric_effect", value.get_auxiliary_numeric_effect());
    }
};

}

#endif
