#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_TASK_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_TASK_VIEW_HPP_

#include "tyr/formalism/planning/fdr_task_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/domain_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_variable_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_value_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "tyr/serialization/formalism/planning/metric_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FDRTaskView>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
    ar.field("domain", [](const auto& value) -> decltype(auto) { return (value.get_domain()); });
    ar.field("derived_predicates", [](const auto& value) -> decltype(auto) { return (value.get_derived_predicates()); });
    ar.field("objects", [](const auto& value) -> decltype(auto) { return (value.get_objects()); });
    ar.field("static_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.template get_atoms<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.template get_atoms<::tyr::formalism::FluentTag>()); });
    ar.field("derived_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.template get_atoms<::tyr::formalism::DerivedTag>()); });
    ar.field("static_ground_function_term_values", [](const auto& value) -> decltype(auto) { return (value.template get_fterm_values<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_ground_function_term_values", [](const auto& value) -> decltype(auto) { return (value.template get_fterm_values<::tyr::formalism::FluentTag>()); });
    ar.field("auxiliary_ground_function_term_value", [](const auto& value) -> decltype(auto) { return (value.get_auxiliary_fterm_value()); });
    ar.field("goal", [](const auto& value) -> decltype(auto) { return (value.get_goal()); });
    ar.field("metric", [](const auto& value) -> decltype(auto) { return (value.get_metric()); });
    ar.field("axioms", [](const auto& value) -> decltype(auto) { return (value.get_axioms()); });
    ar.field("fdr_variables", [](const auto& value) -> decltype(auto) { return (value.get_fluent_variables()); });
    ar.field("fdr_facts", [](const auto& value) -> decltype(auto) { return (value.get_fluent_facts()); });
    ar.field("ground_actions", [](const auto& value) -> decltype(auto) { return (value.get_ground_actions()); });
    ar.field("ground_axioms", [](const auto& value) -> decltype(auto) { return (value.get_ground_axioms()); });
}

}

#endif
