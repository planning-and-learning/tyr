#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_repository.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests::planning_temporal_successors
{
namespace
{
const auto* const work_domain = R"(
(define (domain work)
  (:requirements :strips :typing :durative-actions)
  (:types obj)
  (:predicates (a ?x - obj) (b ?x - obj) (free))
  (:durative-action work
    :parameters (?x - obj)
    :duration (= ?duration 2)
    :condition (and (at start (a ?x)) (over all (free)))
    :effect (and (at start (not (a ?x))) (at end (b ?x))))
  (:durative-action block
    :parameters ()
    :duration (= ?duration 1)
    :condition (at start (free))
    :effect (and (at start (not (free))) (at end (free)))))
)";

const auto* const work_problem = R"(
(define (problem work-1) (:domain work) (:objects o1 o2 - obj) (:init (a o1) (a o2) (free)) (:goal (and (b o1) (b o2))))
)";

/// A transition in a form to compare ground and lifted ones by: its kind, action (name and objects), and the position of
/// the open action an end ends.
using Label = std::tuple<planning::TransitionKind, std::string, std::uint32_t>;

template<typename Successor>
Label label_of(const Successor& successor)
{
    const auto label = successor.get_label();
    auto name = std::string();
    if (const auto binding = label.get_binding())
    {
        name = binding->get_relation().get_name().str();
        for (const auto object : binding->template resolve<formalism::Object>())
            name += " " + object.get_name().str();
    }
    return { label.get_kind(), name, label.get_open() };
}

/// The node of a registered outcome: its state (its key, read through its repository) and its metric.
template<TaskKind Kind, typename Outcome>
planning::Node<Kind> registered_node(const Outcome& outcome)
{
    const auto state = std::get<0>(outcome.get_state());
    return planning::Node<Kind>(state.context().get_registered_state(state.index()), outcome.get_metric());
}

/// The search space of a task explored breadth first through successors with outcomes: per depth the sorted labels of
/// the transitions generated, and the depth at which a goal state is first reached (each node once).
template<TaskKind Kind>
std::pair<std::vector<std::vector<Label>>, std::optional<std::size_t>> explore(const planning::TaskPtr<Kind>& task, std::size_t max_depth)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = std::make_shared<planning::StateRepository<Kind>>(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    const auto initial = generator.get_initial_node(*state_repository, *axiom_evaluator);

    auto layers = std::vector<std::vector<Label>>();
    auto goal_depth = std::optional<std::size_t>();
    auto seen = urd::Store<urd::set<planning::StateView<Kind>>> {};
    seen.insert(initial.get_state());
    auto layer = std::vector<planning::Node<Kind>> { initial };
    const auto goal = task->get_task().get_goal();
    for (std::size_t depth = 0; depth < max_depth && !layer.empty(); ++depth)
    {
        auto labels = std::vector<Label>();
        auto next = std::vector<planning::Node<Kind>>();
        for (const auto& node : layer)
            generator.for_each_successor(node,
                                         *state_repository,
                                         *axiom_evaluator,
                                         [&](const planning::SuccessorView<Kind>& successor)
                                         {
                                             labels.push_back(label_of(successor));
                                             for (const auto distribution : successor.get_distributions())
                                                 for (const auto outcome : distribution.get_outcomes())
                                                 {
                                                     const auto node = registered_node<Kind>(outcome);
                                                     if (!outcome.get_failed() && seen.insert(node.get_state()).second)
                                                         next.push_back(node);
                                                 }
                                             return true;
                                         });
        std::ranges::sort(labels);
        layers.push_back(std::move(labels));
        for (const auto& node : next)
            if (!goal_depth && planning::is_applicable(goal, planning::StateContext<Kind>(*task, node.get_state(), ygg::float_t { 0 })))
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
    auto parser = formalism::Parser(std::string(work_domain), std::nullopt);
    const auto [layers, goal_depth] = explore_both(parser.parse_task(std::string(work_problem), std::nullopt), 5);

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
std::optional<planning::Node<Kind>> step(planning::SuccessorGenerator<Kind>& generator,
                                         const planning::Node<Kind>& node,
                                         planning::StateRepository<Kind>& repository,
                                         planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                         Matches matches)
{
    auto result = std::optional<planning::Node<Kind>>();
    generator.for_each_successor(node,
                                 repository,
                                 axiom_evaluator,
                                 [&](const planning::SuccessorView<Kind>& successor)
                                 {
                                     if (matches(label_of(successor)))
                                         result = registered_node<Kind>(successor.get_distributions().front().get_outcomes().front());
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
    auto state_repository = std::make_shared<planning::StateRepository<Kind>>(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    auto node = generator.get_initial_node(*state_repository, *axiom_evaluator);
    for (const auto* name : { "work o1", "work o2" })
    {
        const auto started = step(generator,
                                  node,
                                  *state_repository,
                                  *axiom_evaluator,
                                  [&](const Label& label) { return std::get<0>(label) == START && std::get<1>(label) == name; });
        if (!started.has_value())
        {
            ADD_FAILURE() << name << " does not start";
            return;
        }
        EXPECT_DOUBLE_EQ(started->get_metric(), 0);
        node = *started;
    }
    for (std::size_t i = 0; i < 2; ++i)
    {
        const auto ended = step(generator, node, *state_repository, *axiom_evaluator, [](const Label& label) { return std::get<0>(label) == END; });
        if (!ended.has_value())
        {
            ADD_FAILURE() << "no end";
            return;
        }
        EXPECT_DOUBLE_EQ(ended->get_metric(), 2);
        node = *ended;
    }
}

TEST(TyrTests, TemporalTransitionsMeasureTheMakespan)
{
    const auto problem = std::string(R"(
(define (problem work-2) (:domain work) (:objects o1 o2 - obj) (:init (a o1) (a o2) (free)) (:goal (and (b o1) (b o2)))
  (:metric minimize (total-time)))
)");
    auto parser = formalism::Parser(std::string(work_domain), std::nullopt);
    const auto lifted = planning::Task<LiftedTag>::create(parser.parse_task(problem, std::nullopt));
    expect_makespan_metric<LiftedTag>(lifted);
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    expect_makespan_metric<GroundTag>(ground);
}

/// Applies the transitions `labels` accepts in turn, each its first outcome; the nodes reached (fewer where one fails).
template<TaskKind Kind>
std::vector<planning::Node<Kind>> run(const planning::TaskPtr<Kind>& task, const std::vector<std::pair<planning::TransitionKind, std::string>>& labels)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = std::make_shared<planning::StateRepository<Kind>>(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    auto nodes = std::vector<planning::Node<Kind>> { generator.get_initial_node(*state_repository, *axiom_evaluator) };
    for (const auto& [kind, name] : labels)
    {
        const auto next = step(generator,
                               nodes.back(),
                               *state_repository,
                               *axiom_evaluator,
                               [&](const Label& label) { return std::get<0>(label) == kind && std::get<1>(label) == name; });
        if (!next.has_value())
            break;
        nodes.push_back(*next);
    }
    return nodes;
}

/// The metrics of the nodes `run` reaches, ground and lifted (which must agree).
std::vector<double> run_both(const std::string& domain, const std::string& problem, const std::vector<std::pair<planning::TransitionKind, std::string>>& labels)
{
    auto parser = formalism::Parser(domain, std::nullopt);
    const auto lifted = planning::Task<LiftedTag>::create(parser.parse_task(problem, std::nullopt));
    auto metrics = std::vector<double>();
    for (const auto& node : run<LiftedTag>(lifted, labels))
        metrics.push_back(node.get_metric());
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    EXPECT_TRUE(ground);
    if (ground)
    {
        auto ground_metrics = std::vector<double>();
        for (const auto& node : run<GroundTag>(ground, labels))
            ground_metrics.push_back(node.get_metric());
        EXPECT_EQ(ground_metrics, metrics);
    }
    return metrics;
}

/// A duration interval: the action may end as soon as its lower bound allows.
TEST(TyrTests, TemporalTransitionsBoundDurations)
{
    const auto domain = std::string(R"(
(define (domain charge) (:requirements :strips :durative-actions :duration-inequalities)
  (:predicates (empty) (full))
  (:durative-action charge :parameters () :duration (and (>= ?duration 1) (<= ?duration 3))
    :condition (at start (empty)) :effect (and (at start (not (empty))) (at end (full)))))
)");
    const auto problem = std::string("(define (problem charge-1) (:domain charge) (:init (empty)) (:goal (full)) (:metric minimize (total-time)))");
    const auto metrics = run_both(domain, problem, { { START, "charge" }, { END, "charge" } });
    ASSERT_EQ(metrics.size(), 3);
    EXPECT_DOUBLE_EQ(metrics[2], 1);
}

const auto* const window_domain = R"(
(define (domain window) (:requirements :strips :durative-actions :timed-initial-literals)
  (:predicates (open) (done))
  (:durative-action go :parameters () :duration (= ?duration 2)
    :condition (over all (open)) :effect (at end (done)))
  (:durative-action crawl :parameters () :duration (= ?duration 6)
    :condition (over all (open)) :effect (at end (done))))
)";

const auto* const window_problem = R"(
(define (problem window-1) (:domain window) (:init (at 5 (open)) (at 10 (not (open)))) (:goal (done)) (:metric minimize (total-time)))
)";

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
    const auto domain = std::string(R"(
(define (domain kitchen) (:requirements :strips :durative-actions)
  (:predicates (light) (cooked) (have-match))
  (:durative-action light-match :parameters () :duration (= ?duration 5)
    :condition (at start (have-match)) :effect (and (at start (light)) (at end (not (light)))))
  (:durative-action cook :parameters () :duration (= ?duration 2)
    :condition (over all (light)) :effect (at end (cooked))))
)");
    const auto problem = std::string("(define (problem kitchen-1) (:domain kitchen) (:init (have-match)) (:goal (cooked)) (:metric minimize (total-time)))");
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
