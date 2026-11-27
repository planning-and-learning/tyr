#include "planning/parser.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successor_view.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <urd/elements/view.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests::planning_temporal_successors
{
namespace
{
const auto work_domain = task_fixture("work/domain.pddl");

const auto work_problem = task_fixture("work/work-1.pddl");

/// A transition in a form to compare ground and lifted ones by: its kind, action (name and objects), and the position of
/// the open action an end ends.
using Label = std::tuple<planning::TransitionKind, std::string, std::uint32_t>;

template<typename Successor>
Label label_of(const Successor& successor)
{
    const auto label = successor.get_label();
    auto name = std::string();
    if (const auto binding = label.get_binding())
        urd::visit_view(*binding,
                        [&](const auto& row)
                        {
                            // Its relation in whichever form the row holds it; its objects through the row's repository.
                            urd::visit_view(row.get_relation(),
                                            [&](const auto& relation)
                                            {
                                                name = relation.get_name().str();
                                                for (std::size_t column = 0; column < relation.get_arity(); ++column)
                                                    name += " " + urd::resolve(row.template get<urd::index<formalism::Object>>(column), row.context()).get_name().str();
                                            });
                        });
    return { label.get_kind(), name, label.get_open() };
}

/// A node read through its repository.
template<TaskKind Kind>
planning::NodeBuilderView<Kind> view_of(const planning::NodeBuilder<Kind>& node, const planning::StateRepository<Kind>& repository)
{ return urd::resolve(node, repository); }

/// Calls `callback` with each successor of `node`, its outcome states registered (a node of one is moved out of it).
template<TaskKind Kind, typename Callback>
void for_each_registered_successor(planning::SuccessorGenerator<Kind>& generator,
                                   const planning::NodeBuilder<Kind>& node,
                                   planning::StateRepository<Kind>& repository,
                                   planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                   Callback callback)
{
    generator.for_each_successor(view_of(node, repository),
                                 axiom_evaluator,
                                 [&](planning::SuccessorBuilder<Kind>& successor)
                                 {
                                     planning::register_outcomes(repository, successor);
                                     return callback(successor);
                                 });
}

/// The search space of a task explored breadth first through successors with outcomes: per depth the sorted labels of
/// the transitions generated, and the depth at which a goal state is first reached (each node once).
template<TaskKind Kind>
std::pair<std::vector<std::vector<Label>>, std::optional<std::size_t>> explore(const planning::TaskPtr<Kind>& task, std::size_t max_depth)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = planning::StateRepository<Kind>::create(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    auto initial = planning::get_initial_node(*state_repository, *axiom_evaluator).release();

    auto layers = std::vector<std::vector<Label>>();
    auto goal_depth = std::optional<std::size_t>();
    auto seen = std::set<urd::index<planning::State<Kind>>> {};
    seen.insert(view_of(initial, *state_repository).pack().state);
    auto layer = std::vector<planning::NodeBuilder<Kind>>();
    layer.push_back(std::move(initial));
    const auto goal = task->get_task().get_goal();
    for (std::size_t depth = 0; depth < max_depth && !layer.empty(); ++depth)
    {
        auto labels = std::vector<Label>();
        auto next = std::vector<planning::NodeBuilder<Kind>>();
        for (const auto& node : layer)
            for_each_registered_successor(generator,
                                          node,
                                          *state_repository,
                                          *axiom_evaluator,
                                          [&](planning::SuccessorBuilder<Kind>& successor)
                                          {
                                              labels.push_back(label_of(urd::resolve(std::as_const(successor), std::as_const(*state_repository))));
                                              for (auto& distribution : successor.distributions)
                                                  for (auto& child : urd::builder_of(distribution).outcomes)
                                                  {
                                                      auto& outcome = urd::builder_of(child);
                                                      if (!outcome.failed && seen.insert(view_of(outcome.node, *state_repository).pack().state).second)
                                                          next.push_back(std::move(outcome.node));
                                                  }
                                              return true;
                                          });
        std::ranges::sort(labels);
        layers.push_back(std::move(labels));
        for (const auto& node : next)
            if (!goal_depth && planning::is_applicable(goal, planning::state_context_of(view_of(node, *state_repository))))
                goal_depth = depth + 1;
        layer = std::move(next);
    }
    return { layers, goal_depth };
}

/// Both kinds of a task explored alike.
std::pair<std::vector<std::vector<Label>>, std::optional<std::size_t>> explore_both(formalism::PlanningTask<LiftedTag> formalism_task, std::size_t max_depth)
{
    const auto lifted = planning::Task<LiftedTag>::create(std::move(formalism_task));
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    EXPECT_TRUE(ground);
    const auto result = explore<LiftedTag>(lifted, max_depth);
    if (ground)
        EXPECT_EQ(explore<GroundTag>(ground, max_depth), result);
    return result;
}

using enum planning::TransitionKind;

/// Snap transitions with over-all conditions: from the initial state each action can start; while work runs, block may not
/// start (it would falsify work's over-all condition); an open action can end; both works started and ended reach the
/// goal.
TEST(TyrTests, TemporalTransitionsStartEndAndProtectOverAll)
{
    auto parser = formalism::Parser(work_domain);
    const auto [layers, goal_depth] = explore_both(parser.parse_task(work_problem), 5);

    ASSERT_GE(layers.size(), 2);
    EXPECT_EQ(layers[0], (std::vector<Label> { { START, "block", 0 }, { START, "work o1", 0 }, { START, "work o2", 0 } }));
    // After work(o1) starts: work(o2) starts or work(o1) ends (block may not start); likewise after work(o2); after block
    // starts: it ends (a work started now would have its over-all condition false).
    EXPECT_EQ(layers[1],
              (std::vector<Label> { { START, "work o1", 0 }, { START, "work o2", 0 }, { END, "block", 0 }, { END, "work o1", 0 }, { END, "work o2", 0 } }));
    EXPECT_EQ(goal_depth, std::optional<std::size_t>(4));
}

/// A temporal benchmark is explored alike ground and lifted (lifted without grounding any action).
TEST(TyrTests, TemporalTransitionsOfABenchmark)
{
    const auto domain = std::filesystem::path(BENCHMARKS_DIR) / "temporal" / "tests" / "ipc2002-depots-time-simple-automatic";
    auto parser = formalism::Parser(domain / "domain.pddl");
    const auto [layers, goal_depth] = explore_both(parser.parse_task(domain / "test-1.pddl"), 3);
    ASSERT_EQ(layers.size(), 3);
    EXPECT_FALSE(layers[0].empty());
    EXPECT_TRUE(std::ranges::any_of(layers[1], [](const Label& label) { return std::get<0>(label) == END; }));
}

/// Applies the first transition `matches(label)` accepts, its first outcome: the successor node.
template<TaskKind Kind, typename Matches>
std::optional<planning::NodeBuilder<Kind>> step(planning::SuccessorGenerator<Kind>& generator,
                                                const planning::NodeBuilder<Kind>& node,
                                                planning::StateRepository<Kind>& repository,
                                                planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                                Matches matches)
{
    auto result = std::optional<planning::NodeBuilder<Kind>>();
    for_each_registered_successor(generator,
                                  node,
                                  repository,
                                  axiom_evaluator,
                                  [&](planning::SuccessorBuilder<Kind>& successor)
                                  {
                                      if (matches(label_of(urd::resolve(std::as_const(successor), std::as_const(repository)))))
                                          result = std::move(urd::builder_of(urd::builder_of(successor.distributions.front()).outcomes.front()).node);
                                      return !result;
                                  });
    return result;
}

/// Under (minimize (total-time)), a node's metric is its makespan: both works start at 0 and end at 2.
template<TaskKind Kind>
void expect_makespan_metric(const planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = planning::StateRepository<Kind>::create(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    auto node = planning::get_initial_node(*state_repository, *axiom_evaluator).release();
    for (const auto* name : { "work o1", "work o2" })
    {
        auto started = step(generator,
                                  node,
                                  *state_repository,
                                  *axiom_evaluator,
                                  [&](const Label& label) { return std::get<0>(label) == START && std::get<1>(label) == name; });
        if (!started.has_value())
        {
            ADD_FAILURE() << name << " does not start";
            return;
        }
        EXPECT_DOUBLE_EQ(started->metric, 0);
        node = std::move(*started);
    }
    for (std::size_t i = 0; i < 2; ++i)
    {
        auto ended = step(generator, node, *state_repository, *axiom_evaluator, [](const Label& label) { return std::get<0>(label) == END; });
        if (!ended.has_value())
        {
            ADD_FAILURE() << "no end";
            return;
        }
        EXPECT_DOUBLE_EQ(ended->metric, 2);
        node = std::move(*ended);
    }
}

TEST(TyrTests, TemporalTransitionsMeasureTheMakespan)
{
    const auto problem = task_fixture("work/work-2.pddl");
    auto parser = formalism::Parser(work_domain);
    const auto lifted = planning::Task<LiftedTag>::create(parser.parse_task(problem));
    expect_makespan_metric<LiftedTag>(lifted);
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    expect_makespan_metric<GroundTag>(ground);
}

/// Applies the transitions `labels` accepts in turn, each its first outcome; the metrics of the nodes reached (fewer where
/// one fails).
template<TaskKind Kind>
std::vector<double> run(const planning::TaskPtr<Kind>& task, const std::vector<std::pair<planning::TransitionKind, std::string>>& labels)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = planning::StateRepository<Kind>::create(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    auto nodes = std::vector<planning::NodeBuilder<Kind>>();
    nodes.push_back(planning::get_initial_node(*state_repository, *axiom_evaluator).release());
    for (const auto& [kind, name] : labels)
    {
        auto next = step(generator,
                               nodes.back(),
                               *state_repository,
                               *axiom_evaluator,
                               [&](const Label& label) { return std::get<0>(label) == kind && std::get<1>(label) == name; });
        if (!next.has_value())
            break;
        nodes.push_back(std::move(*next));
    }
    auto metrics = std::vector<double>();
    for (const auto& node : nodes)
        metrics.push_back(node.metric);
    return metrics;
}

/// The metrics of the nodes `run` reaches, ground and lifted (which must agree).
std::vector<double> run_both(const std::filesystem::path& domain, const std::filesystem::path& problem, const std::vector<std::pair<planning::TransitionKind, std::string>>& labels)
{
    auto parser = formalism::Parser(domain);
    const auto lifted = planning::Task<LiftedTag>::create(parser.parse_task(problem));
    const auto metrics = run<LiftedTag>(lifted, labels);
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    EXPECT_TRUE(ground);
    if (ground)
    {
        EXPECT_EQ(run<GroundTag>(ground, labels), metrics);
    }
    return metrics;
}

/// A duration interval: the action may end as soon as its lower bound allows.
TEST(TyrTests, TemporalTransitionsBoundDurations)
{
    const auto domain = task_fixture("charge/domain.pddl");
    const auto problem = task_fixture("charge/charge-1.pddl");
    const auto metrics = run_both(domain, problem, { { START, "charge" }, { END, "charge" } });
    ASSERT_EQ(metrics.size(), 3);
    EXPECT_DOUBLE_EQ(metrics[2], 1);
}

const auto window_domain = task_fixture("window/domain.pddl");

const auto window_problem = task_fixture("window/window-1.pddl");

/// Timed initial literals open and close a window: an action that fits runs inside it (it starts after the opening, its
/// end before the closing); one that does not fit cannot have the closing after its end.
TEST(TyrTests, TemporalTransitionsFitTimedInitialLiterals)
{
    const auto fits = run_both(window_domain, window_problem, { { TIMED_LITERAL, "" }, { START, "go" }, { END, "go" }, { TIMED_LITERAL, "" } });
    ASSERT_EQ(fits.size(), 5);
    EXPECT_NEAR(fits[1], 5, 1e-9);
    EXPECT_NEAR(fits[3], 7, 1e-9);
    EXPECT_NEAR(fits[4], 10, 1e-9);

    // crawl ends at 11 at the earliest: the window cannot close after it.
    const auto overruns = run_both(window_domain, window_problem, { { TIMED_LITERAL, "" }, { START, "crawl" }, { END, "crawl" }, { TIMED_LITERAL, "" } });
    EXPECT_EQ(overruns.size(), 4);
}

/// Required concurrency: cooking needs light over all, which only a burning match gives; the plan nests cooking inside the
/// match, and the same action may also run twice at once.
TEST(TyrTests, TemporalTransitionsAllowRequiredConcurrencyAndSelfOverlap)
{
    const auto domain = task_fixture("kitchen/domain.pddl");
    const auto problem = task_fixture("kitchen/kitchen-1.pddl");
    const auto nested = run_both(domain, problem, { { START, "light-match" }, { START, "cook" }, { END, "cook" }, { END, "light-match" } });
    ASSERT_EQ(nested.size(), 5);
    EXPECT_NEAR(nested[3], 2, 1e-9);
    EXPECT_NEAR(nested[4], 5, 1e-9);
    // Two cooks at once inside the match.
    const auto overlapping = run_both(domain, problem, { { START, "light-match" }, { START, "cook" }, { START, "cook" }, { END, "cook" }, { END, "cook" } });
    EXPECT_EQ(overlapping.size(), 6);
}
}  // namespace
}  // namespace tyr::tests::planning_temporal_successors
