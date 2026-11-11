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

#ifndef TYR_EXE_SEARCH_CLI_HPP_
#define TYR_EXE_SEARCH_CLI_HPP_

#include <argparse/argparse.hpp>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fmt/ostream.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/formalism/planning/parser.hpp>
#include <tyr/formalism/planning/repository.hpp>
#include <tyr/planning/algorithms/utils.hpp>
#include <tyr/planning/axiom_evaluator.hpp>
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
#include <tyr/planning/lifted/successor_generator.hpp>
#include <tyr/planning/lifted/task.hpp>
#include <tyr/planning/lifted/task_grounder_decl.hpp>
#include <urd/config.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/memory.hpp>
#include <yggdrasil/execution/onetbb.hpp>

/// The command line shared by the search executables: arguments, task parsing (lifted, or ground with -G), heuristics,
/// plan and statistics output. Each executable adds its own arguments and sets its algorithm's options.
namespace tyr::cli
{

/// What every search executable reads from its command line.
struct Arguments
{
    explicit Arguments(const argparse::ArgumentParser& program) :
        domain_filepath(program.get<std::string>("--domain-filepath")),
        problem_filepath(program.get<std::string>("--problem-filepath")),
        plan_filepath(program.get<std::string>("--plan-filepath")),
        num_datalog_threads(program.get<std::size_t>("--num-datalog-threads")),
        num_search_workers(program.get<std::size_t>("--num-search-workers")),
        state_repository_mode(program.get<std::string>("--state-repository-mode")),
        dist_hash_mode(program.get<std::string>("--dist-hash-mode")),
        collect_destination_lock_statistics(program.get<bool>("--collect-destination-lock-statistics")),
        random_seed(program.get<uint64_t>("--random-seed")),
        shuffle_labeled_succ_nodes(program.get<bool>("--shuffle-labeled-succ-nodes")),
        instantiate_ground_task(program.get<bool>("--instantiate-ground-task")),
        disable_invariant_synthesis(program.get<bool>("--disable-invariant-synthesis")),
        heuristic_type(program.get<std::string>("--heuristic-type")),
        heuristic_cost_mode(program.get<std::string>("--heuristic-cost-type") == "unit" ? CostMode::UNIT : CostMode::GENERAL),
        search_cost_mode(program.get<std::string>("--search-cost-type") == "unit" ? CostMode::UNIT : CostMode::GENERAL),
        verbosity(program.get<size_t>("--verbosity"))
    {
    }

    std::string domain_filepath;
    std::string problem_filepath;
    std::string plan_filepath;
    std::size_t num_datalog_threads;
    std::size_t num_search_workers;
    std::string state_repository_mode;
    std::string dist_hash_mode;
    bool collect_destination_lock_statistics;
    uint64_t random_seed;
    bool shuffle_labeled_succ_nodes;
    bool instantiate_ground_task;
    bool disable_invariant_synthesis;
    std::string heuristic_type;
    CostMode heuristic_cost_mode;
    CostMode search_cost_mode;
    size_t verbosity;
};

/// The objects a search runs on.
template<TaskKind Kind>
struct Search
{
    planning::TaskPtr<Kind> task;
    std::shared_ptr<ygg::ExecutionContext> execution_context;
    planning::AxiomEvaluatorPtr<Kind> axiom_evaluator;
    planning::StateRepositoryPtr<Kind> state_repository;
    planning::SuccessorGeneratorPtr<Kind> successor_generator;
    planning::HeuristicPtr<Kind> heuristic;
};

inline void add_common_arguments(argparse::ArgumentParser& program)
{
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
    program.add_argument("-S", "--shuffle-labeled-succ-nodes").default_value(false).implicit_value(true).help("Enable shuffling the labeled successor nodes.");
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
}

template<TaskKind Kind>
planning::HeuristicPtr<Kind> make_heuristic(const Arguments& arguments, const Search<Kind>& search)
{
    const auto& type = arguments.heuristic_type;
    const auto cost_mode = arguments.heuristic_cost_mode;
    if (type == "blind")
        return planning::BlindHeuristic<Kind>::create();
    if (type == "goal_count")
        return planning::GoalCountHeuristic<Kind>::create(search.task);
    if (type == "rpg_add")
        return planning::AddRPGHeuristic<Kind>::create(search.task, search.execution_context, cost_mode);
    if (type == "rpg_max")
        return planning::MaxRPGHeuristic<Kind>::create(search.task, search.execution_context, cost_mode);
    if (type == "rpg_ff")
        return planning::FFRPGHeuristic<Kind>::create(search.task, search.execution_context, cost_mode);
    if (type == "lmcut")
        return planning::LMCutHeuristic<Kind>::create(search.task, search.execution_context, cost_mode);
    throw std::invalid_argument("The heuristic is not implemented.");
}

/// Sets the options every search algorithm shares.
template<typename Options, TaskKind Kind>
void set_common_options(Options& options, const Arguments& arguments, const Search<Kind>& search)
{
    options.start_node = search.successor_generator->get_initial_node(*search.state_repository, *search.axiom_evaluator);
    options.cost_mode = arguments.search_cost_mode;
    options.random_seed = arguments.random_seed;
    options.num_search_workers = arguments.num_search_workers;
    options.dist_hash_mode = arguments.dist_hash_mode == "random" ? planning::DistHashMode::RANDOM : planning::DistHashMode::LMCUT;
    options.collect_destination_lock_statistics = arguments.collect_destination_lock_statistics;
    options.shuffle_labeled_succ_nodes = arguments.shuffle_labeled_succ_nodes;
}

inline void print_summary(const formalism::planning::Repository& repository, const planning::Statistics& statistics)
{
    std::cout << "[Total] Number of objects: " << repository.symbols().template size<formalism::Object>() << '\n';
    std::cout << "[Total] Number of fluent atoms: " << repository.symbols().template size<formalism::Atom<GroundTag, formalism::FluentTag>>() << '\n';
    std::cout << "[Total] Number of derived atoms: " << repository.symbols().template size<formalism::Atom<GroundTag, formalism::DerivedTag>>() << '\n';
    std::cout << "[Total] Number of fluent fterms: " << repository.symbols().template size<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>() << '\n';
    std::cout << "[Total] Action bindings memory usage: " << statistics.get_action_bindings_memory_usage() << " bytes" << '\n';
    std::cout << "[Total] Predicate bindings memory usage: " << statistics.get_predicate_bindings_memory_usage() << " bytes" << '\n';
    std::cout << "[Total] Axiom bindings memory usage: " << statistics.get_axiom_bindings_memory_usage() << " bytes" << '\n';
    std::cout << "[Total] Function bindings memory usage: " << statistics.get_function_bindings_memory_usage() << " bytes" << '\n';
}

template<TaskKind Kind>
void print_search_statistics(const planning::SearchResult<Kind>& result)
{
    fmt::print(std::cout, "[Search] Worker utilization: {}\n", result.get_worker_utilization());
    for (std::size_t i = 0; i < result.worker_statistics.size(); ++i)
    {
        const auto& worker = result.worker_statistics[i];
        fmt::print(std::cout,
                   "[Search] Worker {}: idle={} ns, expanded={}, generated={}, deadends={}, pruned={}, registered={}, state_storage={} bytes\n",
                   i,
                   ygg::to_ns(worker.get_idle_time()),
                   worker.get_num_expanded(),
                   worker.get_num_generated_successors(),
                   worker.get_num_deadends(),
                   worker.get_num_pruned(),
                   worker.get_num_registered_states(),
                   worker.get_state_storage_memory_usage());
        fmt::print(std::cout,
                   "[Search] Worker {} communication: generated_candidates={}, transferred_candidates={}\n",
                   i,
                   worker.get_num_generated_candidates(),
                   worker.get_num_transferred_candidates());
        fmt::print(std::cout,
                   "[Search] Worker {} destination lock: acquisitions={}, wait={} ns, hold={} ns\n",
                   i,
                   worker.get_num_destination_lock_acquisitions(),
                   ygg::to_ns(worker.get_destination_lock_wait_time()),
                   ygg::to_ns(worker.get_destination_lock_hold_time()));
    }
}

/// Builds the search objects for task, runs solve(search) -> SearchResult<Kind>, and writes the plan and statistics.
template<TaskKind Kind, typename Solve>
int run_search(const Arguments& arguments,
               planning::TaskPtr<Kind> task,
               std::shared_ptr<ygg::ExecutionContext> execution_context,
               Solve& solve,
               bool print_component_summaries)
{
    auto search = Search<Kind> { std::move(task), std::move(execution_context) };
    search.axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(search.task, search.execution_context);
    search.state_repository = std::make_shared<planning::StateRepository<Kind>>(search.task, arguments.state_repository_mode == "shared");
    search.successor_generator = std::make_shared<planning::SuccessorGenerator<Kind>>(search.task, search.execution_context);
    search.heuristic = make_heuristic(arguments, search);

    const planning::SearchResult<Kind> result = solve(search);
    print_search_statistics(result);

    if (result.status == planning::SearchStatus::SOLVED)
    {
        auto plan_file = std::ofstream(arguments.plan_filepath);
        if (!plan_file.is_open())
        {
            std::cerr << "Error opening file!" << '\n';
            return 1;
        }
        URD_ASSUME(result.plan.has_value());
        fmt::print(plan_file, "{}", *result.plan);
    }

    // Only the lifted successor generator and axiom evaluator keep statistics.
    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        if (print_component_summaries && arguments.num_search_workers == 1)
        {
            search.successor_generator->print_summary(1);
            search.axiom_evaluator->print_summary(1);
            search.heuristic->print_summary(1);
        }
    }

    print_summary(*search.task->get_repository(), result.statistics);
    std::cout << (arguments.num_search_workers == 1 ? "[Total] States memory usage: " : "[Total] Retained plan states memory usage: ")
              << search.state_repository->memory_usage() << " bytes" << '\n';
    return 0;
}

/// The whole executable: program has its own arguments added; print_inputs prints them; solve is a generic lambda
/// []<TaskKind Kind>(Search<Kind>&) -> SearchResult<Kind> that sets the algorithm's options and runs it.
template<typename PrintInputs, typename Solve>
int run(int argc, char** argv, argparse::ArgumentParser& program, PrintInputs print_inputs, Solve solve, bool print_component_summaries)
{
    try
    {
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

        auto exit_code = 0;
        auto total_time = std::chrono::nanoseconds { 0 };
        {
            auto stop_watch = ygg::StopwatchScope(total_time);

            const auto arguments = Arguments(program);
            if (arguments.num_search_workers == 0)
            {
                std::cerr << "--num-search-workers must be greater than zero." << '\n';
                return 1;
            }

            std::cout << "[INPUT] Num Datalog threads: " << arguments.num_datalog_threads << '\n';
            std::cout << "[INPUT] Num search workers: " << arguments.num_search_workers << '\n';
            std::cout << "[INPUT] State repository mode: " << arguments.state_repository_mode << '\n';
            std::cout << "[INPUT] Distribution hash mode: " << arguments.dist_hash_mode << '\n';
            std::cout << "[INPUT] Collect destination lock statistics: " << arguments.collect_destination_lock_statistics << '\n';
            std::cout << "[INPUT] Random seed: " << arguments.random_seed << '\n';
            std::cout << "[INPUT] Shuffle labeled successor nodes: " << arguments.shuffle_labeled_succ_nodes << '\n';
            print_inputs();

            auto parser_options = loki::semantic::ParserOptions();
            auto parser = formalism::planning::Parser(arguments.domain_filepath, parser_options);
            auto domain = parser.get_domain();
            auto lifted_task = planning::Task<LiftedTag>::create(parser.parse_task(arguments.problem_filepath));

            if (arguments.verbosity > 1)
            {
                fmt::print(std::cout, "{}\n", domain);
                fmt::print(std::cout, "{}\n", *lifted_task);
            }

            auto execution_context = ygg::ExecutionContext::create(arguments.num_datalog_threads);

            if (!arguments.instantiate_ground_task)
            {
                exit_code = run_search(arguments, std::move(lifted_task), std::move(execution_context), solve, print_component_summaries);
            }
            else
            {
                auto ground_task_instantiation_options = planning::GroundTaskInstantiationOptions();
                ground_task_instantiation_options.disable_invariant_synthesis = arguments.disable_invariant_synthesis;
                auto ground_task_instantiation_result = lifted_task->instantiate_ground_task(*execution_context, ground_task_instantiation_options);

                if (ground_task_instantiation_result.status == planning::GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE)
                    std::cout << "[TaskGrounder] Task is unsolvable!" << '\n';
                else if (ground_task_instantiation_result.status == planning::GroundTaskInstantiationStatus::SUCCESS)
                    exit_code = run_search(arguments, ground_task_instantiation_result.task, std::move(execution_context), solve, print_component_summaries);
            }
        }
        if (exit_code != 0)
            return exit_code;

        std::cout << "[Total] Peak memory usage: " << ygg::get_peak_memory_usage_in_bytes() << " bytes" << '\n';
        std::cout << "[Total] Total time: " << ygg::to_ms(total_time) << " ms (" << ygg::to_ns(total_time) << " ns)" << '\n';
        return 0;
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << "\n";
        return 1;
    }
}

}

#endif
