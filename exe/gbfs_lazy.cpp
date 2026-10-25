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

#include "search_output.hpp"

#include <argparse/argparse.hpp>
#include <chrono>
#include <exception>
#include <fmt/ostream.h>
#include <fstream>
#include <queue>
#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/formalism/planning/parser.hpp>
#include <tyr/planning/algorithms/gbfs_lazy.hpp>
#include <tyr/planning/algorithms/gbfs_lazy/event_handler.hpp>
#include <tyr/planning/algorithms/utils.hpp>
#include <tyr/planning/factory.hpp>
#include <tyr/planning/formatter.hpp>
#include <tyr/planning/ground/successor_generator.hpp>
#include <tyr/planning/ground/task.hpp>
#include <tyr/planning/heuristic.hpp>
#include <tyr/planning/heuristics/blind.hpp>
#include <tyr/planning/heuristics/goal_count.hpp>
#include <tyr/planning/heuristics/lmcut.hpp>
#include <tyr/planning/heuristics/rpg_add.hpp>
#include <tyr/planning/heuristics/rpg_ff.hpp>
#include <tyr/planning/heuristics/rpg_max.hpp>
#include <tyr/planning/lifted/axiom_evaluator.hpp>
#include <tyr/planning/lifted/successor_generator.hpp>
#include <tyr/planning/lifted/task.hpp>
#include <tyr/planning/lifted/task_grounder_decl.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/memory.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace
{
int run(int argc, char** argv)
{
    auto program = argparse::ArgumentParser("Lazy GBFS search.");
    program.add_argument("-D", "--domain-filepath").required().help("The path to the PDDL domain file.");
    program.add_argument("-P", "--problem-filepath").required().help("The path to the PDDL problem file.");
    program.add_argument("-O", "--plan-filepath").default_value(std::string("plan.out")).help("The path to the output plan file.");
    program.add_argument("-N", "--num-datalog-threads").default_value(static_cast<size_t>(1)).scan<'u', size_t>().help("The number of Datalog threads.");
    program.add_argument("-M", "--num-search-workers").default_value(static_cast<size_t>(1)).scan<'u', size_t>().help("The number of search workers.");
    program.add_argument("--state-repository-mode")
        .default_value(std::string("hash-distributed"))
        .choices("hash-distributed", "shared")
        .help("The state repository mode used by parallel search.");
    program.add_argument("--dist-hash-mode")
        .default_value(std::string("random"))
        .choices("random", "lmcut")
        .help("The state hash used by hash-distributed parallel search.");
    program.add_argument("--collect-destination-lock-statistics")
        .default_value(false)
        .implicit_value(true)
        .help("Collect destination worker lock wait and hold times.");
    program.add_argument("-R", "--random-seed").default_value(static_cast<uint64_t>(0)).scan<'u', uint64_t>().help("The random seed.");
    program.add_argument("-S", "--shuffle-labeled-succ-nodes").default_value(false).implicit_value(true).help("Toggle shuffling the labeled successor nodes.");
    program.add_argument("--disable-preferred-actions").default_value(false).implicit_value(true).help("Disable preferred action queues.");
    program.add_argument("-G", "--instantiate-ground-task")
        .default_value(false)
        .implicit_value(true)
        .help("Enable instantiating the ground task before search.");
    program.add_argument("--disable-invariant-synthesis")
        .default_value(false)
        .implicit_value(true)
        .help("Disable invariant synthesis during ground task instantiation.");
    program.add_argument("-H", "--heuristic-type").default_value("blind").choices("blind", "goal_count", "rpg_max", "rpg_add", "rpg_ff", "lmcut");
    program.add_argument("--heuristic-cost-type").default_value("unit").choices("unit", "general");
    program.add_argument("--search-cost-type").default_value("general").choices("unit", "general");
    program.add_argument("-V", "--verbosity")
        .default_value(static_cast<size_t>(1))
        .scan<'u', size_t>()
        .help("The verbosity level. Defaults to minimal amount of debug output.");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return 1;
    }

    auto total_time = std::chrono::nanoseconds { 0 };
    {
        auto stop_watch = ygg::StopwatchScope(total_time);

        auto domain_filepath = program.get<std::string>("--domain-filepath");
        auto problem_filepath = program.get<std::string>("--problem-filepath");
        auto plan_filepath = program.get<std::string>("--plan-filepath");
        auto num_datalog_threads = program.get<std::size_t>("--num-datalog-threads");
        auto num_search_workers = program.get<std::size_t>("--num-search-workers");
        auto state_repository_mode_name = program.get<std::string>("--state-repository-mode");
        auto dist_hash_mode_name = program.get<std::string>("--dist-hash-mode");
        auto dist_hash_mode = dist_hash_mode_name == "random" ? tyr::planning::DistHashMode::RANDOM : tyr::planning::DistHashMode::LMCUT;
        auto collect_destination_lock_statistics = program.get<bool>("--collect-destination-lock-statistics");
        auto random_seed = program.get<uint64_t>("--random-seed");
        auto shuffle_labeled_succ_nodes = program.get<bool>("--shuffle-labeled-succ-nodes");
        auto disable_preferred_actions = program.get<bool>("--disable-preferred-actions");
        auto instantiate_ground_task = program.get<bool>("--instantiate-ground-task");
        auto disable_invariant_synthesis = program.get<bool>("--disable-invariant-synthesis");
        auto heuristic_type = program.get<std::string>("--heuristic-type");
        auto heuristic_cost_mode = program.get<std::string>("--heuristic-cost-type") == "unit" ? tyr::CostMode::UNIT : tyr::CostMode::GENERAL;
        auto search_cost_mode = program.get<std::string>("--search-cost-type") == "unit" ? tyr::CostMode::UNIT : tyr::CostMode::GENERAL;
        auto verbosity = program.get<size_t>("--verbosity");

        if (num_search_workers == 0)
        {
            std::cerr << "--num-search-workers must be greater than zero." << '\n';
            return 1;
        }

        std::cout << "[INPUT] Num Datalog threads: " << num_datalog_threads << '\n';
        std::cout << "[INPUT] Num search workers: " << num_search_workers << '\n';
        std::cout << "[INPUT] State repository mode: " << state_repository_mode_name << '\n';
        std::cout << "[INPUT] Distribution hash mode: " << dist_hash_mode_name << '\n';
        std::cout << "[INPUT] Collect destination lock statistics: " << collect_destination_lock_statistics << '\n';
        std::cout << "[INPUT] Random seed: " << random_seed << '\n';
        std::cout << "[INPUT] Shuffle labeled successor nodes: " << shuffle_labeled_succ_nodes << '\n';
        std::cout << "[INPUT] Use preferred actions: " << !disable_preferred_actions << '\n';

        auto parser_options = loki::semantic::ParserOptions();
        // parser_options.strict = true;
        auto parser = tyr::formalism::planning::Parser(domain_filepath, parser_options);
        auto domain = parser.get_domain();

        auto lifted_task = tyr::planning::Task<tyr::LiftedTag>::create(parser.parse_task(problem_filepath));

        if (verbosity > 1)
            fmt::print(std::cout, "{}\n", domain);

        if (verbosity > 1)
            fmt::print(std::cout, "{}\n", *lifted_task);

        auto execution_context = ygg::ExecutionContext::create(num_datalog_threads);

        if (!instantiate_ground_task)
        {
            auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<tyr::LiftedTag>().create(lifted_task, execution_context);
            auto state_repository_factory = tyr::planning::StateRepositoryFactory<tyr::LiftedTag>();
            auto state_repository =
                state_repository_mode_name == "shared" ? state_repository_factory.create_concurrent(lifted_task) : state_repository_factory.create(lifted_task);
            auto successor_generator = tyr::planning::SuccessorGeneratorFactory<tyr::LiftedTag>().create(lifted_task, execution_context);

            auto options = tyr::planning::gbfs_lazy::Options<tyr::LiftedTag>();
            options.start_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
            options.event_handler = tyr::planning::gbfs_lazy::DefaultEventHandler<tyr::LiftedTag>::create(verbosity);
            options.cost_mode = search_cost_mode;
            options.random_seed = random_seed;
            options.num_search_workers = num_search_workers;
            options.dist_hash_mode = dist_hash_mode;
            options.collect_destination_lock_statistics = collect_destination_lock_statistics;
            options.use_preferred_actions = !disable_preferred_actions;
            options.shuffle_labeled_succ_nodes = shuffle_labeled_succ_nodes;

            auto heuristic = std::shared_ptr<tyr::planning::Heuristic<tyr::LiftedTag>> { nullptr };
            if (heuristic_type == "blind")
                heuristic = tyr::planning::BlindHeuristic<tyr::LiftedTag>::create();
            else if (heuristic_type == "goal_count")
                heuristic = tyr::planning::GoalCountHeuristic<tyr::LiftedTag>::create(lifted_task);
            else if (heuristic_type == "rpg_add")
                heuristic = tyr::planning::AddRPGHeuristic<tyr::LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else if (heuristic_type == "rpg_max")
                heuristic = tyr::planning::MaxRPGHeuristic<tyr::LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else if (heuristic_type == "rpg_ff")
                heuristic = tyr::planning::FFRPGHeuristic<tyr::LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else if (heuristic_type == "lmcut")
                heuristic = tyr::planning::LMCutHeuristic<tyr::LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else
                throw std::invalid_argument("The heuristic is not implemented.");

            auto result = tyr::planning::gbfs_lazy::find_solution(*lifted_task, *state_repository, *axiom_evaluator, *successor_generator, *heuristic, options);
            tyr::cli::print_search_statistics(result);

            if (result.status == tyr::planning::SearchStatus::SOLVED)
            {
                std::ofstream plan_file;
                plan_file.open(plan_filepath);
                if (!plan_file.is_open())
                {
                    std::cerr << "Error opening file!" << '\n';
                    return 1;
                }
                YGG_ASSUME(result.plan.has_value());
                fmt::print(plan_file, "{}", *result.plan);
                plan_file.close();
            }

            if (num_search_workers == 1)
            {
                successor_generator->print_summary(1);
                axiom_evaluator->print_summary(1);
                heuristic->print_summary(1);
            }

            tyr::cli::print_summary(*lifted_task->get_repository());
            std::cout << (num_search_workers == 1 ? "[Total] States memory usage: " : "[Total] Retained plan states memory usage: ")
                      << state_repository->memory_usage() << " bytes" << '\n';
        }
        else
        {
            auto ground_task_instantiation_options = tyr::planning::GroundTaskInstantiationOptions();
            ground_task_instantiation_options.disable_invariant_synthesis = disable_invariant_synthesis;
            auto ground_task_instantiation_result = lifted_task->instantiate_ground_task(*execution_context, ground_task_instantiation_options);

            if (ground_task_instantiation_result.status == tyr::planning::GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE)
            {
                std::cout << "[TaskGrounder] Task is unsolvable!" << '\n';
            }
            else if (ground_task_instantiation_result.status == tyr::planning::GroundTaskInstantiationStatus::SUCCESS)
            {
                auto ground_task = ground_task_instantiation_result.task;

                auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<tyr::GroundTag>().create(ground_task, execution_context);
                auto state_repository_factory = tyr::planning::StateRepositoryFactory<tyr::GroundTag>();
                auto state_repository = state_repository_mode_name == "shared" ? state_repository_factory.create_concurrent(ground_task) :
                                                                                 state_repository_factory.create(ground_task);
                auto successor_generator = tyr::planning::SuccessorGeneratorFactory<tyr::GroundTag>().create(ground_task, execution_context);

                auto options = tyr::planning::gbfs_lazy::Options<tyr::GroundTag>();
                options.start_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
                options.event_handler = tyr::planning::gbfs_lazy::DefaultEventHandler<tyr::GroundTag>::create(verbosity);
                options.cost_mode = search_cost_mode;
                options.random_seed = random_seed;
                options.num_search_workers = num_search_workers;
                options.dist_hash_mode = dist_hash_mode;
                options.collect_destination_lock_statistics = collect_destination_lock_statistics;
                options.use_preferred_actions = !disable_preferred_actions;
                options.shuffle_labeled_succ_nodes = shuffle_labeled_succ_nodes;

                auto heuristic = std::shared_ptr<tyr::planning::Heuristic<tyr::GroundTag>> { nullptr };
                if (heuristic_type == "blind")
                    heuristic = tyr::planning::BlindHeuristic<tyr::GroundTag>::create();
                else if (heuristic_type == "goal_count")
                    heuristic = tyr::planning::GoalCountHeuristic<tyr::GroundTag>::create(ground_task);
                else if (heuristic_type == "rpg_add")
                    heuristic = tyr::planning::AddRPGHeuristic<tyr::GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else if (heuristic_type == "rpg_max")
                    heuristic = tyr::planning::MaxRPGHeuristic<tyr::GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else if (heuristic_type == "rpg_ff")
                    heuristic = tyr::planning::FFRPGHeuristic<tyr::GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else if (heuristic_type == "lmcut")
                    heuristic = tyr::planning::LMCutHeuristic<tyr::GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else
                    throw std::invalid_argument("The heuristic is not implemented.");

                auto result =
                    tyr::planning::gbfs_lazy::find_solution(*ground_task, *state_repository, *axiom_evaluator, *successor_generator, *heuristic, options);
                tyr::cli::print_search_statistics(result);

                if (result.status == tyr::planning::SearchStatus::SOLVED)
                {
                    std::ofstream plan_file;
                    plan_file.open(plan_filepath);
                    if (!plan_file.is_open())
                    {
                        std::cerr << "Error opening file!" << '\n';
                        return 1;
                    }
                    YGG_ASSUME(result.plan.has_value());
                    fmt::print(plan_file, "{}", *result.plan);
                    plan_file.close();
                }

                tyr::cli::print_summary(*ground_task->get_repository());
                std::cout << (num_search_workers == 1 ? "[Total] States memory usage: " : "[Total] Retained plan states memory usage: ")
                          << state_repository->memory_usage() << " bytes" << '\n';
            }
        }
    }

    std::cout << "[Total] Peak memory usage: " << ygg::get_peak_memory_usage_in_bytes() << " bytes" << '\n';
    std::cout << "[Total] Total time: " << ygg::to_ms(total_time) << " ms (" << ygg::to_ns(total_time) << " ns)" << '\n';

    return 0;
}
}  // namespace

int main(int argc, char** argv)
{
    try
    {
        return run(argc, argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << "\n";
        return 1;
    }
}
