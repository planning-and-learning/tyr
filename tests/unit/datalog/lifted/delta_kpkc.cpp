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
#include "tyr/planning/factory.hpp"
#include "tyr/planning/lifted/heuristics/rpg_ff.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
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
    auto action_program = p::ApplicableActionProgram<p::LiftedTag>(task.get_task());
    auto& program = action_program.get_datalog_program();
    const auto program_view = program.get_program();

    const d::ConstRuleWorkspace<LiftedTag>* pick_workspace = nullptr;
    const auto& rules = program.get_const_program_workspace().rules;
    for (ygg::uint_t i = 0; i < program_view.get_rules().size(); ++i)
    {
        const auto is_pick = ygg::visit(
            [&](auto&& head)
            {
                using Head = std::decay_t<decltype(head)>;
                if constexpr (std::is_same_v<Head, fd::AtomView<f::FluentTag>>)
                    return action_program.get_predicate_to_action_mapping().at(head.get_predicate()).get_name() == "pick";
                else
                    return false;
            },
            program_view.get_rules()[i].get_head());

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
    auto kpkc = d::kpkc::DeltaKPKC(static_graph);
    kpkc.set_next_assignment_sets(static_graph, d::AssignmentSets(static_assignments, empty_assignments));
    kpkc.set_next_assignment_sets(static_graph, d::AssignmentSets(static_assignments, initial_assignments));
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

#ifdef TYR_ENABLE_INNER_PARALLELISM
TEST(TyrDatalogLiftedDeltaKPKC, InnerParallelismMatchesSequentialRPG)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task = p::Task<p::LiftedTag>::create(make_test_parser(root / "classical/profiling/rovers-large-simple/domain.pddl")
                                                  .parse_task(root / "classical/profiling/rovers-large-simple/p-r1-w1000-o1-1-g2.pddl"));

    auto sequential_context = ygg::ExecutionContext::create(1);
    auto parallel_context = ygg::ExecutionContext::create(ygg::ExecutionContext::get_max_num_threads());
    auto axiom_evaluator = p::AxiomEvaluatorFactory<p::LiftedTag>().create(task, sequential_context);
    auto state_repository = p::StateRepositoryFactory<p::LiftedTag>().create(task, axiom_evaluator);
    auto successor_generator = p::SuccessorGeneratorFactory<p::LiftedTag>().create(task, sequential_context, state_repository);
    const auto initial_state = successor_generator->get_initial_node().get_state();

    auto sequential = p::FFRPGHeuristic<p::LiftedTag>::create(task, sequential_context);
    auto parallel = p::FFRPGHeuristic<p::LiftedTag>::create(task, parallel_context);
    const auto sequential_value = sequential->evaluate(initial_state);
    const auto parallel_value = parallel->evaluate(initial_state);

    EXPECT_EQ(parallel_value, sequential_value);
    EXPECT_EQ(parallel->get_preferred_actions(), sequential->get_preferred_actions());

    const auto used_inner_parallelism = [](const auto& workspace)
    {
        return std::ranges::any_of(workspace.rules,
                                   [](const auto& rule) { return rule && rule->worker.size() == 2 && rule->worker[1].solve.statistics.num_executions > 0; });
    };
    EXPECT_FALSE(used_inner_parallelism(sequential->get_workspace()));
    EXPECT_TRUE(used_inner_parallelism(parallel->get_workspace()));
}
#endif
}
