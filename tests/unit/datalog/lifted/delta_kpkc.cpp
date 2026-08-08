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

#include "tyr/datalog/lifted/delta_kpkc.hpp"

#include "planning/parser.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/solver.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/heuristics/rpg_ff.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/programs/rpg.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace p = tyr::planning;

namespace tyr::tests
{
TEST(TyrDatalogLiftedDeltaKPKC, DeltaEdgesSupportRoundRobinAnchorReplay)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task = make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl");
    auto action_program = p::ApplicableActionProgram<::tyr::LiftedTag>(task.get_task());
    auto& program = action_program.get_datalog_program();
    const auto program_view = program.get_program();

    const d::ConstRuleWorkspace<LiftedTag, f::PredicateTag>* pick_workspace = nullptr;
    const auto& rules = program.get_const_program_workspace().get_rules<f::PredicateTag>();
    for (ygg::uint_t i = 0; i < program_view.get_rules<f::PredicateTag>().size(); ++i)
    {
        const auto is_pick =
            action_program.get_predicate_to_action_mapping().at(program_view.get_rules<f::PredicateTag>()[i].get_head().get_predicate()).get_name() == "pick";

        if (is_pick)
        {
            ASSERT_TRUE(rules[i].has_value());
            pick_workspace = &*rules[i];
            break;
        }
    }
    ASSERT_NE(pick_workspace, nullptr);

    const auto predicates = program_view.get_predicates<f::FluentTag>();
    const auto functions = program_view.get_functions<f::FluentTag>();
    const auto& domains = program.get_domains();
    const auto num_objects = program_view.get_objects().size();
    auto empty_assignments =
        d::TaggedAssignmentSets<f::FluentTag>(predicates, functions, domains.fluent_predicate_domains, domains.fluent_function_domains, num_objects);
    auto initial_assignments =
        d::TaggedAssignmentSets<f::FluentTag>(predicates, functions, domains.fluent_predicate_domains, domains.fluent_function_domains, num_objects);
    for (const auto atom : program_view.get_atoms<f::FluentTag>())
        initial_assignments.predicate.insert(atom);

    const auto& static_assignments = program.get_const_program_workspace().facts.assignment_sets;
    const auto& static_graph = pick_workspace->get_static_consistency_graph();
    const auto& graph_layout = static_graph.get_graph_layout();
    const auto& adjacency_layout = static_graph.get_partitioned_adjacency_layout();
    auto expected_offset = size_t { 0 };
    auto saw_runtime = false;
    auto saw_implicit = false;
    for (ygg::uint_t pi = 0; pi < graph_layout.k; ++pi)
    {
        for (const auto v : graph_layout.vertex_partitions[pi])
        {
            for (ygg::uint_t pj = 0; pj < graph_layout.k; ++pj)
            {
                if (adjacency_layout.is_runtime(pi, pj))
                {
                    saw_runtime = true;
                    ASSERT_LT(expected_offset, d::kpkc::PartitionedAdjacencyLayout::STATIC_ONLY);
                    EXPECT_EQ(adjacency_layout.get_offset(v, pi, pj), static_cast<ygg::uint_t>(expected_offset));
                    expected_offset += graph_layout.info.infos[pj].num_blocks;
                }
                else if (adjacency_layout.is_static_only(pi, pj))
                {
                    EXPECT_EQ(adjacency_layout.get_offset(v, pi, pj), d::kpkc::PartitionedAdjacencyLayout::STATIC_ONLY);
                }
                else
                {
                    saw_implicit = true;
                    EXPECT_EQ(adjacency_layout.get_offset(v, pi, pj), d::kpkc::PartitionedAdjacencyLayout::IMPLICIT);
                }
            }
        }
    }
    EXPECT_TRUE(saw_runtime);
    EXPECT_TRUE(saw_implicit);
    EXPECT_EQ(adjacency_layout.num_blocks(), expected_offset);

    auto kpkc = d::kpkc::DeltaKPKC(static_graph);
    EXPECT_EQ(kpkc.get_delta_graph().matrix.bitset_data().size(), expected_offset);
    EXPECT_EQ(kpkc.get_full_graph().matrix.bitset_data().size(), expected_offset);
    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, empty_assignments));
    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, initial_assignments));
    ASSERT_EQ(kpkc.get_iteration(), 2);

    auto authoritative_edges = std::vector<d::kpkc::Edge> {};
    kpkc.get_delta_graph().matrix.for_each_edge([&](auto&& edge) { authoritative_edges.push_back(edge); });
    ASSERT_FALSE(authoritative_edges.empty());
    const auto& delta_edges = kpkc.materialize_delta_edges();
    ASSERT_EQ(delta_edges, authoritative_edges);

    const auto& layout = kpkc.get_graph_layout();
    EXPECT_TRUE(std::ranges::any_of(delta_edges,
                                    [&](const auto& edge)
                                    {
                                        const auto pi = layout.vertex_to_partition[edge.src.index];
                                        const auto pj = layout.vertex_to_partition[edge.dst.index];
                                        return !static_graph.get_variable_dependeny_graph().binary().has_dependency(pi, pj);
                                    }));

    using Clique = std::vector<ygg::uint_t>;
    const auto collect = [](auto& output)
    {
        return [&output](const auto& clique)
        {
            auto indices = Clique {};
            indices.reserve(clique.size());
            for (const auto vertex : clique)
                indices.push_back(vertex.index);
            output.push_back(std::move(indices));
        };
    };

    auto sequential_cliques = std::vector<Clique> {};
    auto sequential_workspace = d::kpkc::Workspace(layout);
    kpkc.for_each_new_k_clique(collect(sequential_cliques), sequential_workspace);

    auto striped_cliques = std::vector<Clique> {};
    auto even_workspace = d::kpkc::Workspace(layout);
    auto odd_workspace = d::kpkc::Workspace(layout);
    for (size_t stripe = 0; stripe < 2; ++stripe)
    {
        auto& workspace = stripe == 0 ? even_workspace : odd_workspace;
        for (auto edge_index = stripe; edge_index < delta_edges.size(); edge_index += 2)
        {
            if (kpkc.seed_from_anchor(delta_edges[edge_index], workspace))
                kpkc.complete_from_seed<d::kpkc::Edge>(collect(striped_cliques), 0, workspace);
        }
    }

    std::ranges::sort(sequential_cliques);
    std::ranges::sort(striped_cliques);
    ASSERT_FALSE(sequential_cliques.empty());
    EXPECT_EQ(striped_cliques, sequential_cliques);
}

TEST(TyrDatalogLiftedDeltaKPKC, StaticOnlyAdjacencyPreservesFullAndClearsDelta)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task = make_test_parser(root / "classical/tests/visitall/domain.pddl").parse_task(root / "classical/tests/visitall/test-1.pddl");
    auto action_program = p::ApplicableActionProgram<::tyr::LiftedTag>(task.get_task());
    auto& program = action_program.get_datalog_program();
    const auto program_view = program.get_program();

    const d::ConstRuleWorkspace<LiftedTag, f::PredicateTag>* move_workspace = nullptr;
    const auto& rules = program.get_const_program_workspace().get_rules<f::PredicateTag>();
    for (ygg::uint_t i = 0; i < program_view.get_rules<f::PredicateTag>().size(); ++i)
    {
        const auto rule = program_view.get_rules<f::PredicateTag>()[i];
        if (action_program.get_predicate_to_action_mapping().at(rule.get_head().get_predicate()).get_name() != "move")
            continue;

        ASSERT_TRUE(rules[i].has_value());
        move_workspace = &*rules[i];
        break;
    }
    ASSERT_NE(move_workspace, nullptr);

    const auto predicates = program_view.get_predicates<f::FluentTag>();
    const auto functions = program_view.get_functions<f::FluentTag>();
    const auto& domains = program.get_domains();
    auto assignments = d::TaggedAssignmentSets<f::FluentTag>(predicates,
                                                             functions,
                                                             domains.fluent_predicate_domains,
                                                             domains.fluent_function_domains,
                                                             program_view.get_objects().size());
    auto num_at_robot_atoms = size_t { 0 };
    for (const auto atom : program_view.get_atoms<f::FluentTag>())
    {
        if (atom.get_predicate().get_name() != "at-robot")
            continue;
        assignments.predicate.insert(atom);
        ++num_at_robot_atoms;
    }
    ASSERT_EQ(num_at_robot_atoms, 1);

    const auto& static_assignments = program.get_const_program_workspace().facts.assignment_sets;
    const auto& static_graph = move_workspace->get_static_consistency_graph();
    const auto& binary = static_graph.get_variable_dependeny_graph().binary();
    ASSERT_EQ(static_graph.get_graph_layout().k, 2);
    ASSERT_TRUE(binary.has_dependency(0, 1));
    EXPECT_FALSE((binary.has_literal_dependency<f::FluentTag, f::PositiveTag>(0, 1)));
    EXPECT_FALSE((binary.has_literal_dependency<f::FluentTag, f::NegativeTag>(0, 1)));
    EXPECT_FALSE(binary.has_numeric_dependency(0, 1));

    using Binding = std::array<std::string, 2>;
    const auto collect_edges = [&](const auto& graph)
    {
        auto result = std::vector<Binding> {};
        graph.matrix.for_each_edge(
            [&](const d::kpkc::Edge edge)
            {
                auto binding = Binding {};
                const auto insert = [&](const d::kpkc::Vertex vertex)
                {
                    const auto& value = static_graph.get_vertex(vertex.index);
                    binding[ygg::uint_t(value.get_parameter_index())] =
                        ygg::make_view(value.get_object_index(), program.get_program_repository()).get_name().str();
                };
                insert(edge.src);
                insert(edge.dst);
                result.push_back(std::move(binding));
            });
        std::ranges::sort(result);
        return result;
    };
    const auto expected = std::vector<Binding> {
        { "loc-x0-y1", "loc-x0-y0" },
        { "loc-x0-y1", "loc-x0-y2" },
    };

    auto kpkc = d::kpkc::DeltaKPKC(static_graph);
    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, assignments));
    ASSERT_EQ(kpkc.get_iteration(), 1);
    EXPECT_EQ(collect_edges(kpkc.get_full_graph()), expected);
    EXPECT_EQ(collect_edges(kpkc.get_delta_graph()), expected);

    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, assignments));
    ASSERT_EQ(kpkc.get_iteration(), 2);
    EXPECT_EQ(collect_edges(kpkc.get_full_graph()), expected);
    EXPECT_TRUE(collect_edges(kpkc.get_delta_graph()).empty());
}

TEST(TyrDatalogLiftedDeltaKPKC, MixedAdjacencyMatchesAcrossIterations)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task = make_test_parser(root / "classical/tests/transport/domain.pddl").parse_task(root / "classical/tests/transport/test-1.pddl");
    auto action_program = p::ApplicableActionProgram<::tyr::LiftedTag>(task.get_task());
    auto& program = action_program.get_datalog_program();
    const auto program_view = program.get_program();

    const d::ConstRuleWorkspace<LiftedTag, f::PredicateTag>* drive_workspace = nullptr;
    const auto& rules = program.get_const_program_workspace().get_rules<f::PredicateTag>();
    for (ygg::uint_t i = 0; i < program_view.get_rules<f::PredicateTag>().size(); ++i)
    {
        const auto rule = program_view.get_rules<f::PredicateTag>()[i];
        if (action_program.get_predicate_to_action_mapping().at(rule.get_head().get_predicate()).get_name() != "drive")
            continue;

        ASSERT_TRUE(rules[i].has_value());
        drive_workspace = &*rules[i];
        break;
    }
    ASSERT_NE(drive_workspace, nullptr);

    const auto predicates = program_view.get_predicates<f::FluentTag>();
    const auto functions = program_view.get_functions<f::FluentTag>();
    const auto& domains = program.get_domains();
    const auto make_assignments = [&]
    {
        return d::TaggedAssignmentSets<f::FluentTag>(predicates,
                                                     functions,
                                                     domains.fluent_predicate_domains,
                                                     domains.fluent_function_domains,
                                                     program_view.get_objects().size());
    };
    auto first_assignments = make_assignments();
    auto second_assignments = make_assignments();
    auto found_truck_1 = false;
    auto found_truck_2 = false;
    for (const auto atom : program_view.get_atoms<f::FluentTag>())
    {
        if (atom.get_predicate().get_name() != "at")
            continue;

        const auto object = atom.get_objects().front().get_name();
        if (object == "truck-1")
        {
            first_assignments.predicate.insert(atom);
            second_assignments.predicate.insert(atom);
            found_truck_1 = true;
        }
        else if (object == "truck-2")
        {
            second_assignments.predicate.insert(atom);
            found_truck_2 = true;
        }
    }
    ASSERT_TRUE(found_truck_1);
    ASSERT_TRUE(found_truck_2);

    const auto& static_assignments = program.get_const_program_workspace().facts.assignment_sets;
    const auto& static_graph = drive_workspace->get_static_consistency_graph();
    const auto& layout = static_graph.get_graph_layout();
    const auto& adjacency_layout = static_graph.get_partitioned_adjacency_layout();
    ASSERT_EQ(layout.k, 3);
    EXPECT_TRUE(adjacency_layout.is_runtime(0, 1));
    EXPECT_TRUE(adjacency_layout.is_runtime(1, 0));
    EXPECT_TRUE(adjacency_layout.is_static_only(1, 2));
    EXPECT_TRUE(adjacency_layout.is_static_only(2, 1));
    EXPECT_TRUE(adjacency_layout.is_implicit(0, 2));
    EXPECT_TRUE(adjacency_layout.is_implicit(2, 0));

    using Binding = std::array<std::string, 3>;
    auto kpkc = d::kpkc::DeltaKPKC(static_graph);
    const auto collect_cliques = [&](bool delta)
    {
        auto result = std::vector<Binding> {};
        auto workspace = d::kpkc::Workspace(layout);
        const auto collect = [&](const auto& clique)
        {
            auto binding = Binding {};
            for (const auto vertex : clique)
            {
                const auto& value = static_graph.get_vertex(vertex.index);
                binding[ygg::uint_t(value.get_parameter_index())] = ygg::make_view(value.get_object_index(), program.get_program_repository()).get_name().str();
            }
            result.push_back(std::move(binding));
        };
        if (delta)
            kpkc.for_each_new_k_clique(collect, workspace);
        else
            kpkc.for_each_k_clique(collect, workspace);
        std::ranges::sort(result);
        return result;
    };
    const auto count_vertices = [](const auto& graph)
    {
        auto result = size_t { 0 };
        graph.matrix.for_each_vertex([&](auto) { ++result; });
        return result;
    };
    const auto count_edges = [](const auto& graph)
    {
        auto result = size_t { 0 };
        graph.matrix.for_each_edge([&](auto) { ++result; });
        return result;
    };

    const auto first_expected = std::vector<Binding> {
        { "truck-1", "city-loc-3", "city-loc-1" },
        { "truck-1", "city-loc-3", "city-loc-2" },
    };
    auto second_expected = first_expected;
    second_expected.push_back({ "truck-2", "city-loc-1", "city-loc-3" });
    std::ranges::sort(second_expected);

    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, first_assignments));
    EXPECT_EQ(count_vertices(kpkc.get_full_graph()), 5);
    EXPECT_EQ(count_vertices(kpkc.get_delta_graph()), 5);
    EXPECT_EQ(count_edges(kpkc.get_full_graph()), 6);
    EXPECT_EQ(count_edges(kpkc.get_delta_graph()), 6);
    EXPECT_EQ(collect_cliques(false), first_expected);
    EXPECT_EQ(collect_cliques(true), first_expected);

    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, second_assignments));
    EXPECT_EQ(count_vertices(kpkc.get_full_graph()), 7);
    EXPECT_EQ(count_vertices(kpkc.get_delta_graph()), 5);
    EXPECT_EQ(count_edges(kpkc.get_full_graph()), 11);
    EXPECT_EQ(count_edges(kpkc.get_delta_graph()), 5);
    EXPECT_EQ(collect_cliques(false), second_expected);
    EXPECT_EQ(collect_cliques(true), (std::vector<Binding> { { "truck-2", "city-loc-1", "city-loc-3" } }));

    kpkc.set_next_assignment_sets(d::AssignmentSets(static_assignments, second_assignments));
    EXPECT_EQ(count_vertices(kpkc.get_full_graph()), 7);
    EXPECT_EQ(count_vertices(kpkc.get_delta_graph()), 0);
    EXPECT_EQ(count_edges(kpkc.get_full_graph()), 11);
    EXPECT_EQ(count_edges(kpkc.get_delta_graph()), 0);
    EXPECT_EQ(collect_cliques(false), second_expected);
    EXPECT_TRUE(collect_cliques(true).empty());
}

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
TEST(TyrDatalogLiftedDeltaKPKC, InnerParallelismMatchesSequentialRPG)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task = p::Task<::tyr::LiftedTag>::create(make_test_parser(root / "classical/profiling/rovers-large-simple/domain.pddl")
                                                      .parse_task(root / "classical/profiling/rovers-large-simple/p-r1-w1000-o1-1-g2.pddl"));

    auto sequential_context = ygg::ExecutionContext::create(1);
    auto parallel_context = ygg::ExecutionContext::create(ygg::ExecutionContext::get_max_num_threads());
    auto axiom_evaluator = p::AxiomEvaluatorFactory<::tyr::LiftedTag>().create(task, sequential_context);
    auto state_repository = p::StateRepositoryFactory<::tyr::LiftedTag>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<::tyr::LiftedTag>().create(task, sequential_context);
    const auto initial_state = successor_generator->get_initial_node(*state_repository, *axiom_evaluator).get_state();

    auto sequential_heuristic = p::FFRPGHeuristic<::tyr::LiftedTag>::create(task, sequential_context);
    auto parallel_heuristic = p::FFRPGHeuristic<::tyr::LiftedTag>::create(task, parallel_context);
    const auto sequential_value = sequential_heuristic->evaluate(initial_state);
    const auto parallel_value = parallel_heuristic->evaluate(initial_state);
    EXPECT_EQ(parallel_value, sequential_value);
    EXPECT_EQ(parallel_heuristic->get_preferred_actions(), sequential_heuristic->get_preferred_actions());

    using OrPolicy = d::OrAnnotationPolicy<LiftedTag>;
    using AndPolicy = d::AndAnnotationPolicy<LiftedTag, d::SumAggregation>;
    using Termination = d::TerminationPolicy<LiftedTag, d::SumAggregation>;
    using Workspace = d::ProgramWorkspace<LiftedTag, OrPolicy, AndPolicy, Termination>;

    auto program = p::RPGProgram<LiftedTag>(task->get_task());
    auto sequential = Workspace(program.get_datalog_program(), OrPolicy {}, AndPolicy {}, Termination {});
    auto parallel = Workspace(program.get_datalog_program(), OrPolicy {}, AndPolicy {}, Termination {});

    const auto solve = [&](Workspace& workspace, const ygg::ExecutionContextPtr& execution_context)
    {
        workspace.tp.set_goals(program.get_goal());
        p::insert_unextended_state(initial_state.get_state_builder(), *task->get_repository(), program.get_translation_context().p2d, workspace);

        auto context = d::ProgramExecutionContext(workspace);
        execution_context->arena().execute([&] { d::compute_model(context); });
    };
    solve(sequential, sequential_context);
    solve(parallel, parallel_context);

    using NormalizedFact = std::tuple<ygg::uint_t, std::vector<ygg::uint_t>, std::optional<d::Cost>>;
    const auto normalize = [](const Workspace& workspace)
    {
        auto result = std::vector<NormalizedFact> {};
        for (const auto& set : workspace.facts.fact_sets.predicate.get_sets())
        {
            for (const auto binding : set.get_bindings())
            {
                auto objects = std::vector<ygg::uint_t> {};
                for (const auto object : binding.get_objects())
                    objects.push_back(ygg::uint_t(object.get_index()));

                const auto* annotation = workspace.and_annot.find(binding);
                result.emplace_back(ygg::uint_t(binding.get_relation().get_index()),
                                    std::move(objects),
                                    annotation ? std::optional<d::Cost>(d::get_cost(*annotation)) : std::nullopt);
            }
        }
        std::ranges::sort(result);
        return result;
    };

    const auto sequential_facts = normalize(sequential);
    const auto parallel_facts = normalize(parallel);
    ASSERT_FALSE(sequential_facts.empty());
    EXPECT_EQ(parallel_facts, sequential_facts);

    const auto used_inner_parallelism = [](const auto& workspace)
    {
        return std::ranges::any_of(workspace.template get_rules<f::PredicateTag>(),
                                   [](const auto& rule) { return rule && rule->worker.size() == 2 && rule->worker[1].solve.statistics.num_executions > 0; });
    };
    EXPECT_FALSE(used_inner_parallelism(sequential));
    EXPECT_TRUE(used_inner_parallelism(parallel));
}
#endif
}
