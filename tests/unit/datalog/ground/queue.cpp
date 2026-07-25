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

#include "tyr/datalog/ground/queue.hpp"

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <algorithm>
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
struct GroundQueueFixture
{
    fd::RepositoryFactory factory;
    fd::Repository repository = factory.create();
    std::vector<ygg::Index<f::Predicate<f::FluentTag>>> fluent_predicates;
    std::vector<ygg::Index<fd::GroundAtom<f::FluentTag>>> initial_fluent_atoms;
    std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>> ground_rules;
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

    fd::GroundConjunctiveConditionView condition(std::initializer_list<fd::GroundLiteralView<f::FluentTag>> fluent_literals = {})
    {
        auto condition_builder = ygg::Data<fd::GroundConjunctiveCondition>();
        for (const auto literal : fluent_literals)
            condition_builder.fluent_literals.push_back(literal.get_index());
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

    fd::ProgramView<GroundTag> program()
    {
        auto program_builder = ygg::Data<fd::GroundProgram>();
        program_builder.fluent_predicates.insert(program_builder.fluent_predicates.end(), fluent_predicates.begin(), fluent_predicates.end());
        program_builder.fluent_atoms.insert(program_builder.fluent_atoms.end(), initial_fluent_atoms.begin(), initial_fluent_atoms.end());
        program_builder.predicate_ground_rules.insert(program_builder.predicate_ground_rules.end(), ground_rules.begin(), ground_rules.end());
        canonicalize(program_builder);
        return repository.get_or_create(program_builder).first;
    }
};

std::vector<ygg::uint_t> atom_indices(const ygg::UnorderedSet<fd::GroundAtomView<f::FluentTag>>& atoms)
{
    auto indices = std::vector<ygg::uint_t> {};
    for (const auto atom : atoms)
        indices.push_back(ygg::uint_t(atom.get_index()));
    std::sort(indices.begin(), indices.end());
    return indices;
}

std::vector<ygg::uint_t> expected_indices(std::initializer_list<fd::GroundAtomView<f::FluentTag>> atoms)
{
    auto indices = std::vector<ygg::uint_t> {};
    for (const auto atom : atoms)
        indices.push_back(ygg::uint_t(atom.get_index()));
    std::sort(indices.begin(), indices.end());
    return indices;
}

std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>> rule_indices(const std::vector<fd::GroundRuleView<f::PredicateTag>>& rules)
{
    auto indices = std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>> {};
    for (const auto rule : rules)
        indices.push_back(rule.get_index());
    return indices;
}

ygg::UnorderedSet<fd::GroundAtomView<f::FluentTag>> initial_fluent_atoms(GroundQueueFixture& fixture)
{
    auto result = ygg::UnorderedSet<fd::GroundAtomView<f::FluentTag>> {};
    for (const auto atom_index : fixture.initial_fluent_atoms)
        result.insert(ygg::make_view(atom_index, fixture.repository));
    return result;
}

struct SolvedGroundQueue
{
    std::vector<ygg::uint_t> atom_indices;
    datalog::GroundQueueStatistics statistics;
};

SolvedGroundQueue solve_default_state(GroundQueueFixture& fixture)
{
    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);
    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);
    return { atom_indices(ctx.out().fluent_atoms()), ctx.out().statistics() };
}
}

TEST(TyrDatalogGroundQueueTest, GroundProgramStoresGroundRules)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), atom);

    const auto program = fixture.program();

    EXPECT_EQ(program.get_ground_rules<f::PredicateTag>().size(), 1);
    EXPECT_EQ(program.get_ground_rules<f::PredicateTag>()[0].get_index(), fixture.ground_rules[0]);
    EXPECT_NE(fmt::format("{}", program).find("GroundProgram"), std::string::npos);
}

TEST(TyrDatalogGroundQueueTest, EmptyBodyRuleFires)
{
    auto fixture = GroundQueueFixture();
    const auto atom = fixture.fluent_atom("a");
    fixture.rule(fixture.condition(), atom);

    const auto result = solve_default_state(fixture);

    EXPECT_EQ(result.atom_indices, expected_indices({ atom }));
    EXPECT_EQ(result.statistics.num_rules_fired, 1);
    EXPECT_EQ(result.statistics.num_facts_derived, 1);
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

    EXPECT_EQ(result.atom_indices, expected_indices({ a, b, c }));
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
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);

    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);
    const auto first_statistics = ctx.out().statistics();
    EXPECT_EQ(atom_indices(ctx.out().fluent_atoms()), expected_indices({ a, b }));
    EXPECT_EQ(first_statistics.num_facts_derived, 2);

    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);
    const auto second_statistics = ctx.out().statistics();
    EXPECT_EQ(atom_indices(ctx.out().fluent_atoms()), expected_indices({ a, b }));
    EXPECT_EQ(second_statistics.num_facts_derived, 2);
    EXPECT_EQ(second_statistics.num_rules_fired, first_statistics.num_rules_fired);
}

TEST(TyrDatalogGroundQueueTest, MultiPreconditionRuleWaitsForAllFacts)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    const auto c = fixture.fluent_atom("c");
    fixture.initial_fluent_atoms.push_back(a.get_index());
    fixture.rule(fixture.condition(), b);
    fixture.rule(fixture.condition({ fixture.fluent_literal(a), fixture.fluent_literal(b) }), c);

    const auto result = solve_default_state(fixture);

    EXPECT_EQ(result.atom_indices, expected_indices({ a, b, c }));
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
    const auto a_it = dependencies.fluent_precondition_to_rules.find(a);
    const auto d_it = dependencies.fluent_precondition_to_rules.find(d);

    ASSERT_NE(a_it, dependencies.fluent_precondition_to_rules.end());
    ASSERT_NE(d_it, dependencies.fluent_precondition_to_rules.end());
    EXPECT_EQ(rule_indices(a_it->second), std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>>({ fixture.ground_rules[0], fixture.ground_rules[1] }));
    EXPECT_EQ(rule_indices(d_it->second), std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>>({ fixture.ground_rules[2] }));
}

TEST(TyrDatalogGroundQueueTest, InitialFluentFactsSatisfyDynamicUnsatisfiedCounts)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.initial_fluent_atoms.push_back(a.get_index());
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);
    const auto& dependencies = const_workspace.get_dependencies<f::PredicateTag>();
    const auto a_it = dependencies.fluent_precondition_to_rules.find(a);

    ASSERT_NE(a_it, dependencies.fluent_precondition_to_rules.end());
    EXPECT_EQ(rule_indices(a_it->second), std::vector<ygg::Index<fd::GroundRule<f::PredicateTag>>>({ fixture.ground_rules[0] }));

    ctx.initialize(initial_fluent_atoms(fixture));
    EXPECT_EQ(ctx.out().rule_states<f::PredicateTag>()[fixture.ground_rules[0].get_value()].unsatisfied_count, 0);
}

TEST(TyrDatalogGroundQueueTest, ExplicitFluentStateDrivesDynamicUnsatisfiedCounts)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.initial_fluent_atoms.push_back(a.get_index());
    fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    auto workspace = datalog::ProgramWorkspace<GroundTag>(const_workspace);
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);

    ctx.out().fluent_atoms().clear();
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
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);

    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);

    EXPECT_EQ(atom_indices(ctx.out().fluent_atoms()), expected_indices({ a, b }));
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

    EXPECT_EQ(result.atom_indices, expected_indices({ a }));
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
    cost_policy.set_cost(derive_a, datalog::Cost(7));
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::OrAnnotationPolicy<GroundTag>,
                                                datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::NoTerminationPolicy<GroundTag>,
                                                datalog::RuleCostOverridePolicy<GroundTag>>;
    auto workspace = Workspace(const_workspace,
                               datalog::OrAnnotationPolicy<GroundTag>(),
                               datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>(),
                               datalog::NoTerminationPolicy<GroundTag>(),
                               cost_policy);
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);

    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);

    EXPECT_EQ(atom_indices(ctx.out().fluent_atoms()), expected_indices({ a, b }));
    const auto* annotation = ctx.out().and_annot().find(a.get_row());
    ASSERT_NE(annotation, nullptr);
    EXPECT_EQ(datalog::get_cost(*annotation), 0);
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
                                                datalog::OrAnnotationPolicy<GroundTag>,
                                                datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::OrAnnotationPolicy<GroundTag>(),
                               datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>(),
                               termination_policy);
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);

    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);

    EXPECT_EQ(atom_indices(ctx.out().fluent_atoms()), expected_indices({ a }));
    EXPECT_EQ(ctx.out().statistics().num_rules_fired, 1);
    EXPECT_TRUE(ctx.out().tp().check(datalog::FactSets { ctx.out().facts().static_fact_sets, ctx.out().facts().fluent_fact_sets }));
}

TEST(TyrDatalogGroundQueueTest, AchieverPolicyGroundRecordsFiredRule)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition(), a);
    const auto derive_b = fixture.rule(fixture.condition({ fixture.fluent_literal(a) }), b);

    const auto program = fixture.program();
    const auto const_workspace = datalog::ConstProgramWorkspace<GroundTag>(program);
    using Workspace = datalog::ProgramWorkspace<GroundTag,
                                                datalog::OrAnnotationPolicy<GroundTag>,
                                                datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>,
                                                datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>>;
    auto workspace = Workspace(const_workspace,
                               datalog::OrAnnotationPolicy<GroundTag>(),
                               datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>(),
                               datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>());
    auto queue_workspace = datalog::QueueWorkspace<GroundTag>(program);
    auto ctx = datalog::ProgramExecutionContext(workspace, queue_workspace);

    ctx.initialize(initial_fluent_atoms(fixture));
    dq::solve_ground_queue(ctx);

    EXPECT_EQ(atom_indices(ctx.out().fluent_atoms()), expected_indices({ a, b }));
    const auto* achievers = ctx.out().and_ap().find_achievers(b);
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ((*achievers)[0].get_rule_key().get_index(), derive_b.get_index());
}

TEST(TyrDatalogGroundQueueTest, UnfilteredNegativeFluentLiteralDoesNotFire)
{
    auto fixture = GroundQueueFixture();
    const auto a = fixture.fluent_atom("a");
    const auto b = fixture.fluent_atom("b");
    fixture.rule(fixture.condition({ fixture.fluent_literal(a, false) }), b);

    const auto result = solve_default_state(fixture);

    EXPECT_TRUE(result.atom_indices.empty());
}

}
