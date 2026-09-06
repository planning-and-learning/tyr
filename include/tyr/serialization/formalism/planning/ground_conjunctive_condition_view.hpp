#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/planning/ground_conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/boolean_operator_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/ground_literal_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::GroundConjunctiveConditionView>
{
    static std::string name() { return "GroundConjunctiveCondition"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundConjunctiveConditionView& value)
    {
        ar.field("static_literals", value.get_literals<::tyr::formalism::StaticTag>());
        ar.field("derived_literals", value.get_literals<::tyr::formalism::DerivedTag>());
        ar.field("positive_facts", value.get_facts<::tyr::formalism::PositiveTag>());
        ar.field("negative_facts", value.get_facts<::tyr::formalism::NegativeTag>());
        ar.field("numeric_constraints", value.get_numeric_constraints());
    }
};

}

#endif
