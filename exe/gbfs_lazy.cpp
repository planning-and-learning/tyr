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

#include <tyr/planning/algorithms/gbfs_lazy.hpp>
#include <tyr/planning/algorithms/gbfs_lazy/event_handler.hpp>

int main(int argc, char** argv)
{
    using namespace tyr;

    auto program = argparse::ArgumentParser("Lazy GBFS search.");
    cli::add_common_arguments(program);
    program.add_argument("--max-time").scan<'u', uint64_t>().help("The search time limit in seconds; the search then stops with OUT_OF_TIME.");
    program.add_argument("--disable-preferred-actions").default_value(false).implicit_value(true).help("Disable preferred action queues.");

    return cli::run(
        argc,
        argv,
        program,
        [&] { std::cout << "[INPUT] Use preferred actions: " << !program.get<bool>("--disable-preferred-actions") << '\n'; },
        [&]<TaskKind Kind>(cli::Search<Kind>& search)
        {
            const auto arguments = cli::Arguments(program);
            auto options = planning::gbfs_lazy::Options<Kind>();
            cli::set_common_options(options, arguments, search);
            options.event_handler = planning::gbfs_lazy::DefaultEventHandler<Kind>::create(arguments.verbosity);
            if (const auto max_time = program.present<uint64_t>("--max-time"))
                options.search_budget.max_time = std::chrono::seconds(*max_time);
            options.use_preferred_actions = !program.get<bool>("--disable-preferred-actions");
            return planning::gbfs_lazy::find_solution(*search.task,
                                                      *search.state_repository,
                                                      *search.axiom_evaluator,
                                                      *search.successor_generator,
                                                      *search.heuristic,
                                                      options);
        },
        true);
}
