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

#include "tyr/datalog/solver.hpp"

#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/repository.hpp"

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <variant>
#include <vector>

namespace tyr::tests
{
namespace
{
using GroundAtomViews = urd::Store<urd::sequence<tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag>>>;
using StaticGroundAtomViews = urd::Store<urd::sequence<tyr::formalism::AtomView<GroundTag, tyr::formalism::StaticTag>>>;
using PredicateRowViews = urd::Store<urd::sequence<tyr::formalism::PredicateRowView<tyr::formalism::FluentTag>>>;

template<typename Context>
PredicateRowViews binding_views(const Context& ctx)
{
    auto result = PredicateRowViews {};
    const auto& fact_sets = ctx.out().facts().fact_sets.template get<tyr::formalism::FluentTag>();
    for (ygg::uint_t set = 0; set < fact_sets.predicate.size(); ++set)
        for (const auto binding : tyr::datalog::rows(fact_sets, set))
            result.push_back(binding);
    return result;
}

PredicateRowViews binding_views(std::initializer_list<tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag>> atoms)
{
    auto result = PredicateRowViews {};
    for (const auto atom : atoms)
        result.push_back(atom.get_row());
    return result;
}

struct GroundQueueFixture
{
    tyr::formalism::Repository repository = tyr::formalism::Repository();
    urd::Store<urd::sequence<urd::index<tyr::formalism::Predicate<tyr::formalism::StaticTag>>>> static_predicates;
    urd::Store<urd::sequence<urd::index<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>> fluent_predicates;
    urd::Store<urd::sequence<urd::index<tyr::formalism::Function<tyr::formalism::FluentTag>>>> fluent_functions;
    StaticGroundAtomViews initial_static_atoms;
    GroundAtomViews initial_fluent_atoms;
    urd::Store<urd::sequence<urd::index<tyr::formalism::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>>> initial_fluent_fterm_values;
    urd::Store<urd::sequence<urd::index<tyr::formalism::Rule<GroundTag>>>> ground_rules;

    tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag> fluent_atom(const std::string& name)
    {
        auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(name, 0);
        const auto [predicate, predicate_inserted] = repository.insert(predicate_builder);
        if (predicate_inserted)
            fluent_predicates.push_back(predicate.index());

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        binding_builder.relation = predicate.index();
        const auto [binding, binding_inserted] = repository.insert(binding_builder);
        (void) binding_inserted;

        auto atom_builder = urd::data<tyr::formalism::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.index());
        return repository.insert(atom_builder).first;
    }

    tyr::formalism::LiteralView<GroundTag, tyr::formalism::FluentTag> fluent_literal(tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag> atom,
                                                                                     bool polarity = true)
    {
        auto literal_builder = urd::data<tyr::formalism::Literal<GroundTag, tyr::formalism::FluentTag>>(atom.index(), polarity);
        return repository.insert(literal_builder).first;
    }

    tyr::formalism::AtomView<GroundTag, tyr::formalism::StaticTag> static_atom(const std::string& name)
    {
        auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::StaticTag>>(name, 0);
        const auto [predicate, predicate_inserted] = repository.insert(predicate_builder);
        if (predicate_inserted)
            static_predicates.push_back(predicate.index());

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::StaticTag>>>();
        binding_builder.relation = predicate.index();
        const auto binding = repository.insert(binding_builder).first;

        auto atom_builder = urd::data<tyr::formalism::Atom<GroundTag, tyr::formalism::StaticTag>>(binding.index());
        return repository.insert(atom_builder).first;
    }

    tyr::formalism::LiteralView<GroundTag, tyr::formalism::StaticTag> static_literal(tyr::formalism::AtomView<GroundTag, tyr::formalism::StaticTag> atom,
                                                                                     bool polarity = true)
    {
        auto literal_builder = urd::data<tyr::formalism::Literal<GroundTag, tyr::formalism::StaticTag>>(atom.index(), polarity);
        return repository.insert(literal_builder).first;
    }

    tyr::formalism::FunctionTermView<GroundTag, tyr::formalism::FluentTag> fluent_function_term(const std::string& name)
    {
        auto function_builder = urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(name, 0);
        const auto function = repository.insert(function_builder).first;
        fluent_functions.push_back(function.index());

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
        binding_builder.relation = function.index();
        const auto binding = repository.insert(binding_builder).first;

        auto term_builder = urd::data<tyr::formalism::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(binding.index());
        return repository.insert(term_builder).first;
    }

    void initial_fluent_function_value(tyr::formalism::FunctionTermView<GroundTag, tyr::formalism::FluentTag> term, ygg::float_t value)
    {
        auto value_builder = urd::data<tyr::formalism::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(term.index(), value);
        initial_fluent_fterm_values.push_back(repository.insert(value_builder).first.index());
    }

    tyr::formalism::RuleBodyView<GroundTag>
    condition(std::initializer_list<tyr::formalism::LiteralView<GroundTag, tyr::formalism::FluentTag>> fluent_literals = {},
              std::initializer_list<tyr::formalism::LiteralView<GroundTag, tyr::formalism::StaticTag>> static_literals = {})
    {
        auto condition_builder = urd::data<tyr::formalism::RuleBody<GroundTag>>();
        for (const auto literal : fluent_literals)
            condition_builder.fluent_literals.push_back(literal.index());
        for (const auto literal : static_literals)
            condition_builder.static_literals.push_back(literal.index());
        return repository.insert(condition_builder).first;
    }

    tyr::formalism::RuleBodyView<GroundTag>
    numeric_condition(tyr::formalism::FunctionTermView<GroundTag, tyr::formalism::FluentTag> term, tyr::formalism::BooleanOperatorKind op, ygg::float_t value)
    {
        auto comparison_builder = urd::data<tyr::formalism::BinaryOperator<GroundTag, tyr::formalism::BooleanOperatorKind>>(
            op,
            tyr::formalism::FunctionExpression<GroundTag>(term.index()),
            tyr::formalism::FunctionExpression<GroundTag>(value));
        const auto comparison = repository.insert(comparison_builder).first;

        auto condition_builder = urd::data<tyr::formalism::RuleBody<GroundTag>>();
        condition_builder.numeric_constraints.emplace_back(comparison.index());
        return repository.insert(condition_builder).first;
    }

    /// Identical rules are one rule: a rule already in the program is not added again.
    tyr::formalism::RuleView<GroundTag>
    rule(tyr::formalism::RuleBodyView<GroundTag> body, tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag> head, ygg::float_t cost = 0)
    {
        auto rule_builder = urd::data<tyr::formalism::Rule<GroundTag>>();
        rule_builder.body = body.index();
        rule_builder.fluent_heads.push_back(head.index());
        rule_builder.cost = cost;
        const auto [ground_rule, inserted] = repository.insert(rule_builder);
        if (inserted)
            ground_rules.push_back(ground_rule.index());
        return ground_rule;
    }

    /// A body that holds wherever the empty one does, but a rule of its own: a true static atom of its own name.
    tyr::formalism::RuleBodyView<GroundTag> distinct_true_condition(const std::string& name)
    {
        const auto atom = static_atom(name);
        initial_static_atoms.push_back(atom);
        return condition({}, { static_literal(atom) });
    }

    /// As distinct_true_condition, with the fluent literals of `fluent_literals`.
    tyr::formalism::RuleBodyView<GroundTag>
    distinct_true_condition(const std::string& name, std::initializer_list<tyr::formalism::LiteralView<GroundTag, tyr::formalism::FluentTag>> fluent_literals)
    {
        const auto atom = static_atom(name);
        initial_static_atoms.push_back(atom);
        return condition(fluent_literals, { static_literal(atom) });
    }

    tyr::formalism::ProgramView<GroundTag> program()
    {
        auto program_builder = urd::data<tyr::formalism::Program<GroundTag>>();
        program_builder.static_predicates.insert(program_builder.static_predicates.end(), static_predicates.begin(), static_predicates.end());
        program_builder.fluent_predicates.insert(program_builder.fluent_predicates.end(), fluent_predicates.begin(), fluent_predicates.end());
        program_builder.fluent_functions.insert(program_builder.fluent_functions.end(), fluent_functions.begin(), fluent_functions.end());
        for (const auto atom : initial_static_atoms)
            program_builder.static_atoms.push_back(atom.index());
        for (const auto atom : initial_fluent_atoms)
            program_builder.fluent_atoms.push_back(atom.index());
        program_builder.fluent_fterm_values.insert(program_builder.fluent_fterm_values.end(),
                                                   initial_fluent_fterm_values.begin(),
                                                   initial_fluent_fterm_values.end());
        program_builder.rules.insert(program_builder.rules.end(), ground_rules.begin(), ground_rules.end());
        return repository.insert(program_builder).first;
    }
};

urd::Store<urd::sequence<urd::index<tyr::formalism::Rule<GroundTag>>>> rule_indices(const urd::Store<urd::sequence<tyr::formalism::RuleView<GroundTag>>>& rules)
{
    auto indices = urd::Store<urd::sequence<urd::index<tyr::formalism::Rule<GroundTag>>>> {};
    for (const auto rule : rules)
        indices.push_back(rule.index());
    return indices;
}

struct SolvedGroundQueue
{
    PredicateRowViews fluent_bindings;
    datalog::GroundQueueStatistics statistics;
};

SolvedGroundQueue solve_default_state(GroundQueueFixture& fixture)
{
    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);
    return { binding_views(ctx), ctx.out().scheduler().statistics() };
}
}

TEST(TyrDatalogGroundQueueTest, GroundProgramStoresGroundRules)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), atom);

    const auto program = fixture.program();

    EXPECT_EQ(program.get_rules().size(), 1);
    EXPECT_EQ(program.get_rules()[0].index(), fixture.ground_rules[0]);
    // In Datalog: the program's last statement is its rule, a fact without a body, its cost a comment after it.
    const auto rule_text = fmt::format("{}", program.get_rules()[0]);
    EXPECT_EQ(rule_text, fmt::format("{}.  % cost {}", program.get_rules()[0].get_fluent_heads()[0], program.get_rules()[0].get_cost()));
    EXPECT_TRUE(fmt::format("{}", program).ends_with(rule_text));
}

TEST(TyrDatalogGroundQueueTest, EmptyBodyRuleFires)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), atom);

    const auto result = solve_default_state(fixture);

    EXPECT_EQ(result.fluent_bindings, binding_views({ atom }));
    EXPECT_EQ(result.statistics.num_rules_fired, 1);
    EXPECT_EQ(result.statistics.num_facts_derived, 1);
}

TEST(TyrDatalogGroundQueueTest, NoAnnotationPolicyDerivesFactsWithoutStoringAnnotations)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), atom);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_TRUE(tyr::datalog::contains(ctx.out().facts().fact_sets, atom.get_row().index()));
    EXPECT_EQ(ctx.out().annotations().find(atom.get_row().index()), nullptr);
}

TEST(TyrDatalogGroundQueueTest, ChainedRulesDeriveFixpoint)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto c = fixture.fluent_atom("c");
    fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);
    fixture.rule(fixture.condition({ fixture.fluent_literal(b) }), c);

    const auto result = solve_default_state(fixture);

    EXPECT_EQ(result.fluent_bindings, binding_views({ a, b, c }));
    EXPECT_EQ(result.statistics.num_facts_derived, 3);
}

TEST(TyrDatalogGroundQueueTest, ReusesGroundProgramExecutionContext)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);
    const auto first_statistics = ctx.out().scheduler().statistics();
    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    EXPECT_EQ(first_statistics.num_facts_derived, 2);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);
    const auto second_statistics = ctx.out().scheduler().statistics();
    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    EXPECT_EQ(second_statistics.num_facts_derived, 2);
    EXPECT_EQ(second_statistics.num_rules_fired, first_statistics.num_rules_fired);
}

TEST(TyrDatalogGroundQueueTest, MultiPreconditionRuleWaitsForAllFacts)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto c = fixture.fluent_atom("c");
    fixture.initial_fluent_atoms.push_back(a);
    fixture.rule(fixture.condition(), b);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a), fixture.fluent_literal(b) }), c);

    const auto result = solve_default_state(fixture);

    EXPECT_EQ(result.fluent_bindings, binding_views({ a, b, c }));
    EXPECT_EQ(result.statistics.num_facts_derived, 2);
}

TEST(TyrDatalogGroundQueueTest, PositiveFluentPreconditionIndexMapsFactToWaitingRules)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto c = fixture.fluent_atom("c");
    const auto d = fixture.fluent_atom("d");
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), c);
    fixture.rule(fixture.condition({ fixture.fluent_literal(d) }), c);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    const auto& dependencies = const_workspace.dependencies;
    const auto* a_rules = dependencies.find(a.get_row().index());
    const auto* d_rules = dependencies.find(d.get_row().index());

    ASSERT_NE(a_rules, nullptr);
    ASSERT_NE(d_rules, nullptr);
    EXPECT_EQ(rule_indices(*a_rules),
              (urd::Store<urd::sequence<urd::index<tyr::formalism::Rule<GroundTag>>>>({ fixture.ground_rules[0], fixture.ground_rules[1] })));
    EXPECT_EQ(rule_indices(*d_rules), (urd::Store<urd::sequence<urd::index<tyr::formalism::Rule<GroundTag>>>>({ fixture.ground_rules[2] })));
}

TEST(TyrDatalogGroundQueueTest, InitialFluentFactsSatisfyDynamicUnsatisfiedCounts)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.initial_fluent_atoms.push_back(a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    const auto& dependencies = const_workspace.dependencies;
    const auto* a_rules = dependencies.find(a.get_row().index());

    ASSERT_NE(a_rules, nullptr);
    EXPECT_EQ(rule_indices(*a_rules), (urd::Store<urd::sequence<urd::index<tyr::formalism::Rule<GroundTag>>>>({ fixture.ground_rules[0] })));

    ctx.initialize(fixture.initial_fluent_atoms);
    EXPECT_EQ(ctx.out().scheduler().get_states()[fixture.ground_rules[0].value].unsatisfied_count, 0);
}

TEST(TyrDatalogGroundQueueTest, ExplicitFluentStateDrivesDynamicUnsatisfiedCounts)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.initial_fluent_atoms.push_back(a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.out().facts().reset();
    ctx.initialize();
    EXPECT_EQ(ctx.out().scheduler().get_states()[fixture.ground_rules[0].value].unsatisfied_count, 1);
}

TEST(TyrDatalogGroundQueueTest, DerivedFactOnlyDecrementsRulesWaitingOnThatFact)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto c = fixture.fluent_atom("c");
    const auto d = fixture.fluent_atom("d");
    fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);
    fixture.rule(fixture.condition({ fixture.fluent_literal(d) }), c);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    EXPECT_EQ(ctx.out().scheduler().get_states()[fixture.ground_rules[1].value].unsatisfied_count, 0);
    EXPECT_EQ(ctx.out().scheduler().get_states()[fixture.ground_rules[2].value].unsatisfied_count, 1);
}

TEST(TyrDatalogGroundQueueTest, DuplicateHeadsDeriveFactOnce)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.distinct_true_condition("second"), a);

    const auto result = solve_default_state(fixture);

    EXPECT_EQ(result.fluent_bindings, binding_views({ a }));
    EXPECT_EQ(result.statistics.num_rules_fired, 2);
    EXPECT_EQ(result.statistics.num_facts_derived, 1);
}

TEST(TyrDatalogGroundQueueTest, GroundUsedCostOverrideDoesNotCreateEdgeCost)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto derive_a = fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(derive_a.index(), static_cast<datalog::Cost>(7));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<datalog::SumAggregation>,
                                                datalog::NoTerminationPolicy,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), datalog::NoTerminationPolicy(), cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    const auto* annotation = ctx.out().annotations().find(a.get_row().index());
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 0);
}

TEST(TyrDatalogGroundQueueTest, GroundTerminationSkipsWorkWhenInitialFactsSatisfyGoal)
{
    auto fixture = GroundQueueFixture();
    const auto goal_atom = fixture.fluent_atom("goal");
    const auto extra_atom = fixture.fluent_atom("extra");
    fixture.initial_fluent_atoms.push_back(goal_atom);
    fixture.rule(fixture.condition(), extra_atom);
    const auto goal = fixture.condition({ fixture.fluent_literal(goal_atom) });

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(goal);
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ goal_atom }));
    EXPECT_EQ(ctx.out().scheduler().statistics().num_rules_fired, 0);
    EXPECT_EQ(ctx.out().scheduler().statistics().num_facts_derived, 0);
}

TEST(TyrDatalogGroundQueueTest, GroundTerminationStopsAfterGoalDerived)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);
    const auto goal = fixture.condition({ fixture.fluent_literal(a) });

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(goal);
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a }));
    EXPECT_EQ(ctx.out().scheduler().statistics().num_rules_fired, 1);
    EXPECT_TRUE(ctx.out().tp().check(datalog::FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }));
}

TEST(TyrDatalogGroundQueueTest, NegativeFluentGoalIsRejected)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    const auto goal = fixture.condition({ fixture.fluent_literal(atom, false) });

    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    EXPECT_THROW(termination_policy.set_goals(goal), std::invalid_argument);
}

TEST(TyrDatalogGroundQueueTest, GroundTerminationCommitsLowestCostBucket)
{
    auto fixture = GroundQueueFixture();
    const auto goal_atom = fixture.fluent_atom("goal");
    const auto peer_atom = fixture.fluent_atom("peer");
    const auto expensive_source = fixture.fluent_atom("expensive_source");
    fixture.initial_fluent_atoms.push_back(expensive_source);
    fixture.rule(fixture.condition(), goal_atom);
    fixture.rule(fixture.condition(), peer_atom);
    fixture.rule(fixture.condition({ fixture.fluent_literal(expensive_source) }), goal_atom);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(fixture.condition({ fixture.fluent_literal(goal_atom) }));
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    ctx.out().annotations().insert_or_assign(expensive_source.get_row().index(), datalog::BaseAnnotation(static_cast<datalog::Cost>(2)));
    tyr::datalog::compute_model(ctx);

    EXPECT_TRUE(tyr::datalog::contains(ctx.out().facts().fact_sets, goal_atom.get_row().index()));
    EXPECT_TRUE(tyr::datalog::contains(ctx.out().facts().fact_sets, peer_atom.get_row().index()));
    const auto* goal_annotation = ctx.out().annotations().find(goal_atom.get_row().index());
    ASSERT_NE(goal_annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*goal_annotation), 0);
    EXPECT_EQ(ctx.out().scheduler().statistics().num_rules_fired, 2);
}

TEST(TyrDatalogGroundQueueTest, GroundTerminationExposesOnlyOptimalGoalFrontierAchievers)
{
    auto fixture = GroundQueueFixture();
    const auto goal = fixture.fluent_atom("goal");
    const auto cheap = fixture.rule(fixture.condition(), goal, 2);
    fixture.rule(fixture.distinct_true_condition("second"), goal, 2);

    auto termination_policy = datalog::TerminationPolicy<datalog::MaxAggregation>();
    termination_policy.set_goals(fixture.condition({ fixture.fluent_literal(goal) }));
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(cheap.index(), static_cast<datalog::Cost>(1));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<datalog::MaxAggregation>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace =
        Workspace(const_workspace, datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>(), termination_policy, cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    const auto* achievers = ctx.out().annotation_policy().find_achievers(goal.get_row().index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_rule_key(), cheap.index());
}

TEST(TyrDatalogGroundQueueTest, GroundRetainsFirstEqualCostWitness)
{
    auto fixture = GroundQueueFixture();
    const auto goal = fixture.fluent_atom("goal");
    const auto scheduled_first = fixture.rule(fixture.condition(), goal);
    fixture.rule(fixture.distinct_true_condition("second"), goal);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace = datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::MaxAggregation>, datalog::NoTerminationPolicy>;
    auto workspace = Workspace(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    const auto* annotation = ctx.out().annotations().find(goal.get_row().index());
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<datalog::WitnessAnnotation<GroundTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_rule_key(), scheduled_first.index());
}

TEST(TyrDatalogGroundQueueTest, AchieverPolicyGroundRecordsDistinctRuleBindings)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition(), a);
    const auto first_derive_b = fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);
    const auto second_derive_b = fixture.rule(fixture.distinct_true_condition("second", { fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<datalog::MaxAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>(),
                               datalog::TerminationPolicy<datalog::MaxAggregation>());
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    const auto* achievers = ctx.out().annotation_policy().find_achievers(b.get_row().index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 2);
    const auto first_key = (*achievers)[0].get_rule_key();
    const auto second_key = (*achievers)[1].get_rule_key();
    EXPECT_NE(first_key, second_key);
    EXPECT_TRUE((first_key == first_derive_b.index() && second_key == second_derive_b.index())
                || (first_key == second_derive_b.index() && second_key == first_derive_b.index()));
}

TEST(TyrDatalogGroundQueueTest, AchieverPolicyKeepsFirstEqualAndReplacesStrictlyCheaperWitness)
{
    auto fixture = GroundQueueFixture();
    const auto head_atom = fixture.fluent_atom("head");
    const auto head = head_atom.get_row();
    const auto rule = fixture.rule(fixture.condition(), head_atom);
    auto policy = datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>();

    policy.record_achiever(head.index(), datalog::WitnessAnnotation<GroundTag>(rule.index(), static_cast<datalog::Cost>(2)));
    policy.record_achiever(head.index(), datalog::WitnessAnnotation<GroundTag>(rule.index(), static_cast<datalog::Cost>(2)));

    const auto* achievers = policy.find_achievers(head.index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_cost(), static_cast<datalog::Cost>(2));

    policy.record_achiever(head.index(), datalog::WitnessAnnotation<GroundTag>(rule.index(), static_cast<datalog::Cost>(1)));
    achievers = policy.find_achievers(head.index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_cost(), static_cast<datalog::Cost>(1));
}

TEST(TyrDatalogGroundQueueTest, StaticLiteralsUseTheirPolarityAndInitialTruth)
{
    auto fixture = GroundQueueFixture();
    const auto present = fixture.static_atom("present");
    const auto absent = fixture.static_atom("absent");
    fixture.initial_static_atoms.push_back(present);

    const auto positive_present = fixture.fluent_atom("positive_present");
    const auto positive_absent = fixture.fluent_atom("positive_absent");
    const auto negative_present = fixture.fluent_atom("negative_present");
    const auto negative_absent = fixture.fluent_atom("negative_absent");
    fixture.rule(fixture.condition({}, { fixture.static_literal(present) }), positive_present);
    fixture.rule(fixture.condition({}, { fixture.static_literal(absent) }), positive_absent);
    fixture.rule(fixture.condition({}, { fixture.static_literal(present, false) }), negative_present);
    fixture.rule(fixture.condition({}, { fixture.static_literal(absent, false) }), negative_absent);

    const auto source_program = fixture.program();
    EXPECT_THROW(static_cast<void>(datalog::ConstProgramWorkspace<GroundTag> { source_program }), std::invalid_argument);

    auto repository = tyr::formalism::Repository(&source_program.context());
    const auto program = tyr::datalog::remove_statically_inapplicable_rules(source_program, repository);
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    EXPECT_EQ(const_workspace.program.get_rules().size(), 2);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    auto expected = PredicateRowViews {};
    for (const auto rule : program.get_rules())
        expected.push_back(rule.get_fluent_heads().front().get_row());
    EXPECT_EQ(binding_views(ctx), expected);
    EXPECT_EQ(ctx.out().scheduler().statistics().num_rules_fired, 2);
}

TEST(TyrDatalogGroundQueueTest, ExplicitFluentAtomsRestoreDeclaredInitialFunctionValues)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 3);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    tyr::datalog::insert(ctx.out().facts().fact_sets.get<tyr::formalism::FluentTag>(), term.get_row().index(), ygg::ClosedInterval<ygg::float_t>(9, 9));

    ctx.initialize(fixture.initial_fluent_atoms);

    EXPECT_EQ(tyr::datalog::value(ctx.out().facts().fact_sets.get<tyr::formalism::FluentTag>(), term.get_row().index()),
              ygg::ClosedInterval<ygg::float_t>(3, 3));
}

TEST(TyrDatalogGroundQueueTest, NegativeLiteralHoldsWhereItsAtomIsAbsent)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition({ fixture.fluent_literal(a, false) }), b);

    EXPECT_EQ(solve_default_state(fixture).fluent_bindings, binding_views({ b }));

    fixture.initial_fluent_atoms.push_back(a);
    EXPECT_EQ(solve_default_state(fixture).fluent_bindings, binding_views({ a }));
}

TEST(TyrDatalogGroundQueueTest, NegativeLiteralReadsItsAtomOnceALowerStratumIsComplete)
{
    // c is derived from a; d from not c: d's stratum begins only once c's is complete.
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto c = fixture.fluent_atom("c");
    const auto d = fixture.fluent_atom("d");
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), c);
    fixture.rule(fixture.condition({ fixture.fluent_literal(c, false) }), d);
    fixture.initial_fluent_atoms.push_back(a);

    EXPECT_EQ(solve_default_state(fixture).fluent_bindings, binding_views({ a, c }));
}

TEST(TyrDatalogGroundQueueTest, NegativeCycleIsRejected)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    fixture.rule(fixture.condition({ fixture.fluent_literal(a, false) }), a);

    EXPECT_THROW(datalog::ConstProgramWorkspace<GroundTag>(fixture.program()), std::runtime_error);
}
}
