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

#ifndef TYR_PYTHON_DATALOG_DATALOG_HPP_
#define TYR_PYTHON_DATALOG_DATALOG_HPP_

#include "module.hpp"

#include <nanobind/make_iterator.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/datalog/datalog.hpp>
#include <tyr/datalog/ground/solver.hpp>
#include <tyr/datalog/lifted/policies/annotation.hpp>
#include <tyr/datalog/lifted/solver.hpp>
#include <tyr/datalog/policies/annotation.hpp>
#include <tyr/datalog/policies/cost.hpp>
#include <tyr/datalog/policies/termination.hpp>
#include <tyr/formalism/datalog/merge.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::datalog
{

using Interval = ygg::ClosedInterval<ygg::float_t>;

template<::tyr::formalism::FactKind T>
void bind_predicate_fact_set(nb::module_& m, const char* name)
{
    using Set = PredicateFactSet<T>;

    nb::class_<Set>(m, name)
        .def("get_predicate", &Set::get_predicate, nb::keep_alive<0, 1>())
        .def(
            "get_bindings",
            [](const Set& self)
            {
                const auto bindings = self.get_bindings();
                return nb::make_iterator(nb::type<Set>(), "BindingIterator", bindings.begin(), bindings.end(), nb::keep_alive<0, 1>());
            },
            nb::keep_alive<0, 1>())
        .def("count",
             [](const Set& self)
             {
                 auto result = size_t(0);
                 for ([[maybe_unused]] const auto binding : self.get_bindings())
                     ++result;
                 return result;
             })
        .def("contains", &Set::contains, "binding"_a);
}

template<::tyr::formalism::FactKind T>
void bind_function_fact_set(nb::module_& m, const char* name)
{
    using Set = FunctionFactSet<T>;
    using Binding = ::tyr::formalism::datalog::FunctionBindingView<T>;

    nb::class_<Set>(m, name)
        .def("get_function", &Set::get_function, nb::keep_alive<0, 1>())
        .def(
            "get_binding_values",
            [](const Set& self)
            {
                const auto values = self.get_binding_values();
                return nb::make_iterator(nb::type<Set>(), "BindingValueIterator", values.begin(), values.end());
            },
            nb::keep_alive<0, 1>())
        .def("count",
             [](const Set& self)
             {
                 auto result = size_t(0);
                 for ([[maybe_unused]] const auto entry : self.get_binding_values())
                     ++result;
                 return result;
             })
        .def("get", [](const Set& self, Binding binding) { return self[binding]; }, "binding"_a);
}

template<::tyr::formalism::FactKind T>
void bind_tagged_fact_sets(nb::module_& m, const char* name)
{
    using Sets = TaggedFactSets<T>;

    nb::class_<Sets>(m, name)
        .def(
            "get_predicate_sets",
            [](const Sets& self)
            {
                auto result = std::vector<const PredicateFactSet<T>*> {};
                for (const auto& set : self.predicate.get_sets())
                    result.push_back(&set);
                return result;
            },
            nb::rv_policy::reference_internal)
        .def(
            "get_function_sets",
            [](const Sets& self)
            {
                auto result = std::vector<const FunctionFactSet<T>*> {};
                for (const auto& set : self.function.get_sets())
                    result.push_back(&set);
                return result;
            },
            nb::rv_policy::reference_internal);
}

inline void bind_fact_sets(nb::module_& m)
{
    bind_predicate_fact_set<::tyr::formalism::StaticTag>(m, "StaticPredicateFactSet");
    bind_predicate_fact_set<::tyr::formalism::FluentTag>(m, "FluentPredicateFactSet");
    bind_function_fact_set<::tyr::formalism::StaticTag>(m, "StaticFunctionFactSet");
    bind_function_fact_set<::tyr::formalism::FluentTag>(m, "FluentFunctionFactSet");
    bind_tagged_fact_sets<::tyr::formalism::StaticTag>(m, "StaticFactSets");
    bind_tagged_fact_sets<::tyr::formalism::FluentTag>(m, "FluentFactSets");
}

template<TaskKind Kind>
void bind_annotations(nb::module_& m)
{
    using NumericSupportT = NumericSupport<Kind>;
    using BaseAnnotationT = BaseAnnotation<Kind>;
    using WitnessAnnotationT = WitnessAnnotation<Kind, ::tyr::formalism::PredicateTag>;
    using FunctionWitnessAnnotationT = WitnessAnnotation<Kind, ::tyr::formalism::FunctionTag>;
    using PredicateAnnotationStore = PredicateAnnotations<Kind>;
    using FunctionAnnotationStore = FunctionAnnotations<Kind>;
    using RuleKey = WitnessRuleKeyT<Kind, ::tyr::formalism::PredicateTag>;
    using FunctionRuleKey = WitnessRuleKeyT<Kind, ::tyr::formalism::FunctionTag>;
    using NumericKey = FunctionAnnotationHead;
    using PredicateKey = typename PredicateAnnotationStore::Key;

    auto numeric_support_cls = nb::class_<NumericSupportT>(m, "NumericSupport")
                                   .def(nb::init<NumericKey, Interval, Cost>(), "key"_a, "interval"_a, "cost"_a)
                                   .def("get_key", &NumericSupportT::get_key, nb::keep_alive<0, 1>())
                                   .def("get_interval", &NumericSupportT::get_interval)
                                   .def("get_cost", &NumericSupportT::get_cost);
    ygg::add_comparison(numeric_support_cls);

    auto base_annotation_cls = nb::class_<BaseAnnotationT>(m, "BaseAnnotation")
                                   .def(nb::init<>())
                                   .def(nb::init<Cost>(), "cost"_a)
                                   .def(nb::init<Interval, Cost>(), "metric"_a, "cost"_a)
                                   .def("get_metric", &BaseAnnotationT::get_metric)
                                   .def("get_cost", &BaseAnnotationT::get_cost);
    ygg::add_comparison(base_annotation_cls);

    auto witness_annotation_cls =
        nb::class_<WitnessAnnotationT>(m, "WitnessAnnotation")
            .def(nb::init<RuleKey, Cost>(), "rule_key"_a, "cost"_a)
            .def(nb::init<RuleKey, Interval, Cost>(), "rule_key"_a, "metric"_a, "cost"_a)
            .def(nb::init<RuleKey, Interval, Cost, std::vector<NumericSupportT>>(), "rule_key"_a, "metric"_a, "cost"_a, "numeric_supports"_a)
            .def("get_rule_key", &WitnessAnnotationT::get_rule_key, nb::keep_alive<0, 1>())
            .def("get_metric", &WitnessAnnotationT::get_metric)
            .def("get_cost", &WitnessAnnotationT::get_cost)
            .def("get_numeric_supports", &WitnessAnnotationT::get_numeric_supports, nb::rv_policy::copy);
    ygg::add_comparison(witness_annotation_cls);

    auto function_witness_annotation_cls =
        nb::class_<FunctionWitnessAnnotationT>(m, "FunctionWitnessAnnotation")
            .def(nb::init<FunctionRuleKey, Cost>(), "rule_key"_a, "cost"_a)
            .def(nb::init<FunctionRuleKey, Interval, Cost>(), "rule_key"_a, "metric"_a, "cost"_a)
            .def(nb::init<FunctionRuleKey, Interval, Cost, std::vector<NumericSupportT>>(), "rule_key"_a, "metric"_a, "cost"_a, "numeric_supports"_a)
            .def("get_rule_key", &FunctionWitnessAnnotationT::get_rule_key, nb::keep_alive<0, 1>())
            .def("get_metric", &FunctionWitnessAnnotationT::get_metric)
            .def("get_cost", &FunctionWitnessAnnotationT::get_cost)
            .def("get_numeric_supports", &FunctionWitnessAnnotationT::get_numeric_supports, nb::rv_policy::copy);
    ygg::add_comparison(function_witness_annotation_cls);

    nb::class_<PredicateAnnotationStore>(m, "PredicateAnnotations")
        .def(nb::init<>())
        .def("clear", &PredicateAnnotationStore::clear)
        .def(
            "find",
            [](const PredicateAnnotationStore& self, PredicateKey key) -> std::optional<Annotation<Kind>>
            {
                const auto* annotation = self.find(key);
                return annotation ? std::optional<Annotation<Kind>>(*annotation) : std::nullopt;
            },
            "binding"_a);

    nb::class_<FunctionAnnotationStore>(m, "FunctionAnnotations")
        .def(nb::init<>())
        .def("clear", &FunctionAnnotationStore::clear)
        .def("size", &FunctionAnnotationStore::size)
        .def(
            "find",
            [](const FunctionAnnotationStore& self, NumericKey key) -> std::optional<Annotation<Kind, ::tyr::formalism::FunctionTag>>
            {
                const auto* annotation = self.find(key);
                return annotation ? std::optional<Annotation<Kind, ::tyr::formalism::FunctionTag>>(*annotation) : std::nullopt;
            },
            "binding"_a)
        .def(
            "find",
            [](const FunctionAnnotationStore& self, NumericKey key, const Interval& interval) -> std::optional<Annotation<Kind, ::tyr::formalism::FunctionTag>>
            {
                const auto* annotation = self.find(key, interval);
                return annotation ? std::optional<Annotation<Kind, ::tyr::formalism::FunctionTag>>(*annotation) : std::nullopt;
            },
            "binding"_a,
            "interval"_a);
}

template<TaskKind Kind, typename CostPolicy>
void bind_cost_policy(nb::module_& m, const char* name)
{
    using NumericKey = FunctionAnnotationHead;

    auto cls = nb::class_<CostPolicy>(m, name).def(nb::init<>()).def("clear", &CostPolicy::clear);

    const auto bind_rule_costs = [&]<::tyr::formalism::RelationKind R>()
    {
        using RuleKey = WitnessRuleKeyT<Kind, R>;
        cls.def(
               "get_cost",
               [](const CostPolicy& self, RuleKey rule) { return self.get_cost(rule); },
               "rule"_a)
            .def(
                "get_cost",
                [](const CostPolicy& self, RuleKey rule, NumericKey key, const Interval& interval) { return self.get_cost(rule, key, interval); },
                "rule"_a,
                "binding"_a,
                "interval"_a)
            .def(
                "set_cost",
                [](CostPolicy& self, RuleKey rule, Cost cost) { self.set_cost(rule, cost); },
                "rule"_a,
                "cost"_a)
            .def(
                "set_cost",
                [](CostPolicy& self, RuleKey rule, NumericKey key, const Interval& interval, Cost cost) { self.set_cost(rule, key, interval, cost); },
                "rule"_a,
                "binding"_a,
                "interval"_a,
                "cost"_a);
    };
    bind_rule_costs.template operator()<::tyr::formalism::PredicateTag>();
    bind_rule_costs.template operator()<::tyr::formalism::FunctionTag>();
}

template<TaskKind Kind, typename Aggregation>
void bind_termination_policy(nb::module_& m, const char* name)
{
    using Policy = TerminationPolicy<Kind, Aggregation>;

    nb::class_<Policy>(m, name)
        .def(nb::init<>())
        .def("set_goals", &Policy::set_goals, "goals"_a)
        .def("get_goal", &Policy::get_goal, nb::rv_policy::copy)
        .def("reset", &Policy::reset)
        .def("clear", &Policy::clear);
}

template<TaskKind Kind>
void bind_policies(nb::module_& m)
{
    using NoAnnotation = NoAnnotationPolicy<Kind>;
    using SumMinCostAnnotation = MinCostAnnotationPolicy<Kind, SumAggregation>;
    using MaxMinCostAnnotation = MinCostAnnotationPolicy<Kind, MaxAggregation>;
    using MaxMinCostAnnotationWithAchievers = MinCostAnnotationWithAchieversPolicy<Kind, MaxAggregation>;
    using NoTermination = NoTerminationPolicy<Kind>;

    nb::class_<NoAnnotation>(m, "NoAnnotationPolicy").def(nb::init<>());
    nb::class_<SumMinCostAnnotation>(m, "SumMinCostAnnotationPolicy").def(nb::init<>());
    nb::class_<MaxMinCostAnnotation>(m, "MaxMinCostAnnotationPolicy").def(nb::init<>());
    nb::class_<MaxMinCostAnnotationWithAchievers, MaxMinCostAnnotation>(m, "MaxMinCostAnnotationWithAchieversPolicy")
        .def(nb::init<>())
        .def("clear_achievers", &MaxMinCostAnnotationWithAchievers::clear_achievers)
        .def(
            "find_achievers",
            [](const MaxMinCostAnnotationWithAchievers& self,
               PredicateAnnotationHead binding) -> std::optional<typename MaxMinCostAnnotationWithAchievers::Achievers>
            {
                const auto* achievers = self.find_achievers(binding);
                return achievers ? std::optional<typename MaxMinCostAnnotationWithAchievers::Achievers>(*achievers) : std::nullopt;
            },
            "binding"_a);

    nb::class_<NoTermination>(m, "NoTerminationPolicy")
        .def(nb::init<>())
        .def("set_goals", &NoTermination::set_goals, "goals"_a)
        .def("reset", &NoTermination::reset)
        .def("clear", &NoTermination::clear);
    bind_termination_policy<Kind, SumAggregation>(m, "SumTerminationPolicy");
    bind_termination_policy<Kind, MaxAggregation>(m, "MaxTerminationPolicy");

    bind_cost_policy<Kind, RuleCostPolicy<Kind>>(m, "RuleCostPolicy");
    bind_cost_policy<Kind, RuleCostOverridePolicy<Kind>>(m, "RuleCostOverridePolicy");
}

template<TaskKind Kind, typename Workspace>
void bind_workspace(nb::module_& m, const std::string& name)
{
    auto cls = nb::class_<Workspace>(m, name.c_str())
                   .def(nb::init<Program<Kind>&>(), "program"_a, nb::keep_alive<1, 2>())
                   .def(
                       "get_static_fact_sets",
                       [](const Workspace& self) -> const TaggedFactSets<::tyr::formalism::StaticTag>& { return self.const_workspace.facts.fact_sets; },
                       nb::rv_policy::reference_internal)
                   .def(
                       "get_fluent_fact_sets",
                       [](const Workspace& self) -> const TaggedFactSets<::tyr::formalism::FluentTag>& { return self.facts.fact_sets; },
                       nb::rv_policy::reference_internal)
                   .def(
                       "get_annotations",
                       [](Workspace& self) -> auto& { return self.annotations; },
                       nb::rv_policy::reference_internal)
                   .def(
                       "get_numeric_annotations",
                       [](Workspace& self) -> auto& { return self.numeric_annotations; },
                       nb::rv_policy::reference_internal)
                   .def(
                       "get_annotation_policy",
                       [](Workspace& self) -> auto& { return self.annotation_policy; },
                       nb::rv_policy::reference_internal)
                   .def(
                       "get_termination_policy",
                       [](Workspace& self) -> auto& { return self.tp; },
                       nb::rv_policy::reference_internal)
                   .def(
                       "get_cost_policy",
                       [](Workspace& self) -> auto& { return self.cost_policy; },
                       nb::rv_policy::reference_internal)
                   .def("clear_costs", &Workspace::clear_costs);

    if constexpr (std::same_as<Kind, GroundTag>)
    {
        using Atom = ::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag>;
        using FunctionTerm = ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>;

        cls.def("clear_fluent_facts", [](Workspace& self) { self.facts.reset(); })
            .def(
                "insert_fluent_atom",
                [](Workspace& self, Atom atom) { return self.facts.fact_sets.predicate.insert(atom); },
                "atom"_a)
            .def(
                "set_fluent_function",
                [](Workspace& self, FunctionTerm term, const Interval& interval) { return self.facts.fact_sets.function.insert(term, interval); },
                "function_term"_a,
                "interval"_a);
    }
    else
    {
        using Atom = ::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag>;
        using PredicateBinding = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
        using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;

        cls.def(
               "get_workspace_repository",
               [](Workspace& self) -> auto& { return self.workspace_repository; },
               nb::rv_policy::reference_internal)
            .def("reset_evaluation", &Workspace::reset_evaluation)
            .def("reset_facts", [](Workspace& self) { self.facts.reset(); })
            .def(
                "insert_fluent_atom",
                [](Workspace& self, Atom atom)
                {
                    auto context = ::tyr::formalism::datalog::MergeContext { self.datalog_builder, self.workspace_repository };
                    return self.facts.fact_sets.predicate.insert(::tyr::formalism::datalog::merge_d2d(atom, context).first);
                },
                "atom"_a)
            .def(
                "insert_fluent_binding",
                [](Workspace& self, PredicateBinding binding)
                {
                    auto context = ::tyr::formalism::datalog::MergeContext { self.datalog_builder, self.workspace_repository };
                    return self.facts.fact_sets.predicate.insert(::tyr::formalism::datalog::merge_d2d(binding, context).first);
                },
                "binding"_a)
            .def(
                "set_fluent_function",
                [](Workspace& self, FunctionBinding binding, const Interval& interval)
                {
                    auto context = ::tyr::formalism::datalog::MergeContext { self.datalog_builder, self.workspace_repository };
                    return self.facts.fact_sets.function.insert(::tyr::formalism::datalog::merge_d2d(binding, context).first, interval);
                },
                "binding"_a,
                "interval"_a)
            .def("get_statistics", [](Workspace& self) -> auto& { return self.statistics; }, nb::rv_policy::reference_internal);
    }
}

template<TaskKind Kind, typename Workspace, typename Context>
void bind_configuration(nb::module_& m, const char* prefix)
{
    const auto workspace_name = std::string(prefix) + "ProgramWorkspace";
    const auto context_name = std::string(prefix) + "ProgramExecutionContext";
    bind_workspace<Kind, Workspace>(m, workspace_name);

    auto cls = nb::class_<Context>(m, context_name.c_str());
    cls.def(nb::init<Workspace&>(), "workspace"_a, nb::keep_alive<1, 2>())
        .def("clear", &Context::clear)
        .def("get_statistics", [](Context& self) -> auto& { return self.out().statistics(); }, nb::rv_policy::reference_internal);

    if constexpr (std::same_as<Kind, GroundTag>)
    {
        cls.def("initialize", [](Context& self) { self.initialize(); })
            .def(
                "initialize",
                [](Context& self, const std::vector<::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag>>& fluent_atoms)
                { self.initialize(fluent_atoms); },
                "fluent_atoms"_a);
    }

    m.def(
        "compute_model",
        [](Context& context, ygg::ExecutionContext& execution_context) { execution_context.arena().execute([&] { compute_model(context); }); },
        "context"_a,
        "execution_context"_a,
        nb::call_guard<nb::gil_scoped_release>());
}

template<TaskKind Kind, typename AP, typename TP, typename CP>
void bind_configuration(nb::module_& m, const char* prefix)
{
    using Workspace = ProgramWorkspace<Kind, AP, TP, CP>;
    using Context = ProgramExecutionContext<Kind, AP, TP, CP>;
    bind_configuration<Kind, Workspace, Context>(m, prefix);
}

template<TaskKind Kind>
void bind_common_configurations(nb::module_& m)
{
    bind_configuration<Kind, NoAnnotationPolicy<Kind>, NoTerminationPolicy<Kind>, RuleCostPolicy<Kind>>(m, "Unannotated");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, SumAggregation>, NoTerminationPolicy<Kind>, RuleCostPolicy<Kind>>(m, "Sum");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, SumAggregation>, TerminationPolicy<Kind, SumAggregation>, RuleCostPolicy<Kind>>(m, "SumGoal");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, MaxAggregation>, NoTerminationPolicy<Kind>, RuleCostPolicy<Kind>>(m, "Max");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, MaxAggregation>, TerminationPolicy<Kind, MaxAggregation>, RuleCostPolicy<Kind>>(m, "MaxGoal");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, SumAggregation>, NoTerminationPolicy<Kind>, RuleCostOverridePolicy<Kind>>(m, "SumOverride");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, SumAggregation>, TerminationPolicy<Kind, SumAggregation>, RuleCostOverridePolicy<Kind>>(
        m,
        "SumGoalOverride");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, MaxAggregation>, NoTerminationPolicy<Kind>, RuleCostOverridePolicy<Kind>>(m, "MaxOverride");
    bind_configuration<Kind, MinCostAnnotationPolicy<Kind, MaxAggregation>, TerminationPolicy<Kind, MaxAggregation>, RuleCostOverridePolicy<Kind>>(
        m,
        "MaxGoalOverride");
    bind_configuration<Kind, MinCostAnnotationWithAchieversPolicy<Kind, MaxAggregation>, TerminationPolicy<Kind, MaxAggregation>, RuleCostPolicy<Kind>>(
        m,
        "MaxAchieverGoal");
    bind_configuration<Kind, MinCostAnnotationWithAchieversPolicy<Kind, MaxAggregation>, TerminationPolicy<Kind, MaxAggregation>, RuleCostOverridePolicy<Kind>>(
        m,
        "MaxAchieverGoalOverride");
}

}

#endif
