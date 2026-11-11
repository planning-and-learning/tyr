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

#include "search_cli.hpp"

#include <tyr/planning/algorithms/astar_eager.hpp>
#include <tyr/planning/algorithms/astar_eager/event_handler.hpp>

int main(int argc, char** argv)
try
{
    using namespace tyr;

    auto program = argparse::ArgumentParser("AStar eager search.");
    cli::add_common_arguments(program);
    program.add_argument("--parallel-search-mode")
        .default_value(std::string("synchronous"))
        .choices("synchronous", "asynchronous")
        .help("The coordination mode used by parallel A* search.");

    return cli::run(
        argc,
        argv,
        program,
        [&] { std::cout << "[INPUT] Parallel search mode: " << program.get<std::string>("--parallel-search-mode") << '\n'; },
        [&]<TaskKind Kind>(cli::Search<Kind>& search)
        {
            const auto arguments = cli::Arguments(program);
            auto options = planning::astar_eager::Options<Kind>();
            cli::set_common_options(options, arguments, search);
            options.event_handler = planning::astar_eager::DefaultEventHandler<Kind>::create(arguments.verbosity);
            options.parallel_search_mode = program.get<std::string>("--parallel-search-mode") == "synchronous" ?
                                               planning::astar_eager::ParallelSearchMode::SYNCHRONOUS :
                                               planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS;
            return planning::astar_eager::find_solution(*search.task,
                                                        *search.state_repository,
                                                        *search.axiom_evaluator,
                                                        *search.successor_generator,
                                                        *search.heuristic,
                                                        options);
        },
        false);
}
catch (const std::exception& err)
{
    std::cerr << err.what() << "\n";
    return 1;
}
