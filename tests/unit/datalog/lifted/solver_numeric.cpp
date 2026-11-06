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

#include "solver_fixtures.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include <gtest/gtest.h>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{
namespace
{

TEST(TyrDatalogLiftedBottomUpTest, NoAnnotationPolicyComputesZeroCostNumericReachability)
{
    auto fixture = make_lifted_numeric_program();
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);

    tyr::datalog::compute_model(context);

    const auto expected = ygg::ClosedInterval<ygg::float_t>(3, 3);
    EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.target], expected);
    EXPECT_EQ(workspace.annotations.find(fixture.goal.index()), nullptr);
    EXPECT_EQ(workspace.numeric_annotations.size(), 0);
}

TEST(TyrDatalogLiftedBottomUpTest, SingleCoreTerminationExposesOnlyOptimalGoalFrontierAchievers)
{
    auto fixture = make_lifted_predicate_program();
    auto termination_policy = tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>();
    termination_policy.set_goals(fixture.goals);
    auto cost_policy = tyr::datalog::RuleCostOverridePolicy<LiftedTag>();
    cost_policy.set_cost(fixture.first_binding.index(), static_cast<tyr::datalog::Cost>(1));
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag,
                                                     tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>,
                                                     tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>,
                                                     tyr::datalog::RuleCostOverridePolicy<LiftedTag>>;
    auto workspace =
        Workspace(fixture.program, tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>(), termination_policy, cost_policy);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    auto execution_context = ygg::ExecutionContext::create(1);

    tyr::datalog::execute_model(context, *execution_context);

    ASSERT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    const auto* achievers = workspace.annotation_policy.find_achievers(fixture.goal.index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_rule_key(), fixture.first_binding.index());
}

TEST(TyrDatalogLiftedBottomUpTest, SingleCoreRetainsFirstEqualCostWitness)
{
    auto fixture = make_lifted_predicate_program();
    using Workspace =
        tyr::datalog::ProgramWorkspace<LiftedTag, tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::MaxAggregation>, tyr::datalog::NoTerminationPolicy>;
    auto workspace = Workspace(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    auto execution_context = ygg::ExecutionContext::create(1);

    tyr::datalog::execute_model(context, *execution_context);

    const auto* annotation = workspace.annotations.find(fixture.goal.index());
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::PredicateTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_rule_key(), fixture.first_binding.index());
}

TEST(TyrDatalogLiftedBottomUpTest, MissingLiteralBindingUsesClosedWorldPolarityWithoutInterning)
{
    auto fixture = make_lifted_predicate_program();
    auto object_data = urd::data<tyr::formalism::Object>(std::string("missing"));
    tyr::formalism::canonicalize(object_data);
    const auto object = fixture.repository->insert(object_data).first;

    const auto rule = fixture.program.get_program().get_rules<tyr::formalism::PredicateTag>().front();
    const auto positive = rule.get_body().get_literals<tyr::formalism::FluentTag>().front();
    auto negative_data = urd::data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(positive.get_atom().index(), false);
    tyr::formalism::datalog::canonicalize(negative_data);
    const auto negative = fixture.repository->insert(negative_data).first;

    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(fixture.program);
    workspace.binding.push_back(object.index());
    auto grounder = tyr::formalism::datalog::GrounderContext { workspace.datalog_builder, *workspace.workspace_repository, workspace.binding };
    const auto fact_sets = tyr::datalog::FactSets { workspace.const_workspace->facts.fact_sets, workspace.facts.fact_sets };
    const auto applicability_context = tyr::datalog::ApplicabilityContext { fact_sets, grounder };
    const auto relation = positive.get_atom().get_predicate().index();
    const auto num_bindings = workspace.workspace_repository->size(relation);

    EXPECT_FALSE(tyr::datalog::is_applicable(positive, applicability_context));
    EXPECT_TRUE(tyr::datalog::is_applicable(negative, applicability_context));
    EXPECT_EQ(workspace.workspace_repository->size(relation), num_bindings);
}

TEST(TyrDatalogLiftedBottomUpTest, SumAnnotationPricesEachNumericSupportOccurrence)
{
    auto fixture = make_lifted_numeric_program();
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto workspace = Workspace(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);

    tyr::datalog::compute_model(context);

    const auto interval = ygg::ClosedInterval<ygg::float_t>(3, 3);
    const auto* annotation = workspace.numeric_annotations.find(fixture.target.index(), interval);
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::FunctionTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_cost(), 7);
    ASSERT_EQ(witness->get_numeric_supports().size(), 1);
    EXPECT_EQ(witness->get_numeric_supports().front().get_key(), fixture.source.index());
    EXPECT_EQ(witness->get_numeric_supports().front().get_interval(), interval);
    EXPECT_EQ(witness->get_numeric_supports().front().get_cost(), 2);
}

TEST(TyrDatalogLiftedBottomUpTest, StoredNumericCertificateCanStillBecomeAvailable)
{
    auto fixture = make_lifted_numeric_program();
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto workspace = Workspace(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    const auto source_interval = ygg::ClosedInterval<ygg::float_t>(3, 3);
    workspace.numeric_annotations.insert(fixture.source.index(), source_interval, tyr::datalog::BaseAnnotation(2));

    tyr::datalog::compute_model(context);

    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.source], source_interval);
    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.target], source_interval);
}

TEST(TyrDatalogLiftedBottomUpTest, RepricesTentativePredicateAfterAvailableNumericCertificateImproves)
{
    auto fixture = make_lifted_numeric_program(true, { 3 }, true);
    ASSERT_EQ(fixture.program.get_program().get_rules<tyr::formalism::PredicateTag>().front().get_arity(), 1);
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using TerminationPolicy = tyr::datalog::TerminationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, TerminationPolicy>;
    auto termination_policy = TerminationPolicy();
    termination_policy.set_goals(fixture.goals);
    auto workspace = Workspace(fixture.program, AnnotationPolicy(), termination_policy);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    const auto source_interval = ygg::ClosedInterval<ygg::float_t>(3, 3);
    workspace.numeric_annotations.clear();
    workspace.numeric_annotations.insert(fixture.source.index(), source_interval, tyr::datalog::BaseAnnotation(5));

    tyr::datalog::compute_model(context);

    const auto* source = workspace.numeric_annotations.find(fixture.source.index(), source_interval);
    const auto* goal = workspace.annotations.find(fixture.goal.index());
    ASSERT_NE(source, nullptr);
    ASSERT_NE(goal, nullptr);
    EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(tyr::datalog::get_cost(*source), 2);
    EXPECT_EQ(tyr::datalog::get_cost(*goal), 5);
}

TEST(TyrDatalogLiftedBottomUpTest, GroundAndLiftedShareResolvedNumericCandidateSemantics)
{
    auto fixture = make_lifted_numeric_program(false, { 1, 10 });
    const auto source_interval = ygg::ClosedInterval<ygg::float_t>(1, 10);
    const auto low_support_interval = ygg::ClosedInterval<ygg::float_t>(1, 2);
    const auto interior_support_interval = ygg::ClosedInterval<ygg::float_t>(4, 6);
    const auto high_support_interval = ygg::ClosedInterval<ygg::float_t>(8, 10);
    const auto expect_source_annotation_order = [&](const auto* entries)
    {
        ASSERT_NE(entries, nullptr);
        ASSERT_EQ(entries->size(), 3);
        EXPECT_EQ(entries->at(0).interval, interior_support_interval);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(0).annotation), 1);
        EXPECT_EQ(entries->at(1).interval, low_support_interval);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(1).annotation), 2);
        EXPECT_EQ(entries->at(2).interval, high_support_interval);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(2).annotation), 2);
    };

    using LiftedAnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using LiftedWorkspace = tyr::datalog::ProgramWorkspace<LiftedTag, LiftedAnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto lifted_workspace = LiftedWorkspace(fixture.program);
    auto lifted_context = tyr::datalog::ProgramExecutionContext(lifted_workspace);
    lifted_workspace.numeric_annotations.clear();
    lifted_workspace.numeric_annotations.insert(fixture.source.index(),
                                                high_support_interval,
                                                tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(2)));
    lifted_workspace.numeric_annotations.insert(fixture.source.index(),
                                                interior_support_interval,
                                                tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(1)));
    lifted_workspace.numeric_annotations.insert(fixture.source.index(),
                                                low_support_interval,
                                                tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(2)));
    expect_source_annotation_order(lifted_workspace.numeric_annotations.find_entries(fixture.source.index().relation, fixture.source.index().row));

    tyr::datalog::compute_model(lifted_context);

    EXPECT_EQ((lifted_workspace.workspace_repository->template size<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>()), 0);
    EXPECT_EQ((lifted_workspace.workspace_repository->template size<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::FunctionTag>>()), 0);

    auto builder = tyr::formalism::datalog::Builder {};
    auto binding = urd::flat_vector<urd::index<tyr::formalism::Object>> {};
    auto grounder = tyr::formalism::datalog::GrounderContext { builder, *fixture.repository, binding };
    auto ground_program_data = urd::data<tyr::formalism::datalog::Program<GroundTag>> {};
    const auto lifted_program = fixture.program.get_program();
    for (const auto predicate : lifted_program.get_predicates<tyr::formalism::FluentTag>())
        ground_program_data.fluent_predicates.push_back(predicate.index());
    for (const auto function : lifted_program.get_functions<tyr::formalism::FluentTag>())
        ground_program_data.fluent_functions.push_back(function.index());
    for (const auto value : lifted_program.get_fterm_values<tyr::formalism::FluentTag>())
        ground_program_data.fluent_fterm_values.push_back(value.index());
    for (const auto rule : lifted_program.get_rules<tyr::formalism::PredicateTag>())
        ground_program_data.predicate_rules.push_back(tyr::formalism::datalog::ground(rule, grounder).first.index());
    for (const auto rule : lifted_program.get_rules<tyr::formalism::FunctionTag>())
        ground_program_data.function_rules.push_back(tyr::formalism::datalog::ground(rule, grounder).first.index());
    tyr::formalism::datalog::canonicalize(ground_program_data);
    const auto ground_program_view = fixture.repository->insert(ground_program_data).first;
    auto ground_program = tyr::datalog::Program<GroundTag>(ground_program_view, fixture.repository, fixture.factory);

    using GroundAnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using GroundWorkspace = tyr::datalog::ProgramWorkspace<GroundTag, GroundAnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto ground_workspace = GroundWorkspace(ground_program);
    auto ground_context = tyr::datalog::ProgramExecutionContext(ground_workspace);

    auto ground_source_data = urd::data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(fixture.source.index());
    auto ground_target_data = urd::data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(fixture.target.index());
    const auto ground_source = fixture.repository->insert(ground_source_data).first;
    const auto ground_target = fixture.repository->insert(ground_target_data).first;
    ground_workspace.numeric_annotations.clear();
    ground_workspace.numeric_annotations.insert(ground_source.get_row().index(),
                                                high_support_interval,
                                                tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(2)));
    ground_workspace.numeric_annotations.insert(ground_source.get_row().index(),
                                                interior_support_interval,
                                                tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(1)));
    ground_workspace.numeric_annotations.insert(ground_source.get_row().index(),
                                                low_support_interval,
                                                tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(2)));
    expect_source_annotation_order(ground_workspace.numeric_annotations.find_entries(ground_source.get_row().index()));

    tyr::datalog::compute_model(ground_context);

    EXPECT_TRUE(lifted_workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_TRUE(ground_workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(lifted_workspace.facts.fact_sets.function[fixture.source], source_interval);
    EXPECT_EQ(ground_workspace.facts.fact_sets.function[ground_source], source_interval);
    EXPECT_EQ(lifted_workspace.facts.fact_sets.function[fixture.target], source_interval);
    EXPECT_EQ(ground_workspace.facts.fact_sets.function[ground_target], source_interval);

    const auto* lifted_annotation = lifted_workspace.numeric_annotations.find(fixture.target.index(), source_interval);
    const auto* ground_annotation = ground_workspace.numeric_annotations.find(ground_target.get_row().index(), source_interval);
    ASSERT_NE(lifted_annotation, nullptr);
    ASSERT_NE(ground_annotation, nullptr);
    const auto* lifted_witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::FunctionTag>>(lifted_annotation);
    const auto* ground_witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::FunctionTag>>(ground_annotation);
    ASSERT_NE(lifted_witness, nullptr);
    ASSERT_NE(ground_witness, nullptr);
    EXPECT_EQ(lifted_witness->get_cost(), ground_witness->get_cost());
    ASSERT_FALSE(ground_witness->get_numeric_supports().empty());
    ASSERT_EQ(lifted_witness->get_numeric_supports().size(), ground_witness->get_numeric_supports().size());
    for (size_t i = 0; i < ground_witness->get_numeric_supports().size(); ++i)
    {
        const auto& ground_support = ground_witness->get_numeric_supports()[i];
        const auto& lifted_support = lifted_witness->get_numeric_supports()[i];
        EXPECT_EQ(ground_support.get_key(), lifted_support.get_key());
        EXPECT_EQ(ground_support.get_key(), ground_source.get_row().index());
        EXPECT_EQ(ground_support.get_interval(), lifted_support.get_interval());
        EXPECT_EQ(ground_support.get_cost(), lifted_support.get_cost());
    }
    // The assignment target = source covers [1, 10] with the low and high certificates.
    const auto has_support = [&](ygg::ClosedInterval<ygg::float_t> interval)
    {
        return std::ranges::any_of(lifted_witness->get_numeric_supports(),
                                   [&](const auto& support) { return support.get_interval() == interval && support.get_cost() == 2; });
    };
    EXPECT_TRUE(has_support(low_support_interval));
    EXPECT_TRUE(has_support(high_support_interval));
}

TEST(TyrDatalogLiftedBottomUpTest, RepeatedArgumentsNewAtomRecordsExistingHeadAchiever)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(urd::data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto diagonal = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("diagonal"), 2));
    const auto seed = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("seed"), 1));
    const auto goal_predicate = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("goal"), 0));
    const auto a = intern(urd::data<tyr::formalism::Object>(std::string("a")));
    const auto b = intern(urd::data<tyr::formalism::Object>(std::string("b")));
    const auto x = intern(urd::data<tyr::formalism::Variable>(std::string("x")));
    const auto y = intern(urd::data<tyr::formalism::Variable>(std::string("y")));

    const auto make_atom = [&](auto predicate, std::initializer_list<tyr::formalism::ParameterIndex> terms)
    {
        auto data = urd::data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
        data.predicate = predicate.index();
        for (const auto term : terms)
            data.terms.emplace_back(term);
        return intern(std::move(data));
    };
    const auto make_literal = [&](auto atom)
    { return intern(urd::data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(atom.index(), true)); };
    const auto make_initial_atom = [&](auto predicate, std::initializer_list<urd::index<tyr::formalism::Object>> objects)
    {
        auto binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        binding_data.relation = predicate.index();
        binding_data.objects.insert(binding_data.objects.end(), objects.begin(), objects.end());
        return intern(urd::data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(intern(std::move(binding_data)).index()));
    };

    const auto derive_seed_atom = make_atom(seed, { tyr::formalism::ParameterIndex(0) });
    const auto derive_diagonal_atom = make_atom(diagonal, { tyr::formalism::ParameterIndex(0), tyr::formalism::ParameterIndex(0) });
    auto derive_body_data = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    derive_body_data.variables.push_back(y.index());
    derive_body_data.fluent_literals.push_back(make_literal(derive_seed_atom).index());
    const auto derive_body = intern(std::move(derive_body_data));
    auto derive_rule_data = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    derive_rule_data.variables.push_back(y.index());
    derive_rule_data.body = derive_body.index();
    derive_rule_data.heads.push_back(derive_diagonal_atom.index());
    const auto derive_rule = intern(std::move(derive_rule_data));

    const auto goal_diagonal_atom = make_atom(diagonal, { tyr::formalism::ParameterIndex(0), tyr::formalism::ParameterIndex(0) });
    const auto goal_pair_atom = make_atom(diagonal, { tyr::formalism::ParameterIndex(0), tyr::formalism::ParameterIndex(1) });
    const auto goal_seed_atom = make_atom(seed, { tyr::formalism::ParameterIndex(1) });
    const auto goal_atom = make_atom(goal_predicate, {});
    auto goal_body_data = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    goal_body_data.variables.push_back(x.index());
    goal_body_data.variables.push_back(y.index());
    goal_body_data.fluent_literals.push_back(make_literal(goal_diagonal_atom).index());
    goal_body_data.fluent_literals.push_back(make_literal(goal_pair_atom).index());
    goal_body_data.fluent_literals.push_back(make_literal(goal_seed_atom).index());
    const auto goal_body = intern(std::move(goal_body_data));
    auto goal_rule_data = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    goal_rule_data.variables.push_back(x.index());
    goal_rule_data.variables.push_back(y.index());
    goal_rule_data.body = goal_body.index();
    goal_rule_data.heads.push_back(goal_atom.index());
    const auto goal_rule = intern(std::move(goal_rule_data));
    const auto conflicting_goal_condition =
        tyr::datalog::create_overapproximation_conflicting_conjunctive_condition(2, goal_rule.get_body(), *repository).first;
    const auto conflicting_literals = conflicting_goal_condition.get_literals<tyr::formalism::FluentTag>();
    ASSERT_EQ(conflicting_literals.size(), 1);
    EXPECT_EQ(conflicting_literals.front().get_atom().index(), goal_diagonal_atom.index());

    const auto initial_aa = make_initial_atom(diagonal, { a.index(), a.index() });
    const auto initial_ab = make_initial_atom(diagonal, { a.index(), b.index() });
    const auto initial_ba = make_initial_atom(diagonal, { b.index(), a.index() });
    const auto initial_seed = make_initial_atom(seed, { b.index() });

    auto goal_binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    goal_binding_data.relation = goal_predicate.index();
    const auto goal = intern(std::move(goal_binding_data));

    auto program_data = urd::data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.objects = { a.index(), b.index() };
    program_data.fluent_predicates = { diagonal.index(), seed.index(), goal_predicate.index() };
    program_data.fluent_atoms = { initial_aa.index(), initial_ab.index(), initial_ba.index(), initial_seed.index() };
    program_data.predicate_rules = { derive_rule.index(), goal_rule.index() };
    const auto program_view = intern(std::move(program_data));
    auto program = tyr::datalog::Program<LiftedTag>(program_view, repository, factory);

    using AnnotationPolicy = tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>;
    using TerminationPolicy = tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, TerminationPolicy>(program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    tyr::datalog::compute_model(context);

    ASSERT_TRUE(workspace.facts.fact_sets.predicate.contains(goal));
    const auto* achievers = workspace.annotation_policy.find_achievers(goal.index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 2);

    auto bindings = urd::Store<urd::sequence<urd::Store<urd::sequence<urd::index<tyr::formalism::Object>>>>>();
    for (const auto& achiever : *achievers)
    {
        EXPECT_EQ(achiever.get_rule_key().relation, goal_rule.index());
        auto& binding = bindings.emplace_back();
        for (const auto object : urd::resolve(achiever.get_rule_key(), *workspace.workspace_repository).get_objects())
            binding.push_back(object.index());
    }
    std::ranges::sort(bindings);
    const auto expected = urd::Store<urd::sequence<urd::Store<urd::sequence<urd::index<tyr::formalism::Object>>>>> { { a.index(), b.index() }, { b.index(), b.index() } };
    EXPECT_EQ(bindings, expected);
}

TEST(TyrDatalogLiftedBottomUpTest, NewHigherArityAtomsGenerateBindingsWithoutDeltaEdges)
{
    // h(x,y,z) :- t(x,y,z), u(x,y,z). The pairwise projections of t and u hold initially through other atoms, so the
    // consistency graph of the h rule has the clique (a,b,c) before t(a,b,c) and u(a,b,c) are derived in the same
    // iteration. Only the new atoms can generate h(a,b,c), and only the one of the first literal may (Debug checks duplicates).
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(urd::data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    using FluentPredicate = tyr::formalism::Predicate<tyr::formalism::FluentTag>;
    const auto t = intern(urd::data<FluentPredicate>(std::string("t"), 3));
    const auto u = intern(urd::data<FluentPredicate>(std::string("u"), 3));
    const auto h = intern(urd::data<FluentPredicate>(std::string("h"), 3));
    const auto s = intern(urd::data<FluentPredicate>(std::string("s"), 1));
    const auto m = intern(urd::data<FluentPredicate>(std::string("m"), 2));
    const auto a = intern(urd::data<tyr::formalism::Object>(std::string("a")));
    const auto b = intern(urd::data<tyr::formalism::Object>(std::string("b")));
    const auto c = intern(urd::data<tyr::formalism::Object>(std::string("c")));
    const auto d = intern(urd::data<tyr::formalism::Object>(std::string("d")));
    const auto x = intern(urd::data<tyr::formalism::Variable>(std::string("x")));
    const auto y = intern(urd::data<tyr::formalism::Variable>(std::string("y")));
    const auto z = intern(urd::data<tyr::formalism::Variable>(std::string("z")));
    const auto p0 = tyr::formalism::ParameterIndex(0);
    const auto p1 = tyr::formalism::ParameterIndex(1);
    const auto p2 = tyr::formalism::ParameterIndex(2);

    const auto make_atom = [&](auto predicate, std::initializer_list<tyr::formalism::ParameterIndex> terms)
    {
        auto data = urd::data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
        data.predicate = predicate.index();
        for (const auto term : terms)
            data.terms.emplace_back(term);
        return intern(std::move(data));
    };
    const auto make_literal = [&](auto atom)
    { return intern(urd::data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(atom.index(), true)); };
    const auto make_binding = [&](auto predicate, std::initializer_list<urd::index<tyr::formalism::Object>> objects)
    {
        auto data = urd::data<tyr::formalism::RelationBinding<FluentPredicate>>();
        data.relation = predicate.index();
        data.objects.insert(data.objects.end(), objects.begin(), objects.end());
        return intern(std::move(data));
    };
    const auto make_initial_atom = [&](auto predicate, std::initializer_list<urd::index<tyr::formalism::Object>> objects)
    { return intern(urd::data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(make_binding(predicate, objects).index())); };
    const auto make_predicate_rule = [&](auto head, urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>>> literals)
    {
        auto body = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
        body.variables = { x.index(), y.index(), z.index() };
        body.fluent_literals.insert(body.fluent_literals.end(), literals.begin(), literals.end());
        auto rule = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
        rule.variables = { x.index(), y.index(), z.index() };
        rule.body = intern(std::move(body)).index();
        rule.heads.push_back(head.index());
        return intern(std::move(rule));
    };

    const auto seeded = urd::Store<urd::sequence<urd::index<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>>> { make_literal(make_atom(s, { p0 })).index(), make_literal(make_atom(m, { p1, p2 })).index() };
    const auto derive_t = make_predicate_rule(make_atom(t, { p0, p1, p2 }), seeded);
    const auto derive_u = make_predicate_rule(make_atom(u, { p0, p1, p2 }), seeded);
    const auto derive_h =
        make_predicate_rule(make_atom(h, { p0, p1, p2 }),
                            { make_literal(make_atom(t, { p0, p1, p2 })).index(), make_literal(make_atom(u, { p0, p1, p2 })).index() });

    auto program_data = urd::data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.objects = { a.index(), b.index(), c.index(), d.index() };
    program_data.fluent_predicates = { t.index(), u.index(), h.index(), s.index(), m.index() };
    for (const auto predicate : { t, u })
    {
        // Projections (a,b), (b,c) and (a,c) of the triple (a,b,c), each through another atom.
        program_data.fluent_atoms.push_back(make_initial_atom(predicate, { a.index(), b.index(), d.index() }).index());
        program_data.fluent_atoms.push_back(make_initial_atom(predicate, { d.index(), b.index(), c.index() }).index());
        program_data.fluent_atoms.push_back(make_initial_atom(predicate, { a.index(), d.index(), c.index() }).index());
    }
    program_data.fluent_atoms.push_back(make_initial_atom(s, { a.index() }).index());
    program_data.fluent_atoms.push_back(make_initial_atom(m, { b.index(), c.index() }).index());
    program_data.predicate_rules = { derive_t.index(), derive_u.index(), derive_h.index() };
    auto program = tyr::datalog::Program<LiftedTag>(intern(std::move(program_data)), repository, factory);

    using AnnotationPolicy = tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>;
    using TerminationPolicy = tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, TerminationPolicy>(program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    tyr::datalog::compute_model(context);

    const auto target = make_binding(h, { a.index(), b.index(), c.index() });
    ASSERT_TRUE(workspace.facts.fact_sets.predicate.contains(target.index()));
    const auto* achievers = workspace.annotation_policy.find_achievers(target.index());
    ASSERT_NE(achievers, nullptr);
    EXPECT_EQ(achievers->size(), 1);
    for (const auto other : { make_binding(h, { a.index(), b.index(), d.index() }),
                              make_binding(h, { d.index(), b.index(), c.index() }),
                              make_binding(h, { a.index(), d.index(), c.index() }) })
        EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(other.index()));

    // The h rule rejected (a,b,c) before t(a,b,c) existed, so the new atom, not a delta edge, generated it.
    const auto& h_rule = workspace.predicate_rules[static_cast<ygg::uint_t>(derive_h.index().value)];
    ASSERT_NE(h_rule, nullptr);
    EXPECT_GT(h_rule->worker.front().solve.statistics.num_rejected_rules, 0);
}

TEST(TyrDatalogLiftedBottomUpTest, RejectedCanonicalTiesDoNotInternUnneededBindings)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(urd::data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto source = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("source"), 1));
    const auto goal = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("goal"), 0));
    const auto variable = intern(urd::data<tyr::formalism::Variable>(std::string("x")));

    auto source_atom_data = urd::data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    source_atom_data.predicate = source.index();
    source_atom_data.terms.emplace_back(tyr::formalism::ParameterIndex(0));
    const auto source_atom = intern(std::move(source_atom_data));

    const auto source_literal = intern(urd::data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(source_atom.index(), true));
    auto body_data = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    body_data.variables.push_back(variable.index());
    body_data.fluent_literals.push_back(source_literal.index());
    const auto body = intern(std::move(body_data));

    auto goal_atom_data = urd::data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    goal_atom_data.predicate = goal.index();
    const auto goal_atom = intern(std::move(goal_atom_data));

    auto rule_data = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    rule_data.variables.push_back(variable.index());
    rule_data.body = body.index();
    rule_data.heads.push_back(goal_atom.index());
    const auto rule = intern(std::move(rule_data));

    auto program_data = urd::data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.fluent_predicates.push_back(source.index());
    program_data.fluent_predicates.push_back(goal.index());
    program_data.predicate_rules.push_back(rule.index());
    for (const auto* name : { "a", "b", "c", "d" })
    {
        const auto object = intern(urd::data<tyr::formalism::Object>(std::string(name)));
        program_data.objects.push_back(object.index());

        auto binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        binding_data.relation = source.index();
        binding_data.objects.push_back(object.index());
        const auto binding = intern(std::move(binding_data));
        program_data.fluent_atoms.push_back(
            intern(urd::data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.index())).index());
    }
    const auto program_view = intern(std::move(program_data));

    auto program = tyr::datalog::Program<LiftedTag>(program_view, repository, factory);
    const auto& const_rule_workspace = program.get_const_program_workspace().get_rules<tyr::formalism::PredicateTag>().front().value();
    EXPECT_EQ(&const_rule_workspace.get_nullary_condition().context(), &program.get_program_repository());

    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Termination = tyr::datalog::NoTerminationPolicy;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, Termination>(program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    tyr::datalog::compute_model(context);

    EXPECT_EQ(workspace.workspace_repository->size(rule.index()), 1);
    EXPECT_EQ(workspace.workspace_repository->size(goal.index()), 1);
}

TEST(TyrDatalogLiftedBottomUpTest, ProgramWorkspacesOwnIndependentRepositories)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(urd::data<T> data)
    {
        tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto predicate = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("workspace-predicate"), 1));
    const auto object = intern(urd::data<tyr::formalism::Object>(std::string("workspace-object")));
    auto program_data = urd::data<tyr::formalism::datalog::Program<LiftedTag>> {};
    program_data.fluent_predicates.push_back(predicate.index());
    program_data.objects.push_back(object.index());
    tyr::formalism::datalog::canonicalize(program_data);
    const auto program_view = repository->insert(program_data).first;
    const auto program = tyr::datalog::Program<LiftedTag>(program_view, repository, factory);

    auto first = tyr::datalog::ProgramWorkspace<LiftedTag>(program);
    auto second = tyr::datalog::ProgramWorkspace<LiftedTag>(program);

    EXPECT_NE(first.workspace_repository.get(), second.workspace_repository.get());
    EXPECT_EQ(&first.workspace_repository->get_root(), &program.get_program_repository());
    EXPECT_EQ(&second.workspace_repository->get_root(), &program.get_program_repository());

    auto variable = urd::data<tyr::formalism::Variable>(std::string("workspace-only"));
    EXPECT_TRUE(first.workspace_repository->insert(variable).second);
    EXPECT_FALSE(second.workspace_repository->find(variable).has_value());

    auto second_variable = urd::data<tyr::formalism::Variable>(std::string("second-workspace-only"));
    EXPECT_TRUE(second.workspace_repository->insert(second_variable).second);

    auto binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    binding_data.relation = predicate.index();
    binding_data.objects.push_back(object.index());
    const auto binding = first.workspace_repository->insert(binding_data).first;
    const auto binding_index = binding.index();
    first.annotations.insert_or_assign(binding.index(), tyr::datalog::BaseAnnotation(3));

    first.reset_evaluation();

    EXPECT_FALSE(first.workspace_repository->find(urd::data<tyr::formalism::Variable>(std::string("workspace-only"))).has_value());
    EXPECT_TRUE(second.workspace_repository->find(urd::data<tyr::formalism::Variable>(std::string("second-workspace-only"))).has_value());
    EXPECT_EQ(&first.workspace_repository->get_root(), &program.get_program_repository());

    auto reused_binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    reused_binding_data.relation = predicate.index();
    reused_binding_data.objects.push_back(object.index());
    const auto reused_binding = first.workspace_repository->insert(reused_binding_data).first;
    EXPECT_EQ(reused_binding.index(), binding_index);
    EXPECT_EQ(first.annotations.find(reused_binding.index()), nullptr);
}

TEST(TyrDatalogLiftedBottomUpTest, PredicateAnnotationsUseRelationAndRowIndices)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(urd::data<T> data)
    {
        tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto first_predicate = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("first"), 1));
    const auto second_predicate = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("second"), 1));
    const auto first_object = intern(urd::data<tyr::formalism::Object>(std::string("a")));
    const auto second_object = intern(urd::data<tyr::formalism::Object>(std::string("b")));
    const auto make_binding = [&](auto predicate, auto object)
    {
        auto data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        data.relation = predicate.index();
        data.objects.push_back(object.index());
        return intern(std::move(data));
    };

    const auto first = make_binding(first_predicate, first_object);
    const auto hole = make_binding(first_predicate, second_object);
    const auto second = make_binding(second_predicate, first_object);
    auto annotations = tyr::datalog::PredicateAnnotations<>();

    annotations.insert_or_assign(first.index(), tyr::datalog::BaseAnnotation(3));
    annotations.insert_or_assign(second.index(), tyr::datalog::BaseAnnotation(5));
    EXPECT_EQ(tyr::datalog::get_cost(*annotations.find(first.index())), 3);
    EXPECT_EQ(tyr::datalog::get_cost(*std::as_const(annotations).find(second.index())), 5);
    EXPECT_EQ(annotations.find(hole.index()), nullptr);

    annotations.insert_or_assign(first.index(), tyr::datalog::BaseAnnotation(2));
    EXPECT_EQ(tyr::datalog::get_cost(*annotations.find(first.index())), 2);

    annotations.clear();
    EXPECT_EQ(annotations.find(first.index()), nullptr);
    EXPECT_EQ(annotations.find(second.index()), nullptr);
}

TEST(TyrDatalogLiftedBottomUpTest, DeltaAnnotationsAreConcurrentAndReusable)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(urd::data<T> data)
    {
        tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto first_predicate = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("first"), 1));
    const auto second_predicate = intern(urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("second"), 1));
    const auto function = intern(urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("value"), 1));

    auto first_bindings = urd::Store<urd::sequence<tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag>>> {};
    auto second_bindings = urd::Store<urd::sequence<tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag>>> {};
    auto function_bindings = urd::Store<urd::sequence<tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag>>> {};
    for (size_t i = 0; i < 64; ++i)
    {
        const auto object = intern(urd::data<tyr::formalism::Object>("o" + std::to_string(i)));
        const auto make_predicate_binding = [&](auto predicate)
        {
            auto data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
            data.relation = predicate.index();
            data.objects.push_back(object.index());
            return intern(std::move(data));
        };
        const auto make_function_binding = [&](auto function_)
        {
            auto data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>> {};
            data.relation = function_.index();
            data.objects.push_back(object.index());
            return intern(std::move(data));
        };
        first_bindings.push_back(make_predicate_binding(first_predicate));
        second_bindings.push_back(make_predicate_binding(second_predicate));
        function_bindings.push_back(make_function_binding(function));
    }

    using SequentialMap = tyr::datalog::DenseRelationMap<tyr::formalism::PredicateTag, tyr::datalog::Cost>;
    using ConcurrentMap = tyr::datalog::DenseRelationMap<tyr::formalism::PredicateTag, tyr::datalog::Cost, true>;
    static_assert(!SequentialMap::thread_safe);
    static_assert(ConcurrentMap::thread_safe);
    static_assert(!tyr::datalog::PredicateAnnotations<>::thread_safe);
    static_assert(tyr::datalog::PredicateAnnotations<true>::thread_safe);
    static_assert(!tyr::datalog::FunctionAnnotations<>::thread_safe);
    static_assert(tyr::datalog::FunctionAnnotations<true>::thread_safe);
    static_assert(std::is_default_constructible_v<tyr::datalog::PredicateAnnotations<>>);
    static_assert(!std::is_default_constructible_v<tyr::datalog::PredicateAnnotations<true>>);
    static_assert(std::is_default_constructible_v<tyr::datalog::FunctionAnnotations<>>);
    static_assert(!std::is_default_constructible_v<tyr::datalog::FunctionAnnotations<true>>);
    static_assert(std::is_copy_constructible_v<SequentialMap>);
    static_assert(!std::is_copy_constructible_v<ConcurrentMap>);
    static_assert(std::is_nothrow_move_constructible_v<ConcurrentMap>);
    static_assert(std::is_nothrow_move_assignable_v<ConcurrentMap>);
    const auto retain_min = [](auto& map, auto binding, tyr::datalog::Cost candidate)
    {
        map.update(binding.index(),
                   [&](auto& incumbent, bool initialized)
                   {
                       if (!initialized || candidate < incumbent)
                           incumbent = candidate;
                   });
    };
    auto sequential_map = SequentialMap(2);
    auto concurrent_map = ConcurrentMap(2);
    for (size_t worker = 0; worker < 8; ++worker)
        retain_min(sequential_map, first_bindings.front(), static_cast<tyr::datalog::Cost>(8 - worker));

    auto delta_annotations = tyr::datalog::PredicateAnnotations<true>(2);
    auto delta_numeric_annotations = tyr::datalog::FunctionAnnotations<true>(1);
    const auto interval = ygg::ClosedInterval<ygg::float_t>(0, 1);
    auto arena = oneapi::tbb::task_arena(8);
    arena.execute(
        [&]
        {
            oneapi::tbb::parallel_for(
                static_cast<size_t>(0),
                static_cast<size_t>(8),
                [&](size_t worker)
                {
                    const auto predicate_cost = static_cast<tyr::datalog::Cost>(8 - worker);
                    const auto function_cost = static_cast<tyr::datalog::Cost>(worker < 4 ? 3 : 5);
                    retain_min(concurrent_map, first_bindings.front(), predicate_cost);
                    for (size_t i = first_bindings.size(); i-- > 0;)
                    {
                        delta_annotations.insert_if_better(first_bindings[i].index(), tyr::datalog::BaseAnnotation(predicate_cost));
                        delta_annotations.insert_if_better(second_bindings[i].index(), tyr::datalog::BaseAnnotation(predicate_cost));
                        delta_numeric_annotations.insert(function_bindings[i].index(), interval, tyr::datalog::BaseAnnotation(function_cost));
                    }
                });
        });

    ASSERT_NE(sequential_map.find(first_bindings.front().index()), nullptr);
    ASSERT_NE(concurrent_map.find(first_bindings.front().index()), nullptr);
    EXPECT_EQ(*sequential_map.find(first_bindings.front().index()), *concurrent_map.find(first_bindings.front().index()));

    for (size_t i = 0; i < first_bindings.size(); ++i)
    {
        ASSERT_NE(delta_annotations.find(first_bindings[i].index()), nullptr);
        ASSERT_NE(delta_annotations.find(second_bindings[i].index()), nullptr);
        EXPECT_EQ(tyr::datalog::get_cost(*delta_annotations.find(first_bindings[i].index())), 1);
        EXPECT_EQ(tyr::datalog::get_cost(*delta_annotations.find(second_bindings[i].index())), 1);

        const auto* entries = delta_numeric_annotations.find_entries(function_bindings[i].index());
        ASSERT_NE(entries, nullptr);
        ASSERT_EQ(entries->size(), 1);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(0).annotation), 3);
    }

    const auto second_interval = ygg::ClosedInterval<ygg::float_t>(1, 2);
    EXPECT_TRUE(delta_numeric_annotations.insert(function_bindings.back().index(), second_interval, tyr::datalog::BaseAnnotation(4)));
    EXPECT_FALSE(delta_numeric_annotations.insert(function_bindings.back().index(), second_interval, tyr::datalog::BaseAnnotation(5)));
    EXPECT_TRUE(delta_numeric_annotations.insert(function_bindings.back().index(), second_interval, tyr::datalog::BaseAnnotation(2)));
    ASSERT_EQ(delta_numeric_annotations.find_entries(function_bindings.back().index())->size(), 2);
    ASSERT_NE(delta_numeric_annotations.find(function_bindings.back().index(), second_interval), nullptr);
    EXPECT_EQ(tyr::datalog::get_cost(*delta_numeric_annotations.find(function_bindings.back().index(), second_interval)), 2);

    auto numeric_annotations = tyr::datalog::FunctionAnnotations<>();
    numeric_annotations.insert(function_bindings.back().index(), interval, tyr::datalog::BaseAnnotation(5));
    numeric_annotations.insert(function_bindings.back().index(), interval, tyr::datalog::BaseAnnotation(3));
    numeric_annotations.insert(function_bindings.back().index(), interval, tyr::datalog::BaseAnnotation(4));
    numeric_annotations.insert(function_bindings.back().index(), interval, tyr::datalog::BaseAnnotation(3));
    numeric_annotations.insert(function_bindings.back().index(), second_interval, tyr::datalog::BaseAnnotation(2));
    numeric_annotations.insert(function_bindings.back().index(), interval, tyr::datalog::BaseAnnotation(1));
    const auto* numeric_entries = numeric_annotations.find_entries(function.index(), function_bindings.back().index().row);
    ASSERT_NE(numeric_entries, nullptr);
    ASSERT_EQ(numeric_entries->size(), 2);
    EXPECT_EQ(numeric_entries->at(0).interval, interval);
    EXPECT_EQ(tyr::datalog::get_cost(numeric_entries->at(0).annotation), 1);
    EXPECT_EQ(numeric_entries->at(1).interval, second_interval);
    EXPECT_EQ(tyr::datalog::get_cost(numeric_entries->at(1).annotation), 2);
    EXPECT_EQ(tyr::datalog::get_cost(*numeric_annotations.find(function_bindings.back().index(), interval)), 1);

    delta_annotations.clear();
    delta_numeric_annotations.clear();
    sequential_map.clear();
    concurrent_map.clear();
    EXPECT_EQ(delta_annotations.find(first_bindings.back().index()), nullptr);
    EXPECT_EQ(delta_numeric_annotations.find_entries(function_bindings.back().index()), nullptr);
    EXPECT_EQ(sequential_map.find(first_bindings.front().index()), nullptr);
    EXPECT_EQ(concurrent_map.find(first_bindings.front().index()), nullptr);

    delta_annotations.insert_if_better(first_bindings.back().index(), tyr::datalog::BaseAnnotation(7));
    delta_numeric_annotations.insert(function_bindings.back().index(), interval, tyr::datalog::BaseAnnotation(7));
    retain_min(sequential_map, first_bindings.front(), static_cast<tyr::datalog::Cost>(7));
    retain_min(concurrent_map, first_bindings.front(), static_cast<tyr::datalog::Cost>(7));
    EXPECT_EQ(tyr::datalog::get_cost(*delta_annotations.find(first_bindings.back().index())), 7);
    ASSERT_NE(delta_numeric_annotations.find_entries(function_bindings.back().index()), nullptr);
    EXPECT_EQ(delta_numeric_annotations.find_entries(function_bindings.back().index())->size(), 1);
    EXPECT_EQ(*sequential_map.find(first_bindings.front().index()), *concurrent_map.find(first_bindings.front().index()));
}

}  // namespace
}  // namespace tyr::tests
