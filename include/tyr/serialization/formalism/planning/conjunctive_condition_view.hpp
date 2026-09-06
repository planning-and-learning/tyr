#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/boolean_operator_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<formalism::planning::ConjunctiveConditionView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "ConjunctiveCondition"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::ConjunctiveConditionView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("variables", value.get_variables());
        }
        ar.field("static_literals", value.template get_literals<formalism::StaticTag>());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("fluent_literals", value.template get_literals<formalism::FluentTag>());
        }
        ar.field("derived_literals", value.template get_literals<formalism::DerivedTag>());
        if constexpr (std::same_as<T, GroundTag>)
        {
            ar.field("positive_facts", value.template get_facts<formalism::PositiveTag>());
            ar.field("negative_facts", value.template get_facts<formalism::NegativeTag>());
        }
        ar.field("numeric_constraints", value.get_numeric_constraints());
    });
}

}

#endif
