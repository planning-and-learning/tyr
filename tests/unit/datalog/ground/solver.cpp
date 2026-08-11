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

#include "tyr/datalog/ground/solver.hpp"

#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace dq = tyr::datalog;

namespace tyr::tests
{
namespace
{
using GroundAtomViews = std::vector<fd::GroundAtomView<f::FluentTag>>;
using StaticGroundAtomViews = std::vector<fd::GroundAtomView<f::StaticTag>>;
using PredicateBindingViews = std::vector<fd::PredicateBindingView<f::FluentTag>>;

template<typename Context>
PredicateBindingViews binding_views(const Context& ctx)
{
    auto result = PredicateBindingViews {};
    for (const auto& set : ctx.out().fact_sets().predicate.get_sets())
        for (const auto binding : set.get_bindings())
            result.push_back(binding);
    return result;
}

PredicateBindingViews binding_views(std::initializer_list<fd::GroundAtomView<f::FluentTag>> atoms)
{
    auto result = PredicateBindingViews {};
    for (const auto atom : atoms)
        result.push_back(atom.get_row());
    return result;
}

struct GroundQueueFixture
{
    fd::RepositoryFactory factory;
    fd::Repository repository = factory.create();
    std::vector<ygg::Index<f::Predicate<f::StaticTag>>> static_predicates;
    std::vector<ygg::Index<f::Predicate<f::FluentTag>>> fluent_predicates;
    std::vector<ygg::Index<f::Function<f::FluentTag>>> fluent_functions;
    StaticGroundAtomViews initial_static_atoms;
    GroundAtomViews initial_fluent_atoms;
    std::vector<ygg::Index<fd::GroundFunctionTermValue<f::FluentTag>>> initial_fluent_fterm_values;
    std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>> ground_rules;
    std::vector<ygg::Index<fd::GroundRule<f::FunctionTag>>> ground_function_rules;
    ygg::uint_t next_rule_id = 0;

    fd::GroundAtomView<f::FluentTag> fluent_atom(const std::string& name)
    {
        auto predicate_builder = ygg::Data<f::Predicate<f::FluentTag>>(name, 0);
        canonicalize(predicate_builder);
        const auto [predicate, predicate_inserted] = repository.get_or_create(predicate_builder);
        if (predicate_inserted)
            fluent_predicates.push_back(predicate.get_index());

        auto binding_builder = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>>();
        binding_builder.relation = predicate.get_index();
        canonicalize(binding_builder);
        const auto [binding, binding_inserted] = repository.get_or_create(binding_builder);
        (void) binding_inserted;

        auto atom_builder = ygg::Data<fd::GroundAtom<f::FluentTag>>(binding.get_index());
        canonicalize(atom_builder);
        return repository.get_or_create(atom_builder).first;
    }

    fd::GroundLiteralView<f::FluentTag> fluent_literal(fd::GroundAtomView<f::FluentTag> atom, bool polarity = true)
    {
        auto literal_builder = ygg::Data<fd::GroundLiteral<f::FluentTag>>(atom.get_index(), polarity);
        canonicalize(literal_builder);
        return repository.get_or_create(literal_builder).first;
    }

    fd::GroundAtomView<f::StaticTag> static_atom(const std::string& name)
    {
        auto predicate_builder = ygg::Data<f::Predicate<f::StaticTag>>(name, 0);
        canonicalize(predicate_builder);
        const auto [predicate, predicate_inserted] = repository.get_or_create(predicate_builder);
        if (predicate_inserted)
            static_predicates.push_back(predicate.get_index());

        auto binding_builder = ygg::Data<f::RelationBinding<f::Predicate<f::StaticTag>>>();
        binding_builder.relation = predicate.get_index();
        canonicalize(binding_builder);
        const auto binding = repository.get_or_create(binding_builder).first;

        auto atom_builder = ygg::Data<fd::GroundAtom<f::StaticTag>>(binding.get_index());
        canonicalize(atom_builder);
        return repository.get_or_create(atom_builder).first;
    }

    fd::GroundLiteralView<f::StaticTag> static_literal(fd::GroundAtomView<f::StaticTag> atom, bool polarity = true)
    {
        auto literal_builder = ygg::Data<fd::GroundLiteral<f::StaticTag>>(atom.get_index(), polarity);
        canonicalize(literal_builder);
        return repository.get_or_create(literal_builder).first;
    }

    fd::GroundFunctionTermView<f::FluentTag> fluent_function_term(const std::string& name)
    {
        auto function_builder = ygg::Data<f::Function<f::FluentTag>>(name, 0);
        canonicalize(function_builder);
        const auto function = repository.get_or_create(function_builder).first;
        fluent_functions.push_back(function.get_index());

        auto binding_builder = ygg::Data<f::RelationBinding<f::Function<f::FluentTag>>>();
        binding_builder.relation = function.get_index();
        canonicalize(binding_builder);
        const auto binding = repository.get_or_create(binding_builder).first;

        auto term_builder = ygg::Data<fd::GroundFunctionTerm<f::FluentTag>>(binding.get_index());
        canonicalize(term_builder);
        return repository.get_or_create(term_builder).first;
    }

    void initial_fluent_function_value(fd::GroundFunctionTermView<f::FluentTag> term, ygg::float_t value)
    {
        auto value_builder = ygg::Data<fd::GroundFunctionTermValue<f::FluentTag>>(term.get_index(), value);
        canonicalize(value_builder);
        initial_fluent_fterm_values.push_back(repository.get_or_create(value_builder).first.get_index());
    }

    fd::GroundConjunctiveConditionView condition(std::initializer_list<fd::GroundLiteralView<f::FluentTag>> fluent_literals = {},
                                                 std::initializer_list<fd::GroundLiteralView<f::StaticTag>> static_literals = {})
    {
        auto condition_builder = ygg::Data<fd::GroundConjunctiveCondition>();
        for (const auto literal : fluent_literals)
            condition_builder.fluent_literals.push_back(literal.get_index());
        for (const auto literal : static_literals)
            condition_builder.static_literals.push_back(literal.get_index());
        canonicalize(condition_builder);
        return repository.get_or_create(condition_builder).first;
    }

    fd::GroundConjunctiveConditionView numeric_condition(fd::GroundFunctionTermView<f::FluentTag> term, f::BooleanOperatorKind op, ygg::float_t value)
    {
        auto comparison_builder = ygg::Data<fd::GroundBinaryOperatorType<f::BooleanOperatorKind>>(op,
                                                                                                  ygg::Data<fd::GroundFunctionExpression>(term.get_index()),
                                                                                                  ygg::Data<fd::GroundFunctionExpression>(value));
        canonicalize(comparison_builder);
        const auto comparison = repository.get_or_create(comparison_builder).first;

        auto condition_builder = ygg::Data<fd::GroundConjunctiveCondition>();
        condition_builder.numeric_constraints.emplace_back(op, fd::GroundBooleanOperatorData::Variant(comparison.get_index()));
        canonicalize(condition_builder);
        return repository.get_or_create(condition_builder).first;
    }

    fd::RuleBindingView<f::PredicateTag> fresh_rule_binding()
    {
        auto predicate_builder = ygg::Data<f::Predicate<f::FluentTag>>("dummy_" + std::to_string(next_rule_id++), 0);
        canonicalize(predicate_builder);
        const auto predicate = repository.get_or_create(predicate_builder).first;

        auto atom_builder = ygg::Data<fd::Atom<f::FluentTag>>();
        atom_builder.predicate = predicate.get_index();
        canonicalize(atom_builder);
        const auto atom = repository.get_or_create(atom_builder).first;

        auto condition_builder = ygg::Data<fd::ConjunctiveCondition>();
        canonicalize(condition_builder);
        const auto lifted_condition = repository.get_or_create(condition_builder).first;

        auto rule_builder = ygg::Data<fd::Rule<f::PredicateTag>>();
        rule_builder.body = lifted_condition.get_index();
        rule_builder.head = atom.get_index();
        canonicalize(rule_builder);
        const auto rule = repository.get_or_create(rule_builder).first;

        auto binding_builder = ygg::Data<f::RelationBinding<fd::Rule<f::PredicateTag>>>();
        binding_builder.relation = rule.get_index();
        canonicalize(binding_builder);
        return repository.get_or_create(binding_builder).first;
    }

    fd::GroundRuleView<f::PredicateTag> rule(fd::GroundConjunctiveConditionView body, fd::GroundAtomView<f::FluentTag> head)
    {
        auto rule_builder = ygg::Data<fd::GroundRule<f::PredicateTag>>();
        rule_builder.binding = fresh_rule_binding().get_index();
        rule_builder.body = body.get_index();
        rule_builder.head = head.get_index();
        canonicalize(rule_builder);
        const auto ground_rule = repository.get_or_create(rule_builder).first;
        ground_rules.push_back(ground_rule.get_index());
        return ground_rule;
    }

    fd::GroundRuleView<f::FunctionTag> numeric_rule(fd::GroundConjunctiveConditionView body,
                                                    fd::GroundFunctionTermView<f::FluentTag> head,
                                                    f::NumericEffectOperatorKind op,
                                                    ygg::float_t value,
                                                    ygg::float_t metric_delta = 0)
    {
        auto lifted_term_builder = ygg::Data<fd::FunctionTerm<f::FluentTag>>();
        lifted_term_builder.function = head.get_function().get_index();
        canonicalize(lifted_term_builder);
        const auto lifted_term = repository.get_or_create(lifted_term_builder).first;

        auto lifted_effect_builder = ygg::Data<fd::NumericEffect<f::FluentTag>>(op, lifted_term.get_index(), ygg::Data<fd::FunctionExpression>(value));
        canonicalize(lifted_effect_builder);
        const auto lifted_effect = repository.get_or_create(lifted_effect_builder).first;

        auto lifted_condition_builder = ygg::Data<fd::ConjunctiveCondition>();
        canonicalize(lifted_condition_builder);
        const auto lifted_condition = repository.get_or_create(lifted_condition_builder).first;

        auto lifted_rule_builder = ygg::Data<fd::Rule<f::FunctionTag>>();
        lifted_rule_builder.body = lifted_condition.get_index();
        lifted_rule_builder.head = ygg::Data<fd::NumericEffectOperator<f::FluentTag>>(op, lifted_effect.get_index());
        if (metric_delta != 0)
        {
            auto metric_effect_builder = ygg::Data<fd::NumericEffect<f::FluentTag>>(f::NumericEffectOperatorKind::Increase,
                                                                                    lifted_term.get_index(),
                                                                                    ygg::Data<fd::FunctionExpression>(metric_delta));
            canonicalize(metric_effect_builder);
            const auto metric_effect = repository.get_or_create(metric_effect_builder).first;
            lifted_rule_builder.metric_effects.emplace_back(f::NumericEffectOperatorKind::Increase,
                                                            ygg::Data<fd::NumericEffectOperator<f::FluentTag>>::Variant(metric_effect.get_index()));
        }
        canonicalize(lifted_rule_builder);
        const auto lifted_rule = repository.get_or_create(lifted_rule_builder).first;

        auto binding_builder = ygg::Data<f::RelationBinding<fd::Rule<f::FunctionTag>>>();
        binding_builder.relation = lifted_rule.get_index();
        canonicalize(binding_builder);
        const auto binding = repository.get_or_create(binding_builder).first;

        auto effect_builder = ygg::Data<fd::GroundNumericEffect<f::FluentTag>>(op, head.get_index(), ygg::Data<fd::GroundFunctionExpression>(value));
        canonicalize(effect_builder);
        const auto effect = repository.get_or_create(effect_builder).first;

        auto rule_builder = ygg::Data<fd::GroundRule<f::FunctionTag>>();
        rule_builder.binding = binding.get_index();
        rule_builder.body = body.get_index();
        rule_builder.head = ygg::Data<fd::GroundNumericEffectOperator<f::FluentTag>>(op, effect.get_index());
        if (metric_delta != 0)
        {
            auto metric_effect_builder = ygg::Data<fd::GroundNumericEffect<f::FluentTag>>(f::NumericEffectOperatorKind::Increase,
                                                                                          head.get_index(),
                                                                                          ygg::Data<fd::GroundFunctionExpression>(metric_delta));
            canonicalize(metric_effect_builder);
            const auto metric_effect = repository.get_or_create(metric_effect_builder).first;
            rule_builder.metric_effects.emplace_back(f::NumericEffectOperatorKind::Increase,
                                                     ygg::Data<fd::GroundNumericEffectOperator<f::FluentTag>>::Variant(metric_effect.get_index()));
        }
        canonicalize(rule_builder);
        const auto ground_rule = repository.get_or_create(rule_builder).first;
        ground_function_rules.push_back(ground_rule.get_index());
        return ground_rule;
    }

    fd::GroundRuleView<f::FunctionTag>
    assign_rule(fd::GroundConjunctiveConditionView body, fd::GroundFunctionTermView<f::FluentTag> head, ygg::float_t value, ygg::float_t metric_delta = 0)
    {
        return numeric_rule(body, head, f::NumericEffectOperatorKind::Assign, value, metric_delta);
    }

    fd::GroundRuleView<f::FunctionTag> empty_body_assign_rule(fd::GroundFunctionTermView<f::FluentTag> head, ygg::float_t value)
    {
        return assign_rule(condition(), head, value);
    }

    fd::ProgramView<GroundTag> program()
    {
        auto program_builder = ygg::Data<fd::GroundProgram>();
        program_builder.static_predicates.insert(program_builder.static_predicates.end(), static_predicates.begin(), static_predicates.end());
        program_builder.fluent_predicates.insert(program_builder.fluent_predicates.end(), fluent_predicates.begin(), fluent_predicates.end());
        program_builder.fluent_functions.insert(program_builder.fluent_functions.end(), fluent_functions.begin(), fluent_functions.end());
        for (const auto atom : initial_static_atoms)
            program_builder.static_atoms.push_back(atom.get_index());
        for (const auto atom : initial_fluent_atoms)
            program_builder.fluent_atoms.push_back(atom.get_index());
        program_builder.fluent_fterm_values.insert(program_builder.fluent_fterm_values.end(),
                                                   initial_fluent_fterm_values.begin(),
                                                   initial_fluent_fterm_values.end());
        program_builder.predicate_ground_rules.insert(program_builder.predicate_ground_rules.end(), ground_rules.begin(), ground_rules.end());
        program_builder.function_ground_rules.insert(program_builder.function_ground_rules.end(), ground_function_rules.begin(), ground_function_rules.end());
        canonicalize(program_builder);
        return repository.get_or_create(program_builder).first;
    }
};

std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>> rule_indices(const std::vector<fd::GroundRuleView<f::PredicateTag>>& rules)
{
    auto indices = std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>> {};
    for (const auto rule : rules)
        indices.push_back(rule.get_index());
    return indices;
}

struct SolvedGroundQueue
{
    PredicateBindingViews fluent_bindings;
    datalog::GroundQueueStatistics statistics;
};

SolvedGroundQueue solve_default_state(GroundQueueFixture& fixture)
{
    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);
    return { binding_views(ctx), ctx.out().statistics() };
}
}

TEST(TyrDatalogGroundQueueTest, GroundProgramStoresGroundRules)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), atom);

    const auto program = fixture.program();

    EXPECT_EQ(program.get_rules<f::PredicateTag>().size(), 1);
    EXPECT_EQ(program.get_rules<f::PredicateTag>()[0].get_index(), fixture.ground_rules[0]);
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
    fixture.rule(fixture.numeric_condition(term, f::BooleanOperatorKind::Ge, 1), atom);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    EXPECT_TRUE(ctx.out().fact_sets().predicate.contains(atom.get_row()));
    EXPECT_EQ(ctx.out().fact_sets().function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
    EXPECT_EQ(ctx.out().annotations().find(atom.get_row()), nullptr);
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
    dq::compute_model(ctx);
    const auto first_statistics = ctx.out().statistics();
    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    EXPECT_EQ(first_statistics.num_facts_derived, 2);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);
    const auto second_statistics = ctx.out().statistics();
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
    const auto& dependencies = const_workspace.get_dependencies<f::PredicateTag>();
    const auto* a_rules = dependencies.fluent_precondition_to_rules.find(a.get_row());
    const auto* d_rules = dependencies.fluent_precondition_to_rules.find(d.get_row());

    ASSERT_NE(a_rules, nullptr);
    ASSERT_NE(d_rules, nullptr);
    EXPECT_EQ(rule_indices(*a_rules), std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>>({ fixture.ground_rules[0], fixture.ground_rules[1] }));
    EXPECT_EQ(rule_indices(*d_rules), std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>>({ fixture.ground_rules[2] }));
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
    const auto& dependencies = const_workspace.get_dependencies<f::PredicateTag>();
    const auto* a_rules = dependencies.fluent_precondition_to_rules.find(a.get_row());

    ASSERT_NE(a_rules, nullptr);
    EXPECT_EQ(rule_indices(*a_rules), std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>>({ fixture.ground_rules[0] }));

    ctx.initialize(fixture.initial_fluent_atoms);
    EXPECT_EQ(ctx.out().rule_states<f::PredicateTag>()[fixture.ground_rules[0].get_value()].unsatisfied_count, 0);
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
    EXPECT_EQ(ctx.out().rule_states<f::PredicateTag>()[fixture.ground_rules[0].get_value()].unsatisfied_count, 1);
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
    dq::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    EXPECT_EQ(ctx.out().rule_states<f::PredicateTag>()[fixture.ground_rules[1].get_value()].unsatisfied_count, 0);
    EXPECT_EQ(ctx.out().rule_states<f::PredicateTag>()[fixture.ground_rules[2].get_value()].unsatisfied_count, 1);
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

TEST(TyrDatalogGroundQueueTest, GroundUsedCostOverrideDoesNotCreateMetricEffectCost)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto derive_a = fixture.rule(fixture.condition(), a);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(derive_a.get_row(), datalog::Cost(7));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::NoTerminationPolicy<GroundTag>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(),
                               datalog::NoTerminationPolicy<GroundTag>(),
                               cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    const auto* annotation = ctx.out().annotations().find(a.get_row());
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
    auto termination_policy = datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>();
    termination_policy.set_goals(goal);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ goal_atom }));
    EXPECT_EQ(ctx.out().statistics().num_rules_fired, 0);
    EXPECT_EQ(ctx.out().statistics().num_facts_derived, 0);
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
    auto termination_policy = datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>();
    termination_policy.set_goals(goal);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a }));
    EXPECT_EQ(ctx.out().statistics().num_rules_fired, 1);
    EXPECT_TRUE(ctx.out().tp().check(datalog::FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }));
}

TEST(TyrDatalogGroundQueueTest, NegativeFluentGoalIsRejected)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    const auto goal = fixture.condition({ fixture.fluent_literal(atom, false) });

    auto termination_policy = datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>();
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
    const auto expensive_goal = fixture.rule(fixture.condition({ fixture.fluent_literal(expensive_source) }), goal_atom);
    fixture.empty_body_assign_rule(term, 3);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto termination_policy = datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>();
    termination_policy.set_goals(fixture.condition({ fixture.fluent_literal(goal_atom) }));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    ctx.out().annotations().insert_or_assign(expensive_source.get_row(), datalog::BaseAnnotation<GroundTag>(datalog::Cost(2)));
    dq::compute_model(ctx);

    EXPECT_TRUE(ctx.out().fact_sets().predicate.contains(goal_atom.get_row()));
    EXPECT_TRUE(ctx.out().fact_sets().predicate.contains(peer_atom.get_row()));
    EXPECT_EQ(ctx.out().fact_sets().function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
    const auto* goal_annotation = ctx.out().annotations().find(goal_atom.get_row());
    ASSERT_NE(goal_annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*goal_annotation), 0);
    EXPECT_FALSE(ctx.out().rule_states<f::PredicateTag>()[ygg::uint_t(expensive_goal.get_index())].fired);
    EXPECT_EQ(ctx.out().statistics().num_rules_fired, 3);
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
                                                datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>(),
                               datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>());
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    EXPECT_EQ(binding_views(ctx), binding_views({ a, b }));
    const auto* achievers = ctx.out().annotation_policy().find_achievers(b.get_row());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 2);
    const auto first_key = (*achievers)[0].get_rule_key();
    const auto second_key = (*achievers)[1].get_rule_key();
    EXPECT_NE(first_key, second_key);
    EXPECT_TRUE((first_key == first_derive_b.get_row() && second_key == second_derive_b.get_row())
                || (first_key == second_derive_b.get_row() && second_key == first_derive_b.get_row()));
}

TEST(TyrDatalogGroundQueueTest, DerivedNumericIntervalUnblocksRuleAndRecordsSupport)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    const auto head = fixture.fluent_atom("a");
    fixture.empty_body_assign_rule(term, 3);
    const auto derive_head = fixture.rule(fixture.numeric_condition(term, f::BooleanOperatorKind::Ge, 1), head);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationWithAchieversPolicy<GroundTag, datalog::MaxAggregation>(),
                               datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>());
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    EXPECT_EQ(ctx.out().rule_states<f::PredicateTag>()[ygg::uint_t(derive_head.get_index())].unsatisfied_count, 1);

    dq::compute_model(ctx);

    EXPECT_TRUE(ctx.out().fact_sets().predicate.contains(head.get_row()));
    EXPECT_EQ(ctx.out().fact_sets().function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
    const auto* achievers = ctx.out().annotation_policy().find_achievers(head.get_row());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    const auto& supports = achievers->front().get_numeric_supports();
    ASSERT_EQ(supports.size(), 1);
    EXPECT_EQ(supports.front().get_key(), term.get_row());
    EXPECT_EQ(supports.front().get_interval(), ygg::ClosedInterval<ygg::float_t>(3, 3));
}

TEST(TyrDatalogGroundQueueTest, SameCostBroadAndContainedNumericIntervalsRetainExactAnnotations)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 0);
    fixture.initial_fluent_function_value(term, 2);
    fixture.numeric_rule(fixture.condition(), term, f::NumericEffectOperatorKind::ScaleUp, 2, 1);
    fixture.assign_rule(fixture.condition(), term, 3, 1);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto termination_policy = datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>();
    termination_policy.set_goals(fixture.numeric_condition(term, f::BooleanOperatorKind::Ge, 3));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(), termination_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    const auto broad = ygg::ClosedInterval<ygg::float_t>(0, 4);
    const auto contained = ygg::ClosedInterval<ygg::float_t>(3, 3);
    EXPECT_EQ(ctx.out().fact_sets().function[term], broad);
    EXPECT_NE(ctx.out().numeric_annotations().find(term.get_row(), broad), nullptr);
    EXPECT_NE(ctx.out().numeric_annotations().find(term.get_row(), contained), nullptr);
}

TEST(TyrDatalogGroundQueueTest, RevalidatesQueuedRuleAfterContainedNumericCertificateImproves)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    const auto head = fixture.fluent_atom("head");
    fixture.initial_fluent_function_value(term, 1);
    fixture.empty_body_assign_rule(term, 1);
    fixture.rule(fixture.numeric_condition(term, f::BooleanOperatorKind::Ge, 1), head);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    using Workspace =
        datalog::ProgramWorkspace<GroundTag, datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>, datalog::NoTerminationPolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);

    const auto interval = ygg::ClosedInterval<ygg::float_t>(1, 1);
    ctx.out().numeric_annotations().clear();
    ctx.out().numeric_annotations().insert(term.get_row(), interval, datalog::BaseAnnotation<GroundTag>(datalog::Cost(5)));

    dq::compute_model(ctx);

    const auto* annotation = ctx.out().annotations().find(head.get_row());
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 0);
    EXPECT_EQ(ctx.out().statistics().num_stale_queue_pops, 1);
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
    cost_policy.set_cost(rule.get_row(), term.get_row(), interval, datalog::Cost(3));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::NoTerminationPolicy<GroundTag>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace,
                               datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(),
                               datalog::NoTerminationPolicy<GroundTag>(),
                               cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    ctx.out().annotations().insert_or_assign(prerequisite.get_row(),
                                             datalog::BaseAnnotation<GroundTag>(ygg::ClosedInterval<ygg::float_t>(5, 5), datalog::Cost(5)));

    dq::compute_model(ctx);

    EXPECT_EQ(ctx.out().fact_sets().function[term], interval);
    const auto* annotation = ctx.out().numeric_annotations().find(term.get_row(), interval);
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 5);
    EXPECT_EQ(datalog::get_metric(*annotation), ygg::ClosedInterval<ygg::float_t>(5, 5));
}

TEST(TyrDatalogGroundQueueTest, TransitionCreditDoesNotTurnRawPositiveEdgeIntoFreeWidening)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 0);
    const auto rule = fixture.numeric_rule(fixture.condition(), term, f::NumericEffectOperatorKind::Increase, 1, 1);
    const auto raw_interval = ygg::ClosedInterval<ygg::float_t>(1, 1);
    const auto goal = fixture.numeric_condition(term, f::BooleanOperatorKind::Ge, 1);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto cost_policy = datalog::RuleCostOverridePolicy<GroundTag>();
    cost_policy.set_cost(rule.get_row(), term.get_row(), raw_interval, datalog::Cost(1));
    auto termination_policy = datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>();
    termination_policy.set_goals(goal);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace, datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>(), termination_policy, cost_policy);
    auto ctx = datalog::ProgramExecutionContext(workspace);

    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    EXPECT_EQ(ctx.out().fact_sets().function[term], ygg::ClosedInterval<ygg::float_t>(0, 1));
    EXPECT_TRUE(ctx.out().tp().check(datalog::FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }));
}

TEST(TyrDatalogGroundQueueTest, WideningPolicyRequiresLabelPreservationForSum)
{
    const auto sum_policy = datalog::MinCostAnnotationPolicy<GroundTag, datalog::SumAggregation>();
    const auto max_policy = datalog::MinCostAnnotationPolicy<GroundTag, datalog::MaxAggregation>();

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

    auto repository = fixture.factory.create();
    const auto program = dq::remove_statically_inapplicable_rules(source_program, repository);
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    EXPECT_EQ(const_workspace.program.get_rules<f::PredicateTag>().size(), 2);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.initialize(fixture.initial_fluent_atoms);
    dq::compute_model(ctx);

    auto expected = PredicateBindingViews {};
    for (const auto rule : program.get_rules<f::PredicateTag>())
        expected.push_back(rule.get_head().get_row());
    EXPECT_EQ(binding_views(ctx), expected);
    EXPECT_EQ(ctx.out().statistics().num_rules_fired, 2);
}

TEST(TyrDatalogGroundQueueTest, ExplicitFluentAtomsRestoreDeclaredInitialFunctionValues)
{
    auto fixture = GroundQueueFixture();
    const auto term = fixture.fluent_function_term("n");
    fixture.initial_fluent_function_value(term, 3);

    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(fixture.program());
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto ctx = datalog::ProgramExecutionContext(workspace);
    ctx.out().fact_sets().function.insert(term, 9);

    ctx.initialize(fixture.initial_fluent_atoms);

    EXPECT_EQ(ctx.out().fact_sets().function[term], ygg::ClosedInterval<ygg::float_t>(3, 3));
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
