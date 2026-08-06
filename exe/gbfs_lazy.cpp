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

#include <argparse/argparse.hpp>
#include <chrono>
#include <fmt/ostream.h>
#include <fstream>
#include <queue>
#include <tyr/tyr.hpp>
#include <yggdrasil/core/memory.hpp>

using namespace tyr;

namespace
{
void print_summary(const formalism::planning::Repository& repository)
{
    std::cout << "[Total] Number of objects: " << repository.template size<formalism::Object>() << std::endl;
    std::cout << "[Total] Number of fluent atoms: " << repository.template size<formalism::planning::GroundAtom<formalism::FluentTag>>() << std::endl;
    std::cout << "[Total] Number of derived atoms: " << repository.template size<formalism::planning::GroundAtom<formalism::DerivedTag>>() << std::endl;
    std::cout << "[Total] Number of fluent fterms: " << repository.template size<formalism::planning::GroundFunctionTerm<formalism::FluentTag>>() << std::endl;
    std::cout << "[Total] Action bindings memory usage: " << repository.template memory_usage<formalism::RelationBinding<formalism::planning::Action>>()
              << " bytes" << std::endl;
    std::cout << "[Total] Predicate bindings memory usage: "
              << repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::DerivedTag>>>()
              << " bytes" << std::endl;
    std::cout << "[Total] Axiom bindings memory usage: " << repository.template memory_usage<formalism::RelationBinding<formalism::planning::Axiom>>()
              << " bytes" << std::endl;
    std::cout << "[Total] Function bindings memory usage: "
              << repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::StaticTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::FluentTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::AuxiliaryTag>>>()
              << " bytes" << std::endl;
}

template<TaskKind Kind>
void print_search_statistics(const planning::SearchResult<Kind>& result)
{
    fmt::print(std::cout, "[Search] Worker utilization: {}\n", result.get_worker_utilization());
    for (size_t i = 0; i < result.worker_statistics.size(); ++i)
    {
        const auto& worker = result.worker_statistics[i];
        fmt::print(std::cout,
                   "[Search] Worker {}: idle={} ns, expanded={}, generated={}, deadends={}, pruned={}, registered={}, state_storage={} bytes\n",
                   i,
                   ygg::to_ns(worker.get_idle_time()),
                   worker.get_num_expanded(),
                   worker.get_num_generated(),
                   worker.get_num_deadends(),
                   worker.get_num_pruned(),
                   worker.get_num_registered_states(),
                   worker.get_state_storage_memory_usage());
        fmt::print(std::cout,
                   "[Search] Worker {} communication: routed={}, remote={}\n",
                   i,
                   worker.get_num_routed_successors(),
                   worker.get_num_remote_routed_successors());
        fmt::print(std::cout,
                   "[Search] Worker {} destination lock: acquisitions={}, wait={} ns, hold={} ns\n",
                   i,
                   worker.get_num_destination_lock_acquisitions(),
                   ygg::to_ns(worker.get_destination_lock_wait_time()),
                   ygg::to_ns(worker.get_destination_lock_hold_time()));
    }
}
}

int main(int argc, char** argv)
{
    auto program = argparse::ArgumentParser("Lazy GBFS search.");
    program.add_argument("-D", "--domain-filepath").required().help("The path to the PDDL domain file.");
    program.add_argument("-P", "--problem-filepath").required().help("The path to the PDDL problem file.");
    program.add_argument("-O", "--plan-filepath").default_value(std::string("plan.out")).help("The path to the output plan file.");
    program.add_argument("-N", "--num-worker-threads").default_value(size_t(1)).scan<'u', size_t>().help("The number of inner worker threads.");
    program.add_argument("--num-search-workers").default_value(size_t(1)).scan<'u', size_t>().help("The number of search workers.");
    program.add_argument("--state-repository-mode")
        .default_value(std::string("hash-distributed"))
        .choices("hash-distributed", "shared")
        .help("The state repository mode used by parallel search.");
    program.add_argument("--collect-destination-lock-statistics")
        .default_value(false)
        .implicit_value(true)
        .help("Collect destination worker lock wait and hold times.");
    program.add_argument("-R", "--random-seed").default_value(uint64_t(0)).scan<'u', uint64_t>().help("The random seed.");
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
    program.add_argument("--heuristic-cost-type").default_value("general").choices("unit", "general");
    program.add_argument("--search-cost-type").default_value("general").choices("unit", "general");
    program.add_argument("-V", "--verbosity")
        .default_value(size_t(1))
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
        std::exit(1);
    }

    auto total_time = std::chrono::nanoseconds { 0 };
    {
        auto stop_watch = ygg::StopwatchScope(total_time);

        auto domain_filepath = program.get<std::string>("--domain-filepath");
        auto problem_filepath = program.get<std::string>("--problem-filepath");
        auto plan_filepath = program.get<std::string>("--plan-filepath");
        auto num_worker_threads = program.get<std::size_t>("--num-worker-threads");
        auto num_search_workers = program.get<std::size_t>("--num-search-workers");
        auto state_repository_mode_name = program.get<std::string>("--state-repository-mode");
        auto state_repository_mode =
            state_repository_mode_name == "hash-distributed" ? planning::StateRepositoryMode::HASH_DISTRIBUTED : planning::StateRepositoryMode::SHARED;
        auto collect_destination_lock_statistics = program.get<bool>("--collect-destination-lock-statistics");
        auto random_seed = program.get<uint64_t>("--random-seed");
        auto shuffle_labeled_succ_nodes = program.get<bool>("--shuffle-labeled-succ-nodes");
        auto disable_preferred_actions = program.get<bool>("--disable-preferred-actions");
        auto instantiate_ground_task = program.get<bool>("--instantiate-ground-task");
        auto disable_invariant_synthesis = program.get<bool>("--disable-invariant-synthesis");
        auto heuristic_type = program.get<std::string>("--heuristic-type");
        auto heuristic_cost_mode = program.get<std::string>("--heuristic-cost-type") == "unit" ? CostMode::UNIT : CostMode::GENERAL;
        auto search_cost_mode = program.get<std::string>("--search-cost-type") == "unit" ? CostMode::UNIT : CostMode::GENERAL;
        auto verbosity = program.get<size_t>("--verbosity");

        if (num_search_workers == 0)
        {
            std::cerr << "--num-search-workers must be greater than zero." << std::endl;
            return 1;
        }

        if (num_worker_threads > 1 && num_search_workers > 1)
        {
            std::cerr << "--num-worker-threads and --num-search-workers cannot both exceed 1." << std::endl;
            return 1;
        }

        std::cout << "[INPUT] Num worker threads: " << num_worker_threads << std::endl;
        std::cout << "[INPUT] Num search workers: " << num_search_workers << std::endl;
        std::cout << "[INPUT] State repository mode: " << state_repository_mode_name << std::endl;
        std::cout << "[INPUT] Collect destination lock statistics: " << collect_destination_lock_statistics << std::endl;
        std::cout << "[INPUT] Random seed: " << random_seed << std::endl;
        std::cout << "[INPUT] Shuffle labeled successor nodes: " << shuffle_labeled_succ_nodes << std::endl;
        std::cout << "[INPUT] Use preferred actions: " << !disable_preferred_actions << std::endl;

        auto parser_options = loki::ParserOptions();
        // parser_options.strict = true;
        auto parser = formalism::planning::Parser(domain_filepath, parser_options);
        auto domain = parser.get_domain();

        auto lifted_task = planning::Task<LiftedTag>::create(parser.parse_task(problem_filepath));

        if (verbosity > 1)
            fmt::print(std::cout, "{}\n", domain);

        if (verbosity > 1)
            fmt::print(std::cout, "{}\n", *lifted_task);

        auto execution_context = ygg::ExecutionContext::create(num_worker_threads);

        if (!instantiate_ground_task)
        {
            auto axiom_evaluator = planning::AxiomEvaluatorFactory<LiftedTag>().create(lifted_task, execution_context);
            auto state_repository = planning::StateRepositoryFactory<LiftedTag>().create(lifted_task, axiom_evaluator);
            auto successor_generator = planning::SuccessorGeneratorFactory<LiftedTag>().create(lifted_task, execution_context, state_repository);

            auto options = planning::gbfs_lazy::Options<LiftedTag>();
            options.start_node = successor_generator->get_initial_node();
            options.event_handler = planning::gbfs_lazy::DefaultEventHandler<LiftedTag>::create(verbosity);
            options.cost_mode = search_cost_mode;
            options.random_seed = random_seed;
            options.num_search_workers = num_search_workers;
            options.state_repository_mode = state_repository_mode;
            options.collect_destination_lock_statistics = collect_destination_lock_statistics;
            options.use_preferred_actions = !disable_preferred_actions;
            options.shuffle_labeled_succ_nodes = shuffle_labeled_succ_nodes;

            auto heuristic = std::shared_ptr<planning::Heuristic<LiftedTag>> { nullptr };
            if (heuristic_type == "blind")
                heuristic = planning::BlindHeuristic<LiftedTag>::create();
            else if (heuristic_type == "goal_count")
                heuristic = planning::GoalCountHeuristic<LiftedTag>::create(lifted_task);
            else if (heuristic_type == "rpg_add")
                heuristic = planning::AddRPGHeuristic<LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else if (heuristic_type == "rpg_max")
                heuristic = planning::MaxRPGHeuristic<LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else if (heuristic_type == "rpg_ff")
                heuristic = planning::FFRPGHeuristic<LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else if (heuristic_type == "lmcut")
                heuristic = planning::LMCutHeuristic<LiftedTag>::create(lifted_task, execution_context, heuristic_cost_mode);
            else
                throw std::invalid_argument("The heuristic is not implemented.");

            auto result = planning::gbfs_lazy::find_solution(*lifted_task, *successor_generator, *heuristic, options);
            print_search_statistics(result);

            if (result.status == planning::SearchStatus::SOLVED)
            {
                std::ofstream plan_file;
                plan_file.open(plan_filepath);
                if (!plan_file.is_open())
                {
                    std::cerr << "Error opening file!" << std::endl;
                    return 1;
                }
                fmt::print(plan_file, "{}", result.plan.value());
                plan_file.close();
            }

            if (num_search_workers == 1)
            {
                successor_generator->print_summary(1);
                if (successor_generator->get_state_repository()->get_axiom_evaluator())
                    successor_generator->get_state_repository()->get_axiom_evaluator()->print_summary(1);
                heuristic->print_summary(1);
            }

            print_summary(*lifted_task->get_repository());
            std::cout << (num_search_workers == 1 ? "[Total] States memory usage: " : "[Total] Retained plan states memory usage: ")
                      << successor_generator->get_state_repository()->memory_usage() << " bytes" << std::endl;
        }
        else
        {
            auto ground_task_instantiation_options = planning::GroundTaskInstantiationOptions();
            ground_task_instantiation_options.disable_invariant_synthesis = disable_invariant_synthesis;
            auto ground_task_instantiation_result = lifted_task->instantiate_ground_task(*execution_context, ground_task_instantiation_options);

            if (ground_task_instantiation_result.status == planning::GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE)
            {
                std::cout << "[TaskGrounder] Task is unsolvable!" << std::endl;
            }
            else if (ground_task_instantiation_result.status == planning::GroundTaskInstantiationStatus::SUCCESS)
            {
                auto ground_task = ground_task_instantiation_result.task;

                auto axiom_evaluator = planning::AxiomEvaluatorFactory<GroundTag>().create(ground_task, execution_context);
                auto state_repository = planning::StateRepositoryFactory<GroundTag>().create(ground_task, axiom_evaluator);
                auto successor_generator = planning::SuccessorGeneratorFactory<GroundTag>().create(ground_task, execution_context, state_repository);

                auto options = planning::gbfs_lazy::Options<GroundTag>();
                options.start_node = successor_generator->get_initial_node();
                options.event_handler = planning::gbfs_lazy::DefaultEventHandler<GroundTag>::create(verbosity);
                options.cost_mode = search_cost_mode;
                options.random_seed = random_seed;
                options.num_search_workers = num_search_workers;
                options.state_repository_mode = state_repository_mode;
                options.collect_destination_lock_statistics = collect_destination_lock_statistics;
                options.use_preferred_actions = !disable_preferred_actions;
                options.shuffle_labeled_succ_nodes = shuffle_labeled_succ_nodes;

                auto heuristic = std::shared_ptr<planning::Heuristic<GroundTag>> { nullptr };
                if (heuristic_type == "blind")
                    heuristic = planning::BlindHeuristic<GroundTag>::create();
                else if (heuristic_type == "goal_count")
                    heuristic = planning::GoalCountHeuristic<GroundTag>::create(ground_task);
                else if (heuristic_type == "rpg_add")
                    heuristic = planning::AddRPGHeuristic<GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else if (heuristic_type == "rpg_max")
                    heuristic = planning::MaxRPGHeuristic<GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else if (heuristic_type == "rpg_ff")
                    heuristic = planning::FFRPGHeuristic<GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else if (heuristic_type == "lmcut")
                    heuristic = planning::LMCutHeuristic<GroundTag>::create(ground_task, execution_context, heuristic_cost_mode);
                else
                    throw std::invalid_argument("The heuristic is not implemented.");

                auto result = planning::gbfs_lazy::find_solution(*ground_task, *successor_generator, *heuristic, options);
                print_search_statistics(result);

                if (result.status == planning::SearchStatus::SOLVED)
                {
                    std::ofstream plan_file;
                    plan_file.open(plan_filepath);
                    if (!plan_file.is_open())
                    {
                        std::cerr << "Error opening file!" << std::endl;
                        return 1;
                    }
                    fmt::print(plan_file, "{}", result.plan.value());
                    plan_file.close();
                }

                print_summary(*ground_task->get_repository());
                std::cout << (num_search_workers == 1 ? "[Total] States memory usage: " : "[Total] Retained plan states memory usage: ")
                          << successor_generator->get_state_repository()->memory_usage() << " bytes" << std::endl;
            }
        }
    }

    std::cout << "[Total] Peak memory usage: " << ygg::get_peak_memory_usage_in_bytes() << " bytes" << std::endl;
    std::cout << "[Total] Total time: " << ygg::to_ms(total_time) << " ms (" << ygg::to_ns(total_time) << " ns)" << std::endl;

    return 0;
}
