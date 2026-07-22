/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "views.hpp"

#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>
namespace tyr::formalism::planning
{

namespace
{

void bind_object(nb::module_& m, const std::string& name)
{
    using V = ObjectView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_variable(nb::module_& m, const std::string& name)
{
    using V = VariableView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_term(nb::module_& m, const std::string& name)
{
    using V = TermView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_relation_binding(nb::module_& m, const std::string& name)
{
    using V = ygg::View<ygg::Index<RelationBinding<T>>, Repository>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_relation", &V::get_relation, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_predicate(nb::module_& m, const std::string& name)
{
    using V = PredicateView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_arity", &V::get_arity);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_atom(nb::module_& m, const std::string& name)
{
    using V = AtomView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_predicate", &V::get_predicate, nb::keep_alive<0, 1>())
                   .def("get_terms", &V::get_terms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_atom(nb::module_& m, const std::string& name)
{
    using V = GroundAtomView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_predicate", &V::get_predicate, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_literal(nb::module_& m, const std::string& name)
{
    using V = LiteralView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_atom", &V::get_atom, nb::keep_alive<0, 1>())
                   .def("get_polarity", &V::get_polarity);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_literal(nb::module_& m, const std::string& name)
{
    using V = GroundLiteralView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_atom", &V::get_atom, nb::keep_alive<0, 1>())
                   .def("get_polarity", &V::get_polarity);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_fdr_variable(nb::module_& m, const std::string& name)
{
    using V = FDRVariableView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_domain_size", &V::get_domain_size)
                   .def("get_atoms", &V::get_atoms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_fdr_fact(nb::module_& m, const std::string& name)
{
    using V = FDRFactView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variable", &V::get_variable, nb::keep_alive<0, 1>())
                   .def("get_value", &V::get_value)
                   .def("has_value", &V::has_value)
                   .def("get_atom", &V::get_atom, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_function(nb::module_& m, const std::string& name)
{
    using V = FunctionView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_arity", &V::get_arity);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_function_term(nb::module_& m, const std::string& name)
{
    using V = FunctionTermView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_function", &V::get_function, nb::keep_alive<0, 1>())
                   .def("get_terms", &V::get_terms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term(nb::module_& m, const std::string& name)
{
    using V = GroundFunctionTermView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_function", &V::get_function, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term_value(nb::module_& m, const std::string& name)
{
    using V = GroundFunctionTermValueView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_value", &V::get_value);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_numeric_effect(nb::module_& m, const std::string& name)
{
    using V = NumericEffectView<Op, T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_ground_numeric_effect(nb::module_& m, const std::string& name)
{
    using V = GroundNumericEffectView<Op, T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_numeric_effect_operator(nb::module_& m, const std::string& name)
{
    using V = NumericEffectOperatorView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_numeric_effect_operator(nb::module_& m, const std::string& name)
{
    using V = GroundNumericEffectOperatorView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_function_expression(nb::module_& m, const std::string& name)
{
    using V = FunctionExpressionView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_condition(nb::module_& m, const std::string& name)
{
    using V = ConjunctiveConditionView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_variables", &V::get_variables)
                   .def("get_arity", &V::get_arity)
                   .def("get_static_literals", &V::get_literals<StaticTag>)
                   .def("get_fluent_literals", &V::get_literals<FluentTag>)
                   .def("get_derived_literals", &V::get_literals<DerivedTag>)
                   .def("get_numeric_constraints", &V::get_numeric_constraints);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_effect(nb::module_& m, const std::string& name)
{
    using V = ConjunctiveEffectView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_literals", &V::get_literals)
                   .def("get_numeric_effects", &V::get_numeric_effects)
                   .def("get_auxiliary_numeric_effect", &V::get_auxiliary_numeric_effect);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conditional_effect(nb::module_& m, const std::string& name)
{
    using V = ConditionalEffectView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_variables", &V::get_variables)
                   .def("get_arity", &V::get_arity)
                   .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                   .def("get_effect", &V::get_effect, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_action(nb::module_& m, const std::string& name)
{
    using V = ActionView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_original_name", &V::get_original_name)
                   .def("get_original_arity", &V::get_original_arity)
                   .def("get_arity", &V::get_arity)
                   .def("get_variables", &V::get_variables)
                   .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                   .def("get_effects", &V::get_effects);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_axiom(nb::module_& m, const std::string& name)
{
    using V = AxiomView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_arity", &V::get_arity)
                   .def("get_variables", &V::get_variables)
                   .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                   .def("get_head", &V::get_head, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_function_expression(nb::module_& m, const std::string& name)
{
    using V = GroundFunctionExpressionView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conjunctive_condition(nb::module_& m, const std::string& name)
{
    using V = GroundConjunctiveConditionView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_static_facts", &V::get_literals<StaticTag>)
                   .def("get_derived_facts", &V::get_literals<DerivedTag>)
                   .def("get_positive_facts", &V::get_facts<PositiveTag>)
                   .def("get_negative_facts", &V::get_facts<NegativeTag>)
                   .def("get_numeric_constraints", &V::get_numeric_constraints);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conjunctive_effect(nb::module_& m, const std::string& name)
{
    using V = GroundConjunctiveEffectView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_add_facts", &V::get_facts<PositiveTag>)
                   .def("get_del_facts", &V::get_facts<NegativeTag>)
                   .def("get_numeric_effects", &V::get_numeric_effects)
                   .def("get_auxiliary_numeric_effect", &V::get_auxiliary_numeric_effect);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conditional_effect(nb::module_& m, const std::string& name)
{
    using V = GroundConditionalEffectView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                   .def("get_effect", &V::get_effect, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_action(nb::module_& m, const std::string& name)
{
    using V = GroundActionView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_action", &V::get_action, nb::keep_alive<0, 1>())
                   .def("get_row", &V::get_row, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key)
                   .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                   .def("get_effects", &V::get_effects);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_axiom(nb::module_& m, const std::string& name)
{
    using V = GroundAxiomView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_axiom", &V::get_axiom, nb::keep_alive<0, 1>())
                   .def("get_row", &V::get_row, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key)
                   .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                   .def("get_head", &V::get_head, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_metric(nb::module_& m, const std::string& name)
{
    using V = MetricView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_objective", &V::get_objective)
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_domain(nb::module_& m, const std::string& name)
{
    using V = DomainView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_static_predicates", &V::get_predicates<StaticTag>)
                   .def("get_fluent_predicates", &V::get_predicates<FluentTag>)
                   .def("get_derived_predicates", &V::get_predicates<DerivedTag>)
                   .def("get_static_functions", &V::get_functions<StaticTag>)
                   .def("get_fluent_functions", &V::get_functions<FluentTag>)
                   .def("get_auxiliary_function", &V::get_auxiliary_function)
                   .def("get_constants", &V::get_constants)
                   .def("get_actions", &V::get_actions)
                   .def("get_axioms", &V::get_axioms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_lifted_task(nb::module_& m, const std::string& name)
{
    using V = TaskView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_domain", &V::get_domain, nb::keep_alive<0, 1>())
                   .def("get_derived_predicates", &V::get_derived_predicates)
                   .def("get_objects", &V::get_objects)
                   .def("get_static_atoms", &V::get_atoms<StaticTag>)
                   .def("get_fluent_atoms", &V::get_atoms<FluentTag>)
                   .def("get_static_fterm_values", &V::get_fterm_values<StaticTag>)
                   .def("get_fluent_fterm_values", &V::get_fterm_values<FluentTag>)
                   .def("get_auxiliary_fterm_value", &V::get_auxiliary_fterm_value)
                   .def("get_goal", &V::get_goal, nb::keep_alive<0, 1>())
                   .def("get_metric", &V::get_metric)
                   .def("get_axioms", &V::get_axioms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_task(nb::module_& m, const std::string& name)
{
    using V = FDRTaskView;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_domain", &V::get_domain, nb::keep_alive<0, 1>())
                   .def("get_derived_predicates", &V::get_derived_predicates)
                   .def("get_objects", &V::get_objects)
                   .def("get_static_atoms", &V::get_atoms<StaticTag>)
                   .def("get_fluent_atoms", &V::get_atoms<FluentTag>)
                   .def("get_derived_atoms", &V::get_atoms<DerivedTag>)
                   .def("get_static_fterm_values", &V::get_fterm_values<StaticTag>)
                   .def("get_fluent_fterm_values", &V::get_fterm_values<FluentTag>)
                   .def("get_auxiliary_fterm_value", &V::get_auxiliary_fterm_value)
                   .def("get_goal", &V::get_goal, nb::keep_alive<0, 1>())
                   .def("get_metric", &V::get_metric)
                   .def("get_axioms", &V::get_axioms)
                   .def("get_fluent_variables", &V::get_fluent_variables)
                   .def("get_fluent_facts", &V::get_fluent_facts)
                   .def("get_ground_actions", &V::get_ground_actions)
                   .def("get_ground_axioms", &V::get_ground_axioms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_unary_operator(nb::module_& m, const std::string& name)
{
    using V = UnaryOperatorView<Op, T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_arg", &V::get_arg);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_binary_operator(nb::module_& m, const std::string& name)
{
    using V = BinaryOperatorView<Op, T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_lhs", &V::get_lhs)
                   .def("get_rhs", &V::get_rhs);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_multi_operator(nb::module_& m, const std::string& name)
{
    using V = MultiOperatorView<Op, T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_args", &V::get_args);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_arithmetic_operator(nb::module_& m, const std::string& name)
{
    using V = ArithmeticOperatorView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_boolean_operator(nb::module_& m, const std::string& name)
{
    using V = BooleanOperatorView<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}

/**
 * bind_views
 */

void bind_views(nb::module_& m)
{
    bind_object(m, "Object");
    bind_variable(m, "Variable");
    bind_term(m, "Term");

    bind_relation_binding<Predicate<StaticTag>>(m, "StaticPredicateBinding");
    bind_relation_binding<Predicate<FluentTag>>(m, "FluentPredicateBinding");
    bind_relation_binding<Predicate<DerivedTag>>(m, "DerivedPredicateBinding");
    bind_relation_binding<Function<StaticTag>>(m, "StaticFunctionBinding");
    bind_relation_binding<Function<FluentTag>>(m, "FluentFunctionBinding");
    bind_relation_binding<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBinding");
    bind_relation_binding<Action>(m, "ActionBinding");
    bind_relation_binding<Axiom>(m, "AxiomBinding");

    bind_predicate<StaticTag>(m, "StaticPredicate");
    bind_predicate<FluentTag>(m, "FluentPredicate");
    bind_predicate<DerivedTag>(m, "DerivedPredicate");

    bind_atom<StaticTag>(m, "StaticAtom");
    bind_atom<FluentTag>(m, "FluentAtom");
    bind_atom<DerivedTag>(m, "DerivedAtom");

    bind_ground_atom<StaticTag>(m, "StaticGroundAtom");
    bind_ground_atom<FluentTag>(m, "FluentGroundAtom");
    bind_ground_atom<DerivedTag>(m, "DerivedGroundAtom");

    bind_literal<StaticTag>(m, "StaticLiteral");
    bind_literal<FluentTag>(m, "FluentLiteral");
    bind_literal<DerivedTag>(m, "DerivedLiteral");

    bind_ground_literal<StaticTag>(m, "StaticGroundLiteral");
    bind_ground_literal<FluentTag>(m, "FluentGroundLiteral");
    bind_ground_literal<DerivedTag>(m, "DerivedGroundLiteral");

    bind_fdr_variable<FluentTag>(m, "FluentFDRVariable");
    bind_fdr_fact<FluentTag>(m, "FluentFDRFact");

    bind_function<StaticTag>(m, "StaticFunction");
    bind_function<FluentTag>(m, "FluentFunction");
    bind_function<AuxiliaryTag>(m, "AuxiliaryFunction");

    bind_function_term<StaticTag>(m, "StaticFunctionTerm");
    bind_function_term<FluentTag>(m, "FluentFunctionTerm");
    bind_function_term<AuxiliaryTag>(m, "AuxiliaryFunctionTerm");

    bind_ground_function_term<StaticTag>(m, "StaticGroundFunctionTerm");
    bind_ground_function_term<FluentTag>(m, "FluentGroundFunctionTerm");
    bind_ground_function_term<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTerm");

    bind_ground_function_term_value<StaticTag>(m, "StaticGroundFunctionTermValue");
    bind_ground_function_term_value<FluentTag>(m, "FluentGroundFunctionTermValue");
    bind_ground_function_term_value<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermValue");

    bind_unary_operator<Sub, ygg::Data<FunctionExpression>>(m, "UnaryOperatorSub");
    bind_binary_operator<Add, ygg::Data<FunctionExpression>>(m, "BinaryOperatorAdd");
    bind_binary_operator<Sub, ygg::Data<FunctionExpression>>(m, "BinaryOperatorSub");
    bind_binary_operator<Mul, ygg::Data<FunctionExpression>>(m, "BinaryOperatorMul");
    bind_binary_operator<Div, ygg::Data<FunctionExpression>>(m, "BinaryOperatorDiv");
    bind_binary_operator<Eq, ygg::Data<FunctionExpression>>(m, "BinaryOperatorEq");
    bind_binary_operator<Ne, ygg::Data<FunctionExpression>>(m, "BinaryOperatorNe");
    bind_binary_operator<Le, ygg::Data<FunctionExpression>>(m, "BinaryOperatorLe");
    bind_binary_operator<Lt, ygg::Data<FunctionExpression>>(m, "BinaryOperatorLt");
    bind_binary_operator<Ge, ygg::Data<FunctionExpression>>(m, "BinaryOperatorGe");
    bind_binary_operator<Gt, ygg::Data<FunctionExpression>>(m, "BinaryOperatorGt");

    bind_multi_operator<Add, ygg::Data<FunctionExpression>>(m, "MultiOperatorAdd");
    bind_multi_operator<Mul, ygg::Data<FunctionExpression>>(m, "MultiOperatorMul");

    bind_arithmetic_operator<ygg::Data<FunctionExpression>>(m, "ArithmeticOperator");
    bind_boolean_operator<ygg::Data<FunctionExpression>>(m, "BooleanOperator");

    bind_function_expression(m, "FunctionExpression");
    bind_conjunctive_condition(m, "ConjunctiveCondition");

    bind_numeric_effect<Assign, FluentTag>(m, "FluentNumericEffectAssign");
    bind_numeric_effect<Increase, FluentTag>(m, "FluentNumericEffectIncrease");
    bind_numeric_effect<Decrease, FluentTag>(m, "FluentNumericEffectDecrease");
    bind_numeric_effect<ScaleUp, FluentTag>(m, "FluentNumericEffectScaleUp");
    bind_numeric_effect<ScaleDown, FluentTag>(m, "FluentNumericEffectScaleDown");
    bind_numeric_effect<Increase, AuxiliaryTag>(m, "AuxiliaryNumericEffectIncrease");

    bind_numeric_effect_operator<FluentTag>(m, "FluentNumericEffectOperator");
    bind_numeric_effect_operator<AuxiliaryTag>(m, "AuxiliaryNumericEffectOperator");

    bind_conjunctive_effect(m, "ConjunctiveEffect");
    bind_conditional_effect(m, "ConditionalEffect");
    bind_action(m, "Action");
    bind_axiom(m, "Axiom");

    bind_unary_operator<Sub, ygg::Data<GroundFunctionExpression>>(m, "GroundUnaryOperatorSub");
    bind_binary_operator<Add, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorAdd");
    bind_binary_operator<Sub, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorSub");
    bind_binary_operator<Mul, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorMul");
    bind_binary_operator<Div, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorDiv");
    bind_binary_operator<Eq, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorEq");
    bind_binary_operator<Ne, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorNe");
    bind_binary_operator<Le, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorLe");
    bind_binary_operator<Lt, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorLt");
    bind_binary_operator<Ge, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorGe");
    bind_binary_operator<Gt, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorGt");

    bind_multi_operator<Add, ygg::Data<GroundFunctionExpression>>(m, "GroundMultiOperatorAdd");
    bind_multi_operator<Mul, ygg::Data<GroundFunctionExpression>>(m, "GroundMultiOperatorMul");

    bind_arithmetic_operator<ygg::Data<GroundFunctionExpression>>(m, "GroundArithmeticOperator");
    bind_boolean_operator<ygg::Data<GroundFunctionExpression>>(m, "GroundBooleanOperator");

    bind_ground_function_expression(m, "GroundFunctionExpression");
    bind_ground_conjunctive_condition(m, "GroundConjunctiveCondition");

    bind_ground_numeric_effect<Assign, FluentTag>(m, "FluentGroundNumericEffectAssign");
    bind_ground_numeric_effect<Increase, FluentTag>(m, "FluentGroundNumericEffectIncrease");
    bind_ground_numeric_effect<Decrease, FluentTag>(m, "FluentGroundNumericEffectDecrease");
    bind_ground_numeric_effect<ScaleUp, FluentTag>(m, "FluentGroundNumericEffectScaleUp");
    bind_ground_numeric_effect<ScaleDown, FluentTag>(m, "FluentGroundNumericEffectScaleDown");
    bind_ground_numeric_effect<Increase, AuxiliaryTag>(m, "AuxiliaryGroundNumericEffectIncrease");

    bind_ground_numeric_effect_operator<FluentTag>(m, "FluentGroundNumericEffectOperator");
    bind_ground_numeric_effect_operator<AuxiliaryTag>(m, "AuxiliaryGroundNumericEffectOperator");

    bind_ground_conjunctive_effect(m, "GroundConjunctiveEffect");
    bind_ground_conditional_effect(m, "GroundConditionalEffect");
    bind_ground_action(m, "GroundAction");
    bind_ground_axiom(m, "GroundAxiom");

    bind_metric(m, "Metric");
    bind_domain(m, "Domain");
    bind_lifted_task(m, "LiftedTask");
    bind_ground_task(m, "GroundTask");
}

}
