#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/boolean_operator_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::ConjunctiveConditionView>
{
    static std::string name() { return "ConjunctiveCondition"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::ConjunctiveConditionView& value)
    {
        ar.field("variables", value.get_variables());
        ar.field("static_literals", value.get_literals<::tyr::formalism::StaticTag>());
        ar.field("fluent_literals", value.get_literals<::tyr::formalism::FluentTag>());
        ar.field("derived_literals", value.get_literals<::tyr::formalism::DerivedTag>());
        ar.field("numeric_constraints", value.get_numeric_constraints());
    }
};

}

#endif
