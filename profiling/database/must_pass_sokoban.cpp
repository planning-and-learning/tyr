/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Times MustPass on the delivery graphs of one Sokoban task, without a planner: a minimal reader of the task's facts
// builds the relations, and the keyed edge query is the one of a policy's delivery distance (move the agent, or push
// the keyed box), with the agent and the box as vertex columns. Prints the must-pass goal cells of every box and the
// order key "other boxes blocked by this box's goal" of every box.
//
//   must_pass_sokoban <task.pddl> [state.txt] [repeats]
//
// state.txt replaces the initial state: a line "robot <location>", then lines "<box> <location>".

#include "../../tests/unit/database/query_helpers.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/optimization/optimization.hpp"
#include "tyr/datalog/lifted/database/semantics/bound/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/must_pass.hpp"
#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "yggdrasil/formalism/relation_view.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fmt/core.h>
#include <fstream>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using View = ygg::FreeBuilderRelationView;
using ygg::uint_t;
namespace qb = ygg::tests::qb;

/// The layer the profile reads its builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

View borrow(const Relation& relation) { return layer().view(relation); }

/// The atoms "(name args...)" of a text.
urd::Store<urd::sequence<urd::Store<urd::sequence<std::string>>>> atoms(const std::string& text)
{
    auto result = urd::Store<urd::sequence<urd::Store<urd::sequence<std::string>>>>();
    for (size_t open = text.find('('); open != std::string::npos; open = text.find('(', open + 1))
    {
        const auto close = text.find_first_of("()", open + 1);
        if (close == std::string::npos || text[close] != ')')
            continue;
        auto words = urd::Store<urd::sequence<std::string>>();
        auto stream = std::istringstream(text.substr(open + 1, close - open - 1));
        for (std::string word; stream >> word;)
            words.push_back(word);
        if (!words.empty())
            result.push_back(std::move(words));
    }
    return result;
}

struct Task
{
    std::unordered_map<std::string, ygg::uint_t> ids;
    urd::Store<urd::sequence<std::string>> names;
    urd::Store<urd::sequence<std::tuple<ygg::uint_t, ygg::uint_t, ygg::uint_t>>> adjacent;
    urd::Store<urd::sequence<ygg::uint_t>> locations;
    std::map<ygg::uint_t, ygg::uint_t> at;    ///< box -> location
    std::map<ygg::uint_t, ygg::uint_t> goal;  ///< box -> location
    ygg::uint_t robot = 0;

    ygg::uint_t id(const std::string& name)
    {
        const auto [it, added] = ids.emplace(name, static_cast<ygg::uint_t>(names.size()));
        if (added)
            names.push_back(name);
        return it->second;
    }
};

Task read_task(const std::string& path, const std::string& state)
{
    auto file = std::ifstream(path);
    const auto text = std::string(std::istreambuf_iterator<char>(file), {});
    const auto split = text.find(":goal");
    auto task = Task();
    for (const auto& atom : atoms(text.substr(0, split)))
    {
        if (atom[0] == "adjacent" && atom.size() == 4)
            task.adjacent.emplace_back(task.id(atom[1]), task.id(atom[2]), task.id(atom[3]));
        else if (atom[0] == "at" && atom.size() == 3)
            task.at[task.id(atom[1])] = task.id(atom[2]);
        else if (atom[0] == "at-robot" && atom.size() == 2)
            task.robot = task.id(atom[1]);
    }
    for (const auto& atom : atoms(text.substr(split)))
        if (atom[0] == "at" && atom.size() == 3)
            task.goal[task.id(atom[1])] = task.id(atom[2]);
    for (const auto& [from, to, direction] : task.adjacent)
        task.locations.push_back(from);
    std::ranges::sort(task.locations);
    task.locations.erase(std::unique(task.locations.begin(), task.locations.end()), task.locations.end());
    if (!state.empty())
    {
        auto in = std::ifstream(state);
        for (std::string name, location; in >> name >> location;)
            (name == "robot" ? task.robot : task.at[task.id(name)]) = task.id(location);
    }
    return task;
}

/// Admissible and consistent: the pushes a box needs to reach its goal on the empty board, from a key (box) followed by
/// a vertex (agent, box cell). A pattern database over the box column (Culberson & Schaeffer 1998).
struct PushDistance
{
    const Task* task;
    bool obstacles;
    urd::Store<urd::sequence<urd::Store<urd::sequence<std::pair<ygg::uint_t, ygg::uint_t>>>>> into;         ///< cell -> (from, direction) of its incoming edges
    std::map<std::pair<ygg::uint_t, ygg::uint_t>, urd::Store<urd::sequence<ygg::uint_t>>> before;  ///< (cell, direction) -> cells c with c -d-> cell
    std::map<ygg::uint_t, urd::Store<urd::sequence<ygg::uint_t>>> pushes;                     ///< box -> pushes from each location, in the current search
    std::map<ygg::uint_t, urd::Store<urd::sequence<ygg::uint_t>>> base;                       ///< box -> pushes without an excluded cell

    /// With `obstacles`, the other boxes' cells block a box in the current state; still admissible for this state.
    PushDistance(const Task& task_, bool obstacles_) : task(&task_), obstacles(obstacles_), into(task_.names.size())
    {
        for (const auto& [from, to, d] : task->adjacent)
        {
            into[to].emplace_back(from, d);
            before[{ to, d }].push_back(from);
        }
        for (const auto& [b, goal] : task->goal)
            base[b] = distances(b, tyr::datalog::database::infinity);
        pushes = base;
    }
    /// Backward breadth-first pushes to the box's goal: a push from `from` to `to` in direction d needs a cell before
    /// `from` in direction d; blocked cells hold neither the box nor the agent.
    [[nodiscard]] urd::Store<urd::sequence<ygg::uint_t>> distances(ygg::uint_t b, ygg::uint_t excluded) const
    {
        static const bool no_agent_cell = std::getenv("NO_AGENT_CELL") != nullptr;
        auto blocked = std::vector<bool>(task->names.size(), false);
        if (obstacles)
            for (const auto& [other, location] : task->at)
                blocked[location] = other != b;
        if (excluded != tyr::datalog::database::infinity)
            blocked[excluded] = true;
        auto distance = urd::Store<urd::sequence<ygg::uint_t>>(task->names.size(), tyr::datalog::database::infinity);
        const auto goal = task->goal.at(b);
        if (blocked[goal])
            return distance;
        auto frontier = urd::Store<urd::sequence<ygg::uint_t>> { goal };
        distance[goal] = 0;
        for (size_t i = 0; i < frontier.size(); ++i)
            for (const auto& [from, d] : into[frontier[i]])
            {
                if (blocked[from] || distance[from] != tyr::datalog::database::infinity)
                    continue;
                const auto agent = before.find({ from, d });
                // NO_AGENT_CELL: the projection onto the box column alone, where the excluded object only blocks box cells.
                if (agent != before.end()
                    && std::ranges::any_of(agent->second, [&](ygg::uint_t cell) { return !blocked[cell] || (no_agent_cell && cell == excluded); }))
                {
                    distance[from] = distance[frontier[i]] + 1;
                    frontier.push_back(from);
                }
            }
        return distance;
    }
    void exclude(std::span<const std::byte> key, std::span<const std::byte> object)
    {
        auto b = static_cast<ygg::uint_t>(0), x = static_cast<ygg::uint_t>(0);
        std::memcpy(&b, key.data(), sizeof(b));
        if (object.empty())
            pushes[b] = base[b];
        else
        {
            std::memcpy(&x, object.data(), sizeof(x));
            pushes[b] = distances(b, x);
        }
    }
    [[nodiscard]] ygg::uint_t estimate(std::span<const std::byte> key_and_vertex) const
    {
        auto values = std::array<ygg::uint_t, 3> {};
        std::memcpy(values.data(), key_and_vertex.data(), sizeof(values));
        return pushes.at(values[0])[values[2]];
    }
};

double seconds_since(std::chrono::steady_clock::time_point start) { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); }

int run(std::span<char*> args)
{
    if (args.size() < 2)
    {
        fmt::print(stderr, "usage: {} <task.pddl> [state.txt] [repeats]\n", args[0]);
        return 2;
    }
    auto task = read_task(args[1], args.size() > 2 ? args[2] : "");
    const auto repeats = args.size() > 3 ? std::stoul(args[3]) : 1ul;
    const Column box(0), robot(1), boxloc(2), next_robot(3), next_boxloc(4), direction(5), cell(6);

    // The relations of the state.
    auto occupied = std::map<ygg::uint_t, bool>();
    for (const auto& [b, location] : task.at)
        occupied[location] = true;
    auto adjacent = Relation(qb::columns_of({ robot, next_robot, direction }));
    for (const auto& [from, to, d] : task.adjacent)
        adjacent.insert(std::array { ygg::encode_value(from), ygg::encode_value(to), ygg::encode_value(d) });
    auto unfinished = Relation(qb::columns_of({ box }));
    auto clear = Relation(qb::columns_of({ cell }));
    auto at = Relation(qb::columns_of({ box, cell }));
    auto equal = Relation(qb::columns_of({ boxloc, next_boxloc }));
    for (const auto& [b, location] : task.at)
    {
        at.insert(std::array { ygg::encode_value(b), ygg::encode_value(location) });
        if (task.goal.at(b) != location)
            unfinished.insert(std::array { ygg::encode_value(b) });
    }
    for (const auto location : task.locations)
    {
        if (!occupied[location])
            clear.insert(std::array { ygg::encode_value(location) });
        equal.insert(std::array { ygg::encode_value(location), ygg::encode_value(location) });
    }

    // The keyed edge query: free(box, l) = unfinished(box) x clear(l) + at(box, l), i.e. l is clear or the box's own cell.
    auto queries = qb::repository();
    auto slots = std::deque<Relation>();  ///< The rows of each input slot, relabeled to the slot's columns.
    const auto input = [&](const Relation& relation, std::initializer_list<Column> labels)
    {
        auto& rows = slots.emplace_back(qb::columns_of(labels));
        for (const auto row : borrow(relation).rows())
            rows.insert(row);
        return qb::input(queries, slots.size() - 1, labels);
    };
    const auto free = [&](Column at_cell)
    { return qb::union_(queries, qb::join(queries, input(unfinished, { box }), input(clear, { at_cell })), input(at, { box, at_cell })); };
    const auto vertices = { box, robot, boxloc, next_robot, next_boxloc };
    const auto moves = qb::project(
        queries,
        qb::antijoin(
            queries,
            qb::join(queries, qb::join(queries, input(adjacent, { robot, next_robot, direction }), free(next_robot)), input(equal, { boxloc, next_boxloc })),
            input(equal, { next_robot, boxloc })),
        vertices);
    const auto pushes =
        qb::project(queries,
                    qb::join(queries,
                             qb::join(queries,
                                      qb::join(queries, input(adjacent, { robot, boxloc, direction }), input(adjacent, { boxloc, next_boxloc, direction })),
                                      input(equal, { boxloc, next_robot })),
                             free(next_boxloc)),
                    vertices);
    const auto edges = qb::union_(queries, moves, pushes);

    // Sources (box, agent, box cell), targets (box, any agent cell, goal cell) and probes (goal cells of unfinished boxes).
    auto sources = Relation(qb::columns_of({ box, robot, boxloc }));
    auto targets = Relation(qb::columns_of({ box, robot, boxloc }));
    auto probes = Relation(qb::columns_of({ cell }));
    for (size_t r = 0; r < unfinished.size(); ++r)
    {
        const auto b = borrow(unfinished)[r].get<ygg::uint_t>(0);
        sources.insert(std::array { ygg::encode_value(b), ygg::encode_value(task.robot), ygg::encode_value(task.at.at(b)) });
        for (const auto location : task.locations)
            if (!std::getenv("PUSHED_TARGETS")
                || std::ranges::any_of(task.adjacent, [&](const auto& edge) { return std::get<0>(edge) == location && std::get<1>(edge) == task.goal.at(b); }))
                targets.insert(std::array { ygg::encode_value(b), ygg::encode_value(location), ygg::encode_value(task.goal.at(b)) });
        probes.insert(std::array { ygg::encode_value(task.goal.at(b)) });
    }
    const auto plan = tyr::datalog::database::MustPassPlan(sources.columns(), edges.columns(), targets.columns(), probes.columns(), 1);
    auto must_pass = tyr::datalog::database::MustPass<PushDistance>(layer(), edges, plan);
    auto heuristic = PushDistance(task, std::getenv("STATIC_HEURISTIC") == nullptr);
    auto views = urd::Store<urd::sequence<View>>();
    for (const auto& relation : slots)
        views.push_back(borrow(relation));
    const auto slot_input = [&](size_t slot) { return views[slot]; };
    if (std::getenv("BLIND"))  // Without a heuristic, as runir's q_must_pass runs.
    {
        auto blind = tyr::datalog::database::MustPass(layer(), edges, plan);
        auto rows = Relation(plan.output_columns());
        const auto start = std::chrono::steady_clock::now();
        blind.must_pass(slot_input, borrow(sources), borrow(targets), borrow(probes), rows);
        fmt::print("blind: {:.3f} s, {} rows, expanded {}, backward {}\n",
                   seconds_since(start),
                   rows.size(),
                   blind.statistics().vertices,
                   blind.statistics().backward);
    }
    auto out = Relation(plan.output_columns());
    auto total = 0.0;
    for (size_t repeat = 0; repeat < repeats; ++repeat)
    {
        out.clear();
        const auto start = std::chrono::steady_clock::now();
        must_pass.must_pass(slot_input, borrow(sources), borrow(targets), borrow(probes), out, heuristic);
        total += seconds_since(start);
    }
    if (std::getenv("PROJECTED"))  // Times enumerating each key's edges projected onto the box column.
    {
        const auto projected = qb::project(queries, edges, { box, boxloc, next_boxloc });
        auto evaluator = tyr::datalog::database::bound::QueryEvaluator(layer(), projected);
        evaluator.bind(slot_input);
        const auto by_key = evaluator.prepare({ 0 });
        auto rows = static_cast<size_t>(0), loops = static_cast<size_t>(0);
        const auto start = std::chrono::steady_clock::now();
        auto bytes = urd::Store<urd::sequence<std::byte>>();
        for (size_t r = 0; r < unfinished.size(); ++r)
        {
            const auto b = borrow(unfinished)[r].get<ygg::uint_t>(0);
            evaluator.evaluate(by_key,
                               tyr::datalog::database::encode_row<ygg::uint_t>(std::array { b }, bytes),
                               [&](std::span<const std::byte> row)
                               {
                                   auto values = std::array<ygg::uint_t, 3> {};
                                   std::memcpy(values.data(), row.data(), sizeof(values));
                                   ++rows;
                                   loops += values[1] == values[2];
                                   return true;
                               });
        }
        fmt::print("projected edges: {} rows ({} self loops) for {} keys in {:.3f} s\n", rows, loops, unfinished.size(), seconds_since(start));
    }
    if (std::getenv("PROJECTED_FULL"))  // Times the full, optimized evaluation of the edges projected onto the box column.
    {
        const auto projected = qb::project(queries, edges, { box, boxloc, next_boxloc });
        const auto start = std::chrono::steady_clock::now();
        auto evaluator = tyr::datalog::database::QueryEvaluator(layer(), tyr::datalog::database::optimize(projected));
        const auto compiled = seconds_since(start);
        auto loops = static_cast<size_t>(0);
        const auto evaluated = std::chrono::steady_clock::now();
        evaluator.evaluate(views);
        const auto result = evaluator.get_result();
        for (size_t r = 0; r < result.size(); ++r)
            loops += result[r].get<ygg::uint_t>(1) == result[r].get<ygg::uint_t>(2);
        fmt::print("projected edges, full: {} rows ({} self loops), optimize {:.3f} s, evaluate {:.3f} s\n",
                   result.size(),
                   loops,
                   compiled,
                   seconds_since(evaluated));
    }
    if (std::getenv("PER_KEY"))  // Times each key alone.
    {
        auto timings = urd::Store<urd::sequence<std::tuple<double, size_t, std::string>>>();
        for (size_t r = 0; r < unfinished.size(); ++r)
        {
            const auto b = borrow(unfinished)[r].get<ygg::uint_t>(0);
            auto one = Relation(sources.columns());
            one.insert(std::array { ygg::encode_value(b), ygg::encode_value(task.robot), ygg::encode_value(task.at.at(b)) });
            auto alone = tyr::datalog::database::MustPass<PushDistance>(layer(), edges, plan);
            auto rows = Relation(plan.output_columns());
            const auto start = std::chrono::steady_clock::now();
            alone.must_pass(slot_input, borrow(one), borrow(targets), borrow(probes), rows, heuristic);
            timings.emplace_back(seconds_since(start), alone.statistics().vertices, task.names[b] + (alone.statistics().keys == 0 ? " (unreachable)" : ""));
        }
        std::ranges::sort(timings, std::greater<>());
        for (size_t i = 0; i < std::min<size_t>(std::getenv("TOP") ? std::stoul(std::getenv("TOP")) : 5, timings.size()); ++i)
            fmt::print("  {:<22} {:8.3f} s {:10} expanded\n", std::get<2>(timings[i]).c_str(), std::get<0>(timings[i]), std::get<1>(timings[i]));
        auto alone_total = 0.0;
        for (const auto& timing : timings)
            alone_total += std::get<0>(timing);
        fmt::print("  all keys alone: {:.3f} s; top 5: {:.3f} s\n",
                   alone_total,
                   std::get<0>(timings[0]) + (timings.size() > 1 ? std::get<0>(timings[1]) : 0.0));
    }
    const auto& statistics = must_pass.statistics();
    fmt::print("boxes {} unfinished {} locations {}: {:.3f} s per call; keys {}, expanded {}, edges {}, removal searches {}, backward expanded {} (over "
               "{} calls)\n",
               task.at.size(),
               unfinished.size(),
               task.locations.size(),
               total / static_cast<double>(repeats),
               statistics.keys,
               statistics.vertices,
               statistics.edges,
               statistics.removals,
               statistics.backward,
               repeats);

    // The order key of every unfinished box: the other boxes for which its goal cell must be passed.
    auto must = std::map<ygg::uint_t, urd::Store<urd::sequence<ygg::uint_t>>>();
    for (size_t r = 0; r < out.size(); ++r)
        must[borrow(out)[r].get<ygg::uint_t>(0)].push_back(borrow(out)[r].get<ygg::uint_t>(1));
    for (size_t r = 0; r < unfinished.size(); ++r)
    {
        const auto c = borrow(unfinished)[r].get<ygg::uint_t>(0);
        auto blocked = std::string();
        for (const auto& [o, cells] : must)
            if (o != c && std::ranges::find(cells, task.goal.at(c)) != cells.end())
                blocked += " " + task.names[o];
        auto own = std::string();
        for (const auto x : must[c])
            own += " " + task.names[x];
        fmt::print("{:<6} blocks [{} ] must pass [{} ]\n", task.names[c].c_str(), blocked.c_str(), own.c_str());
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    try
    {
        return run(std::span(argv, static_cast<size_t>(argc)));
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "error: {}\n", e.what());
        return 1;
    }
}
