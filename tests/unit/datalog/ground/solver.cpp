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
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace tyr::tests
{
namespace
{
using GroundAtomViews = urd::Store<urd::sequence<tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag>>>;
using StaticGroundAtomViews = urd::Store<urd::sequence<tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::StaticTag>>>;
using PredicateRowViews = urd::Store<urd::sequence<tyr::formalism::datalog::PredicateRowView<tyr::formalism::FluentTag>>>;

template<typename Context>
PredicateRowViews binding_views(const Context& ctx)
{
    auto result = PredicateRowViews {};
    for (const auto& set : ctx.out().facts().fact_sets.predicate.get_sets())
        for (const auto binding : set.get_bindings())
            result.push_back(binding);
    return result;
}

PredicateRowViews binding_views(std::initializer_list<tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag>> atoms)
{
    auto result = PredicateRowViews {};
    for (const auto atom : atoms)
        result.push_back(atom.get_row());
    return result;
}

struct GroundQueueFixture
{
    tyr::formalism::datalog::Repository repository = tyr::formalism::datalog::Repository();
    urd::Store<urd::sequence<urd::index<tyr::formalism::Predicate<tyr::formalism::StaticTag>>>> static_predicates;
    urd::Store<urd::sequence<urd::index<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>> fluent_predicates;
    urd::Store<urd::sequence<urd::index<tyr::formalism::Function<tyr::formalism::FluentTag>>>> fluent_functions;
    StaticGroundAtomViews initial_static_atoms;
    GroundAtomViews initial_fluent_atoms;
    urd::Store<urd::sequence<urd::index<tyr::formalism::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>>> initial_fluent_fterm_values;
    urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>>> ground_rules;
    urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::FunctionTag>>>> ground_function_rules;
    ygg::uint_t next_rule_id = 0;

    tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag> fluent_atom(const std::string& name)
    {
        auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(name, 0);
        const auto [predicate, predicate_inserted] = ygg::formalism::insert(repository, predicate_builder);
        if (predicate_inserted)
            fluent_predicates.push_back(predicate.index());

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        binding_builder.relation = predicate.index();
        const auto [binding, binding_inserted] = ygg::formalism::insert(repository, binding_builder);
        (void) binding_inserted;

        auto atom_builder = urd::data<tyr::formalism::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.data());
        return ygg::formalism::insert(repository, atom_builder).first;
    }

    tyr::formalism::datalog::LiteralView<GroundTag, tyr::formalism::FluentTag>
    fluent_literal(tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag> atom, bool polarity = true)
    {
        auto literal_builder = urd::data<tyr::formalism::Literal<GroundTag, tyr::formalism::FluentTag>>(atom.index(), polarity);
        return ygg::formalism::insert(repository, literal_builder).first;
    }

    tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::StaticTag> static_atom(const std::string& name)
    {
        auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::StaticTag>>(name, 0);
        const auto [predicate, predicate_inserted] = ygg::formalism::insert(repository, predicate_builder);
        if (predicate_inserted)
            static_predicates.push_back(predicate.index());

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::StaticTag>>>();
        binding_builder.relation = predicate.index();
        const auto binding = ygg::formalism::insert(repository, binding_builder).first;

        auto atom_builder = urd::data<tyr::formalism::Atom<GroundTag, tyr::formalism::StaticTag>>(binding.data());
        return ygg::formalism::insert(repository, atom_builder).first;
    }

    tyr::formalism::datalog::LiteralView<GroundTag, tyr::formalism::StaticTag>
    static_literal(tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::StaticTag> atom, bool polarity = true)
    {
        auto literal_builder = urd::data<tyr::formalism::Literal<GroundTag, tyr::formalism::StaticTag>>(atom.index(), polarity);
        return ygg::formalism::insert(repository, literal_builder).first;
    }

    tyr::formalism::datalog::FunctionTermView<GroundTag, tyr::formalism::FluentTag> fluent_function_term(const std::string& name)
    {
        auto function_builder = urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(name, 0);
        const auto function = ygg::formalism::insert(repository, function_builder).first;
        fluent_functions.push_back(function.index());

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
        binding_builder.relation = function.index();
        const auto binding = ygg::formalism::insert(repository, binding_builder).first;

        auto term_builder = urd::data<tyr::formalism::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(binding.data());
        return ygg::formalism::insert(repository, term_builder).first;
    }

    void initial_fluent_function_value(tyr::formalism::datalog::FunctionTermView<GroundTag, tyr::formalism::FluentTag> term, ygg::float_t value)
    {
        auto value_builder = urd::data<tyr::formalism::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(term.index(), value);
        initial_fluent_fterm_values.push_back(ygg::formalism::insert(repository, value_builder).first.index());
    }

    tyr::formalism::datalog::ConjunctiveConditionView<GroundTag>
    condition(std::initializer_list<tyr::formalism::datalog::LiteralView<GroundTag, tyr::formalism::FluentTag>> fluent_literals = {},
              std::initializer_list<tyr::formalism::datalog::LiteralView<GroundTag, tyr::formalism::StaticTag>> static_literals = {})
    {
        auto condition_builder = urd::data<tyr::formalism::datalog::ConjunctiveCondition<GroundTag>>();
        for (const auto literal : fluent_literals)
            condition_builder.fluent_literals.push_back(literal.index());
        for (const auto literal : static_literals)
            condition_builder.static_literals.push_back(literal.index());
        return ygg::formalism::insert(repository, condition_builder).first;
    }

    tyr::formalism::datalog::ConjunctiveConditionView<GroundTag>
    numeric_condition(tyr::formalism::datalog::FunctionTermView<GroundTag, tyr::formalism::FluentTag> term,
                      tyr::formalism::BooleanOperatorKind op,
                      ygg::float_t value)
    {
        auto comparison_builder = urd::data<tyr::formalism::BinaryOperator<GroundTag, tyr::formalism::BooleanOperatorKind>>(
            op,
            tyr::formalism::FunctionExpression<GroundTag>(term.index()),
            tyr::formalism::FunctionExpression<GroundTag>(value));
        const auto comparison = ygg::formalism::insert(repository, comparison_builder).first;

        auto condition_builder = urd::data<tyr::formalism::datalog::ConjunctiveCondition<GroundTag>>();
        condition_builder.numeric_constraints.emplace_back(comparison.index());
        return ygg::formalism::insert(repository, condition_builder).first;
    }

    tyr::formalism::datalog::RuleRowView<tyr::formalism::PredicateTag> fresh_rule_binding()
    {
        auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>("dummy_" + std::to_string(next_rule_id++), 0);
        const auto predicate = ygg::formalism::insert(repository, predicate_builder).first;

        auto atom_builder = urd::data<tyr::formalism::Atom<LiftedTag, tyr::formalism::FluentTag>>();
        atom_builder.predicate = predicate.index();
        const auto atom = ygg::formalism::insert(repository, atom_builder).first;

        auto condition_builder = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
        const auto lifted_condition = ygg::formalism::insert(repository, condition_builder).first;

        auto rule_builder = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
        rule_builder.body = lifted_condition.index();
        rule_builder.heads.push_back(atom.index());
        const auto rule = ygg::formalism::insert(repository, rule_builder).first;

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>>();
        binding_builder.relation = rule.index();
        return ygg::formalism::insert(repository, binding_builder).first;
    }

    tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::PredicateTag>
    rule(tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> body,
         tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag> head,
         tyr::formalism::datalog::RuleRowView<tyr::formalism::PredicateTag> binding)
    {
        auto rule_builder = urd::data<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>();
        rule_builder.row = binding.data();
        rule_builder.body = body.index();
        rule_builder.heads.push_back(head.index());
        const auto ground_rule = ygg::formalism::insert(repository, rule_builder).first;
        ground_rules.push_back(ground_rule.index());
        return ground_rule;
    }

    tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::PredicateTag>
    rule(tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> body, tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag> head)
    { return rule(body, head, fresh_rule_binding()); }

    tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::PredicateTag>
    rule(tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> body,
         tyr::formalism::datalog::AtomView<GroundTag, tyr::formalism::FluentTag> head,
         tyr::formalism::datalog::RuleRowView<tyr::formalism::PredicateTag> binding,
         ygg::float_t cost)
    {
        auto rule_builder = urd::data<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>();
        rule_builder.row = binding.data();
        rule_builder.body = body.index();
        rule_builder.heads.push_back(head.index());
        rule_builder.cost = cost;
        const auto ground_rule = ygg::formalism::insert(repository, rule_builder).first;
        ground_rules.push_back(ground_rule.index());
        return ground_rule;
    }

    tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::FunctionTag>
    numeric_rule(tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> body,
                 tyr::formalism::datalog::FunctionTermView<GroundTag, tyr::formalism::FluentTag> head,
                 tyr::formalism::NumericEffectOperatorKind op,
                 ygg::float_t value,
                 ygg::float_t cost = 0)
    {
        auto lifted_term_builder = urd::data<tyr::formalism::FunctionTerm<LiftedTag, tyr::formalism::FluentTag>>();
        lifted_term_builder.function = head.get_function().index();
        const auto lifted_term = ygg::formalism::insert(repository, lifted_term_builder).first;

        auto lifted_effect_builder = urd::data<tyr::formalism::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
            op,
            lifted_term.index(),
            tyr::formalism::FunctionExpression<LiftedTag>(value));
        const auto lifted_effect = ygg::formalism::insert(repository, lifted_effect_builder).first;

        auto lifted_condition_builder = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
        const auto lifted_condition = ygg::formalism::insert(repository, lifted_condition_builder).first;

        auto lifted_rule_builder = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::FunctionTag>>();
        lifted_rule_builder.body = lifted_condition.index();
        lifted_rule_builder.head = lifted_effect.index();
        const auto lifted_rule = ygg::formalism::insert(repository, lifted_rule_builder).first;

        auto binding_builder = urd::data<ygg::Row<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::FunctionTag>>>();
        binding_builder.relation = lifted_rule.index();
        const auto binding = ygg::formalism::insert(repository, binding_builder).first;

        auto effect_builder = urd::data<tyr::formalism::NumericEffect<GroundTag, tyr::formalism::FluentTag>>(
            op,
            head.index(),
            tyr::formalism::FunctionExpression<GroundTag>(value));
        const auto effect = ygg::formalism::insert(repository, effect_builder).first;

        auto rule_builder = urd::data<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::FunctionTag>>();
        rule_builder.row = binding.data();
        rule_builder.body = body.index();
        rule_builder.head = effect.index();
        rule_builder.cost = cost;
        const auto ground_rule = ygg::formalism::insert(repository, rule_builder).first;
        ground_function_rules.push_back(ground_rule.index());
        return ground_rule;
    }

    tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::FunctionTag>
    assign_rule(tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> body,
                tyr::formalism::datalog::FunctionTermView<GroundTag, tyr::formalism::FluentTag> head,
                ygg::float_t value,
                ygg::float_t cost = 0)
    { return numeric_rule(body, head, tyr::formalism::NumericEffectOperatorKind::Assign, value, cost); }

    tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::FunctionTag>
    empty_body_assign_rule(tyr::formalism::datalog::FunctionTermView<GroundTag, tyr::formalism::FluentTag> head, ygg::float_t value)
    { return assign_rule(condition(), head, value); }

    tyr::formalism::datalog::ProgramView<GroundTag> program()
    {
        auto program_builder = urd::data<tyr::formalism::datalog::Program<GroundTag>>();
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
        program_builder.predicate_rules.insert(program_builder.predicate_rules.end(), ground_rules.begin(), ground_rules.end());
        program_builder.function_rules.insert(program_builder.function_rules.end(), ground_function_rules.begin(), ground_function_rules.end());
        return ygg::formalism::insert(repository, program_builder).first;
    }
};

urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>>>
rule_indices(const urd::Store<urd::sequence<tyr::formalism::datalog::RuleView<GroundTag, tyr::formalism::PredicateTag>>>& rules)
{
    auto indices = urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>>> {};
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

    EXPECT_EQ(program.get_rules<tyr::formalism::PredicateTag>().size(), 1);
    EXPECT_EQ(program.get_rules<tyr::formalism::PredicateTag>()[0].index(), fixture.ground_rules[0]);
    EXPECT_NE(fmt::format("{}", program).find("GroundProgram"), std::string::npos);
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
    const auto term = fixture.fluent_function_term("n");
    fixture.empty_body_assign_rule(term, 3);
    fixture.rule(fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 1), atom);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_TRUE(ctx.out().facts().fact_sets.predicate.contains(atom.get_row()));
    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
    EXPECT_EQ(ctx.out().annotations().find(atom.get_row().data()), nullptr);
    EXPECT_EQ(ctx.out().numeric_annotations().size(), 0);
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
    const auto& dependencies = const_workspace.get_dependencies<tyr::formalism::PredicateTag>();
    const auto* a_rules = dependencies.fluent_precondition_to_rules.find(a.get_row().data());
    const auto* d_rules = dependencies.fluent_precondition_to_rules.find(d.get_row().data());

    ASSERT_NE(a_rules, nullptr);
    ASSERT_NE(d_rules, nullptr);
    EXPECT_EQ(rule_indices(*a_rules),
              (urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>>>(
                  { fixture.ground_rules[0], fixture.ground_rules[1] })));
    EXPECT_EQ(rule_indices(*d_rules),
              (urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>>>({ fixture.ground_rules[2] })));
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
    const auto& dependencies = const_workspace.get_dependencies<tyr::formalism::PredicateTag>();
    const auto* a_rules = dependencies.fluent_precondition_to_rules.find(a.get_row().data());

    ASSERT_NE(a_rules, nullptr);
    EXPECT_EQ(rule_indices(*a_rules),
              (urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>>>({ fixture.ground_rules[0] })));

    ctx.initialize(fixture.initial_fluent_atoms);
    EXPECT_EQ(ctx.out().scheduler().get_states<tyr::formalism::PredicateTag>()[fixture.ground_rules[0].value].unsatisfied_count, 0);
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
    EXPECT_EQ(ctx.out().scheduler().get_states<tyr::formalism::PredicateTag>()[fixture.ground_rules[0].value].unsatisfied_count, 1);
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
    EXPECT_EQ(ctx.out().scheduler().get_states<tyr::formalism::PredicateTag>()[fixture.ground_rules[1].value].unsatisfied_count, 0);
    EXPECT_EQ(ctx.out().scheduler().get_states<tyr::formalism::PredicateTag>()[fixture.ground_rules[2].value].unsatisfied_count, 1);
}

TEST(TyrDatalogGroundQueueTest, DuplicateHeadsDeriveFactOnce)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition(), a);

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
    cost_policy.set_cost(derive_a.get_row().data(), static_cast<datalog::Cost>(7));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<datalog::SumAggregation>,
                                                datalog::NoTerminationPolicy,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), datalog::NoTerminationPolicy(), cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    const auto* annotation = ctx.out().annotations().find(a.get_row().data());
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

TEST(TyrDatalogGroundQueueTest, GroundTerminationCommitsMixedLowestCostBucket)
{
    auto fixture = GroundQueueFixture();
    const auto goal_atom = fixture.fluent_atom("goal");
    const auto peer_atom = fixture.fluent_atom("peer");
    const auto expensive_source = fixture.fluent_atom("expensive_source");
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_atoms.push_back(expensive_source);
    fixture.rule(fixture.condition(), goal_atom);
    fixture.rule(fixture.condition(), peer_atom);
    fixture.rule(fixture.condition({ fixture.fluent_literal(expensive_source) }), goal_atom);
    fixture.empty_body_assign_rule(term, 3);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(fixture.condition({ fixture.fluent_literal(goal_atom) }));
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    ctx.out().annotations().insert_or_assign(expensive_source.get_row().data(), datalog::BaseAnnotation(static_cast<datalog::Cost>(2)));
    tyr::datalog::compute_model(ctx);

    EXPECT_TRUE(ctx.out().facts().fact_sets.predicate.contains(goal_atom.get_row()));
    EXPECT_TRUE(ctx.out().facts().fact_sets.predicate.contains(peer_atom.get_row()));
    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
    const auto* goal_annotation = ctx.out().annotations().find(goal_atom.get_row().data());
    ASSERT_NE(goal_annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*goal_annotation), 0);
    EXPECT_EQ(ctx.out().scheduler().statistics().num_rules_fired, 3);
}

TEST(TyrDatalogGroundQueueTest, GroundTerminationExposesOnlyOptimalGoalFrontierAchievers)
{
    auto fixture = GroundQueueFixture();
    const auto goal = fixture.fluent_atom("goal");
    const auto cheap = fixture.rule(fixture.condition(), goal, fixture.fresh_rule_binding(), 2);
    fixture.rule(fixture.condition(), goal, fixture.fresh_rule_binding(), 2);

    auto termination_policy = datalog::TerminationPolicy<datalog::MaxAggregation>();
    termination_policy.set_goals(fixture.condition({ fixture.fluent_literal(goal) }));
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(cheap.get_row().data(), static_cast<datalog::Cost>(1));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<datalog::MaxAggregation>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>(), termination_policy, cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    const auto* achievers = ctx.out().annotation_policy().find_achievers(goal.get_row().data());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_rule_key(), cheap.get_row().data());
}

TEST(TyrDatalogGroundQueueTest, GroundRetainsFirstEqualCostWitness)
{
    auto fixture = GroundQueueFixture();
    const auto goal = fixture.fluent_atom("goal");
    const auto canonical_first = fixture.fresh_rule_binding();
    const auto scheduled_first = fixture.fresh_rule_binding();
    fixture.rule(fixture.condition(), goal, scheduled_first);
    fixture.rule(fixture.condition(), goal, canonical_first);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace = datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::MaxAggregation>, datalog::NoTerminationPolicy>;
    auto workspace = Workspace(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    const auto* annotation = ctx.out().annotations().find(goal.get_row().data());
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<datalog::WitnessAnnotation<tyr::formalism::PredicateTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_rule_key(), scheduled_first.data());
}

TEST(TyrDatalogGroundQueueTest, AchieverPolicyGroundRecordsDistinctRuleBindings)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition(), a);
    const auto first_derive_b = fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);
    const auto second_derive_b = fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<datalog::MaxAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>(),
                               datalog::TerminationPolicy<datalog::MaxAggregation>());
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    const auto* achievers = ctx.out().annotation_policy().find_achievers(b.get_row().data());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 2);
    const auto first_key = (*achievers)[0].get_rule_key();
    const auto second_key = (*achievers)[1].get_rule_key();
    EXPECT_NE(first_key, second_key);
    EXPECT_TRUE((first_key == first_derive_b.get_row().data() && second_key == second_derive_b.get_row().data())
                || (first_key == second_derive_b.get_row().data() && second_key == first_derive_b.get_row().data()));
}

TEST(TyrDatalogGroundQueueTest, AchieverPolicyKeepsFirstEqualAndReplacesStrictlyCheaperWitness)
{
    auto fixture = GroundQueueFixture();
    const auto head = fixture.fluent_atom("head").get_row();
    const auto rule = fixture.fresh_rule_binding();
    auto policy = datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>();
    policy.initialize(static_cast<ygg::uint_t>(head.data().relation.value) + 1);

    policy.record_achiever(head.data(), datalog::WitnessAnnotation<tyr::formalism::PredicateTag>(rule.data(), static_cast<datalog::Cost>(2)));
    policy.record_achiever(head.data(), datalog::WitnessAnnotation<tyr::formalism::PredicateTag>(rule.data(), static_cast<datalog::Cost>(2)));

    const auto* achievers = policy.find_achievers(head.data());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_cost(), static_cast<datalog::Cost>(2));

    policy.record_achiever(head.data(), datalog::WitnessAnnotation<tyr::formalism::PredicateTag>(rule.data(), static_cast<datalog::Cost>(1)));
    achievers = policy.find_achievers(head.data());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_cost(), static_cast<datalog::Cost>(1));
}

TEST(TyrDatalogGroundQueueTest, DerivedNumericIntervalUnblocksRuleAndRecordsSupport)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    const auto head = fixture.fluent_atom("a");
    fixture.empty_body_assign_rule(term, 3);
    const auto derive_head = fixture.rule(fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 1), head);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<datalog::MaxAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>(),
                               datalog::TerminationPolicy<datalog::MaxAggregation>());
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    EXPECT_EQ(ctx.out().scheduler().get_states<tyr::formalism::PredicateTag>()[static_cast<ygg::uint_t>(derive_head.index().value)].unsatisfied_count, 1);

    tyr::datalog::compute_model(ctx);

    EXPECT_TRUE(ctx.out().facts().fact_sets.predicate.contains(head.get_row()));
    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
    const auto* achievers = ctx.out().annotation_policy().find_achievers(head.get_row().data());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    const auto& supports = achievers->front().get_numeric_supports();
    ASSERT_EQ(supports.size(), 1);
    EXPECT_EQ(supports.front().get_key(), term.get_row().data());
    EXPECT_EQ(supports.front().get_interval(), ygg::ClosedInterval<ygg::float_t>(3, 3));
}

TEST(TyrDatalogGroundQueueTest, SameCostBroadAndContainedNumericIntervalsRetainExactAnnotations)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 0);
    fixture.initial_fluent_function_value(term, 2);
    fixture.numeric_rule(fixture.condition(), term, tyr::formalism::NumericEffectOperatorKind::ScaleUp, 2, 1);
    fixture.assign_rule(fixture.condition(), term, 3, 1);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 3));
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    const auto broad = ygg::ClosedInterval<ygg::float_t>(0, 4);
    const auto contained = ygg::ClosedInterval<ygg::float_t>(3, 3);
    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], broad);
    EXPECT_NE(ctx.out().numeric_annotations().find(term.get_row().data(), broad), nullptr);
    EXPECT_NE(ctx.out().numeric_annotations().find(term.get_row().data(), contained), nullptr);
}

TEST(TyrDatalogGroundQueueTest, RevalidatesQueuedRuleAfterContainedNumericCertificateImproves)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    const auto head = fixture.fluent_atom("head");
    fixture.initial_fluent_function_value(term, 1);
    fixture.empty_body_assign_rule(term, 1);
    fixture.rule(fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 1), head);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace = datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::NoTerminationPolicy>;
    auto workspace = Workspace(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);

    const auto interval = ygg::ClosedInterval<ygg::float_t>(1, 1);
    ctx.out().numeric_annotations().clear();
    ctx.out().numeric_annotations().insert(term.get_row().data(), interval, datalog::BaseAnnotation(static_cast<datalog::Cost>(5)));

    tyr::datalog::compute_model(ctx);

    const auto* annotation = ctx.out().annotations().find(head.get_row().data());
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 0);
    EXPECT_EQ(ctx.out().scheduler().statistics().num_stale_queue_pops, 1);
}

TEST(TyrDatalogGroundQueueTest, EvaluatesReactivatedNumericRuleBeforeTargetHullChanges)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    const auto prerequisite = fixture.fluent_atom("prerequisite");
    fixture.initial_fluent_function_value(term, 0);
    fixture.assign_rule(fixture.condition(), term, 7, 5);
    fixture.assign_rule(fixture.condition(), term, 20, 7);
    fixture.rule(fixture.condition(), prerequisite, fixture.fresh_rule_binding(), 6);
    fixture.numeric_rule(fixture.condition({ fixture.fluent_literal(prerequisite) }), term, tyr::formalism::NumericEffectOperatorKind::Increase, 20);

    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 27));
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    const auto interval = ygg::ClosedInterval<ygg::float_t>(20, 27);
    const auto* annotation = ctx.out().numeric_annotations().find(term.get_row().data(), interval);
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 11);
    EXPECT_TRUE(ctx.out().tp().check(datalog::FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }));
}

TEST(TyrDatalogGroundQueueTest, ReevaluatesPendingPredicateWhenNumericSupportBecomesCheaper)
{
    auto fixture = GroundQueueFixture();
    const auto goal = fixture.fluent_atom("goal");
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 5);
    fixture.assign_rule(fixture.condition(), term, 15, 6);
    fixture.rule(fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 5), goal);

    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(fixture.condition({ fixture.fluent_literal(goal) }));
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    ctx.out().numeric_annotations().clear();
    ctx.out().numeric_annotations().insert(term.get_row().data(),
                                           ygg::ClosedInterval<ygg::float_t>(5, 5),
                                           datalog::BaseAnnotation(static_cast<datalog::Cost>(10)));
    tyr::datalog::compute_model(ctx);

    const auto* annotation = ctx.out().annotations().find(goal.get_row().data());
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 6);
}

TEST(TyrDatalogGroundQueueTest, NumericTransitionCreditOnlyReducesTheLocalEdge)
{
    auto fixture = GroundQueueFixture();
    const auto prerequisite = fixture.fluent_atom("prerequisite");
    fixture.initial_fluent_atoms.push_back(prerequisite);
    const auto term = fixture.fluent_function_term("n");
    const auto rule = fixture.assign_rule(fixture.condition({ fixture.fluent_literal(prerequisite) }), term, 2, 3);
    const auto interval = ygg::ClosedInterval<ygg::float_t>(2, 2);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(rule.get_row().data(), term.get_row().data(), interval, static_cast<datalog::Cost>(3));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<datalog::SumAggregation>,
                                                datalog::NoTerminationPolicy,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), datalog::NoTerminationPolicy(), cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    ctx.out().annotations().insert_or_assign(prerequisite.get_row().data(), datalog::BaseAnnotation(static_cast<datalog::Cost>(5)));

    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], interval);
    const auto* annotation = ctx.out().numeric_annotations().find(term.get_row().data(), interval);
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 5);
}

TEST(TyrDatalogGroundQueueTest, TransitionCreditDoesNotTurnRawPositiveEdgeIntoFreeWidening)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 0);
    const auto rule = fixture.numeric_rule(fixture.condition(), term, tyr::formalism::NumericEffectOperatorKind::Increase, 1, 1);
    const auto raw_interval = ygg::ClosedInterval<ygg::float_t>(1, 1);
    const auto goal = fixture.numeric_condition(term, tyr::formalism::BooleanOperatorKind::Ge, 1);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(rule.get_row().data(), term.get_row().data(), raw_interval, static_cast<datalog::Cost>(1));
    auto termination_policy = datalog::TerminationPolicy<datalog::SumAggregation>();
    termination_policy.set_goals(goal);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<datalog::SumAggregation>,
                                                datalog::TerminationPolicy<datalog::SumAggregation>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>(), termination_policy, cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], ygg::ClosedInterval<ygg::float_t>(0, 1));
    EXPECT_TRUE(ctx.out().tp().check(datalog::FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }));
}

TEST(TyrDatalogGroundQueueTest, WideningPolicyRequiresLabelPreservationForSum)
{
    const auto sum_policy = datalog::MinCostAnnotationPolicy<datalog::SumAggregation>();
    const auto max_policy = datalog::MinCostAnnotationPolicy<datalog::MaxAggregation>();

    EXPECT_TRUE(sum_policy.is_widening_label_preserving(5, 5));
    EXPECT_FALSE(sum_policy.is_widening_label_preserving(6, 5));
    EXPECT_TRUE(max_policy.is_widening_label_preserving(6, 5));
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

    auto repository = tyr::formalism::datalog::Repository();
    const auto program = tyr::datalog::remove_statically_inapplicable_rules(source_program, repository);
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    EXPECT_EQ(const_workspace.program.get_rules<tyr::formalism::PredicateTag>().size(), 2);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    tyr::datalog::compute_model(ctx);

    auto expected = PredicateRowViews {};
    for (const auto rule : program.get_rules<tyr::formalism::PredicateTag>())
        expected.push_back(rule.get_heads().front().get_row());
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
    ctx.out().facts().fact_sets.function.insert(term, 9);

    ctx.initialize(fixture.initial_fluent_atoms);

    EXPECT_EQ(ctx.out().facts().fact_sets.function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
}

TEST(TyrDatalogGroundQueueTest, PredicateRuleWithNegativeFluentLiteralIsRejected)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition({ fixture.fluent_literal(a, false) }), b);

    EXPECT_THROW(datalog::ConstProgramWorkspace<GroundTag>(fixture.program()), std::invalid_argument);
}

TEST(TyrDatalogGroundQueueTest, FunctionRuleWithNegativeFluentLiteralIsRejected)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto term = fixture.fluent_function_term("n");
    fixture.assign_rule(fixture.condition({ fixture.fluent_literal(a, false) }), term, 3);

    EXPECT_THROW(datalog::ConstProgramWorkspace<GroundTag>(fixture.program()), std::invalid_argument);
}

}
