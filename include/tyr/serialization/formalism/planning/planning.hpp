
#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/boolean_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/parameter_index.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, typename T>
void describe_fields(Archive& ar, std::type_identity<ygg::View<ygg::Index<::tyr::formalism::RelationBinding<T>>, ::tyr::formalism::planning::Repository>>)
{
    ar.field("relation", [](const auto& value) -> decltype(auto) { return (value.get_relation()); });
    ar.field("objects", [](const auto& value) -> decltype(auto) { return (value.get_objects()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ActionView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
        ar.field("original_name", [](const auto& value) -> decltype(auto) { return (value.get_original_name()); });
        ar.field("original_arity", [](const auto& value) -> decltype(auto) { return (value.get_original_arity()); });
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
    ar.field("condition", [](const auto& value) -> decltype(auto) { return (value.get_condition()); });
    ar.field("effects", [](const auto& value) -> decltype(auto) { return (value.get_effects()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ArithmeticOperatorView<T>>)
{
    ar.variant([](const auto& value) -> decltype(auto) { return (value.get_variant()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::AtomView<T, F>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("predicate", [](const auto& value) -> decltype(auto) { return (value.get_predicate()); });
        ar.field("terms", [](const auto& value) -> decltype(auto) { return (value.get_terms()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::AxiomView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
    ar.field("body", [](const auto& value) -> decltype(auto) { return (value.get_body()); });
    ar.field("head", [](const auto& value) -> decltype(auto) { return (value.get_head()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::BinaryOperatorKind O>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::BinaryOperatorView<T, O>>)
{
    ar.field("operator", [](const auto& value) -> decltype(auto) { return (value.get_operator()); });
    ar.field("lhs", [](const auto& value) -> decltype(auto) { return (value.get_lhs()); });
    ar.field("rhs", [](const auto& value) -> decltype(auto) { return (value.get_rhs()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::BooleanOperatorView<T>>)
{
    ar.variant([](const auto& value) -> decltype(auto) { return (value.get_variant()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ConditionalEffectView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    ar.field("condition", [](const auto& value) -> decltype(auto) { return (value.get_condition()); });
    ar.field("effect", [](const auto& value) -> decltype(auto) { return (value.get_effect()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ConjunctiveConditionView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    ar.field(std::same_as<T, ::tyr::GroundTag> ? "static_ground_literals" : "static_literals",
             [](const auto& value) -> decltype(auto) { return (value.template get_literals<::tyr::formalism::StaticTag>()); });
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("fluent_literals", [](const auto& value) -> decltype(auto) { return (value.template get_literals<::tyr::formalism::FluentTag>()); });
    }
    ar.field(std::same_as<T, ::tyr::GroundTag> ? "derived_ground_literals" : "derived_literals",
             [](const auto& value) -> decltype(auto) { return (value.template get_literals<::tyr::formalism::DerivedTag>()); });
    if constexpr (std::same_as<T, ::tyr::GroundTag>)
    {
        ar.field("positive_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::PositiveTag>()); });
        ar.field("negative_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::NegativeTag>()); });
    }
    ar.field("numeric_constraints", [](const auto& value) -> decltype(auto) { return (value.get_numeric_constraints()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ConjunctiveEffectView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("literals", [](const auto& value) -> decltype(auto) { return (value.get_literals()); });
    }
    else
    {
        ar.field("add_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::PositiveTag>()); });
        ar.field("delete_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::NegativeTag>()); });
    }
    ar.field("numeric_effects", [](const auto& value) -> decltype(auto) { return (value.get_numeric_effects()); });
    ar.field("auxiliary_numeric_effect", [](const auto& value) -> decltype(auto) { return (value.get_auxiliary_numeric_effect()); });
}

}

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::DomainView>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
    ar.field("static_predicates", [](const auto& value) -> decltype(auto) { return (value.template get_predicates<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_predicates", [](const auto& value) -> decltype(auto) { return (value.template get_predicates<::tyr::formalism::FluentTag>()); });
    ar.field("derived_predicates", [](const auto& value) -> decltype(auto) { return (value.template get_predicates<::tyr::formalism::DerivedTag>()); });
    ar.field("static_functions", [](const auto& value) -> decltype(auto) { return (value.template get_functions<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_functions", [](const auto& value) -> decltype(auto) { return (value.template get_functions<::tyr::formalism::FluentTag>()); });
    ar.field("auxiliary_function", [](const auto& value) -> decltype(auto) { return (value.get_auxiliary_function()); });
    ar.field("constants", [](const auto& value) -> decltype(auto) { return (value.get_constants()); });
    ar.field("actions", [](const auto& value) -> decltype(auto) { return (value.get_actions()); });
    ar.field("axioms", [](const auto& value) -> decltype(auto) { return (value.get_axioms()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::formalism::FactKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FDRFactView<T>>)
{
    ar.field("fdr_variable", [](const auto& value) -> decltype(auto) { return (value.get_variable()); });
    ar.field("value", [](const auto& value) -> decltype(auto) { return (ygg::uint_t(value.get_value())); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::formalism::FactKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FDRVariableView<T>>)
{
    ar.field("atoms", [](const auto& value) -> decltype(auto) { return (value.get_atoms()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FunctionExpressionView<T>>)
{
    ar.variant([](const auto& value) -> decltype(auto) { return (value.get_variant()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, F>>)
{
    ar.field("function_term", [](const auto& value) -> decltype(auto) { return (value.get_fterm()); });
    ar.field("value", [](const auto& value) -> decltype(auto) { return (value.get_value()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FunctionTermView<T, F>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("function", [](const auto& value) -> decltype(auto) { return (value.get_function()); });
        ar.field("terms", [](const auto& value) -> decltype(auto) { return (value.get_terms()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
}

}

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::InvariantView>)
{
    ar.field("num_rigid_variables", [](const auto& value) -> decltype(auto) { return (value.get_num_rigid_variables()); });
    ar.field("num_counted_variables", [](const auto& value) -> decltype(auto) { return (value.get_num_counted_variables()); });
    ar.field("atoms", [](const auto& value) -> decltype(auto) { return (value.get_atoms()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::LiteralView<T, F>>)
{
    ar.field("atom", [](const auto& value) -> decltype(auto) { return (value.get_atom()); });
    ar.field("polarity", [](const auto& value) -> decltype(auto) { return (value.get_polarity()); });
}

}

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::MetricView>)
{
    ar.field("optimization_direction", [](const auto& value) -> decltype(auto) { return (value.get_optimization_direction()); });
    ar.field("function_expression", [](const auto& value) -> decltype(auto) { return (value.get_fexpr()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::MultiOperatorView<T>>)
{
    ar.field("operator", [](const auto& value) -> decltype(auto) { return (value.get_operator()); });
    ar.field("args", [](const auto& value) -> decltype(auto) { return (value.get_args()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::NumericEffectOperatorView<T, F>>)
{
    ar.variant([](const auto& value) -> decltype(auto) { return (value.get_variant()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::NumericEffectView<T, F>>)
{
    ar.field("operator", [](const auto& value) -> decltype(auto) { return (value.get_operator()); });
    ar.field("function_term", [](const auto& value) -> decltype(auto) { return (value.get_fterm()); });
    ar.field("function_expression", [](const auto& value) -> decltype(auto) { return (value.get_fexpr()); });
}

}

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::PlanningDomain>)
{
    ar.field("domain", [](const auto& value) -> decltype(auto) { return (value.get_domain()); });
    ar.field("path", [](const auto& value) -> decltype(auto) { return (value.get_path()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::PlanningTask<T>>)
{
    ar.field("task", [](const auto& value) -> decltype(auto) { return (value.get_task()); });
    ar.field("domain", [](const auto& value) -> decltype(auto) { return (value.get_domain()); });
    ar.field("path", [](const auto& value) -> decltype(auto) { return (value.get_path()); });
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::TaskView<T>>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
    ar.field("domain", [](const auto& value) -> decltype(auto) { return (value.get_domain()); });
    ar.field("derived_predicates", [](const auto& value) -> decltype(auto) { return (value.get_derived_predicates()); });
    ar.field("objects", [](const auto& value) -> decltype(auto) { return (value.get_objects()); });
    ar.field("static_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.template get_atoms<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.template get_atoms<::tyr::formalism::FluentTag>()); });
    if constexpr (std::same_as<T, ::tyr::GroundTag>)
        ar.field("derived_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.template get_atoms<::tyr::formalism::DerivedTag>()); });
    ar.field("static_ground_function_term_values",
             [](const auto& value) -> decltype(auto) { return (value.template get_fterm_values<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_ground_function_term_values",
             [](const auto& value) -> decltype(auto) { return (value.template get_fterm_values<::tyr::formalism::FluentTag>()); });
    ar.field("auxiliary_ground_function_term_value", [](const auto& value) -> decltype(auto) { return (value.get_auxiliary_fterm_value()); });
    ar.field("goal", [](const auto& value) -> decltype(auto) { return (value.get_goal()); });
    ar.field("metric", [](const auto& value) -> decltype(auto) { return (value.get_metric()); });
    ar.field("axioms", [](const auto& value) -> decltype(auto) { return (value.get_axioms()); });
    if constexpr (std::same_as<T, ::tyr::GroundTag>)
    {
        ar.field("fdr_variables", [](const auto& value) -> decltype(auto) { return (value.get_fluent_variables()); });
        ar.field("fdr_facts", [](const auto& value) -> decltype(auto) { return (value.get_fluent_facts()); });
        ar.field("ground_actions", [](const auto& value) -> decltype(auto) { return (value.get_ground_actions()); });
        ar.field("ground_axioms", [](const auto& value) -> decltype(auto) { return (value.get_ground_axioms()); });
    }
}

}

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::UnaryOperatorView<T>>)
{
    ar.field("operator", [](const auto& value) -> decltype(auto) { return (value.get_operator()); });
    ar.field("arg", [](const auto& value) -> decltype(auto) { return (value.get_arg()); });
}

}

#endif
