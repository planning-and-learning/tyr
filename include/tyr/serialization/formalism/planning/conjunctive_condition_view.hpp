#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/boolean_operator_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ConjunctiveConditionView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    ar.field(std::same_as<T, ::tyr::GroundTag> ? "static_ground_literals" : "static_literals", [](const auto& value) -> decltype(auto) { return (value.template get_literals<::tyr::formalism::StaticTag>()); });
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("fluent_literals", [](const auto& value) -> decltype(auto) { return (value.template get_literals<::tyr::formalism::FluentTag>()); });
    }
    ar.field(std::same_as<T, ::tyr::GroundTag> ? "derived_ground_literals" : "derived_literals", [](const auto& value) -> decltype(auto) { return (value.template get_literals<::tyr::formalism::DerivedTag>()); });
    if constexpr (std::same_as<T, ::tyr::GroundTag>)
    {
        ar.field("positive_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::PositiveTag>()); });
        ar.field("negative_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::NegativeTag>()); });
    }
    ar.field("numeric_constraints", [](const auto& value) -> decltype(auto) { return (value.get_numeric_constraints()); });
}

}

#endif
