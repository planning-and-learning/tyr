#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_repository.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests::planning_outcome_successors
{
namespace
{
std::filesystem::path benchmark(const std::string& family, const std::string& domain)
{ return std::filesystem::path(BENCHMARKS_DIR) / family / "tests" / domain; }

/// A successor in a form to compare ground and lifted ones by: its label (action name and objects) and, per
/// distribution, its number of outcomes and the sum of their probabilities.
struct Summary
{
    std::string label;
    std::vector<std::pair<std::size_t, double>> distributions;

    auto operator<=>(const Summary&) const = default;
};

/// The summaries of the successors of the task's initial state.
template<TaskKind Kind>
std::vector<Summary> initial_successors(const planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = std::make_shared<planning::StateRepository<Kind>>(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    const auto initial_node = generator.get_initial_node(*state_repository, *axiom_evaluator);

    auto result = std::vector<Summary> {};
    const auto summarize = [&](const auto& successor)
    {
        auto summary = Summary { "advance", {} };
        if (const auto binding = successor.get_label().get_binding())
        {
            summary.label = binding->get_relation().get_name().str();
            for (const auto object : binding->template resolve<formalism::Object>())
                summary.label += " " + object.get_name().str();
        }
        for (const auto distribution : successor.get_distributions())
        {
            auto sum = 0.0;
            for (const auto outcome : distribution.get_outcomes())
                sum += outcome.get_probability();
            summary.distributions.emplace_back(distribution.get_outcomes().size(), sum);
        }
        std::ranges::sort(summary.distributions);
        result.push_back(std::move(summary));
        return true;
    };
    generator.for_each_successor(initial_node, *state_repository, *axiom_evaluator, summarize);
    std::ranges::sort(result);

    // Into the generator's pooled builders, not registered: the same successors.
    auto registered = std::move(result);
    result.clear();
    generator.for_each_successor(initial_node, *axiom_evaluator, summarize);
    std::ranges::sort(result);
    EXPECT_EQ(result, registered);
    return registered;
}

/// The successors of the initial state of a task, lifted and ground (which must agree). Lifted successor generation keeps
/// to action bindings: it grounds no action.
std::vector<Summary> both_successors(formalism::PlanningTask<LiftedTag> formalism_task)
{
    const auto lifted = planning::Task<LiftedTag>::create(std::move(formalism_task));
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    EXPECT_TRUE(ground);
    const auto ground_actions_before = lifted->get_repository()->template size<formalism::Action<GroundTag>>();
    const auto result = initial_successors<LiftedTag>(lifted);
    EXPECT_EQ(lifted->get_repository()->template size<formalism::Action<GroundTag>>(), ground_actions_before);
    if (ground)
        EXPECT_EQ(initial_successors<GroundTag>(ground), result);
    return result;
}

std::vector<Summary> benchmark_successors(const std::string& family, const std::string& name)
{
    const auto domain = benchmark(family, name);
    auto parser = formalism::Parser(domain / "domain.pddl");
    return both_successors(parser.parse_task(domain / "test-1.pddl"));
}

void expect_distributions_sum_to_one(const std::vector<Summary>& successors)
{
    for (const auto& successor : successors)
        for (const auto& [size, sum] : successor.distributions)
        {
            EXPECT_GT(size, 0) << successor.label;
            EXPECT_NEAR(sum, 1.0, 1e-9) << successor.label;
        }
}

/// A oneof yields one distribution per outcome (each of one outcome); a deterministic action one of one.
TEST(TyrTests, OutcomeSuccessorsOfOneOfEffects)
{
    const auto successors = benchmark_successors("nondeterministic", "ipc2008-faults");
    ASSERT_FALSE(successors.empty());
    expect_distributions_sum_to_one(successors);
    auto oneofs = std::size_t { 0 };
    for (const auto& successor : successors)
    {
        for (const auto& [size, sum] : successor.distributions)
            EXPECT_EQ(size, 1) << successor.label;
        oneofs += successor.distributions.size() > 1 ? 1 : 0;
    }
    EXPECT_GT(oneofs, 0);
}

/// A probabilistic effect yields one distribution over its outcomes.
TEST(TyrTests, OutcomeSuccessorsOfProbabilisticEffects)
{
    const auto successors = benchmark_successors("probabilistic", "ippc2004-bw-nc-pc-nr");
    ASSERT_FALSE(successors.empty());
    expect_distributions_sum_to_one(successors);
    auto chances = std::size_t { 0 };
    for (const auto& successor : successors)
    {
        EXPECT_EQ(successor.distributions.size(), 1) << successor.label;
        chances += successor.distributions.front().first > 1 ? 1 : 0;
    }
    EXPECT_GT(chances, 0);
}

const auto* const mixed_domain = R"(
(define (domain mixed)
  (:requirements :strips :typing :non-deterministic :probabilistic-effects :conditional-effects)
  (:types block)
  (:predicates (p ?b - block) (q ?b - block) (r))
  (:action a
    :parameters (?x - block)
    :precondition (p ?x)
    :effect (oneof (and (q ?x) (probabilistic 0.3 (r) 0.7 (not (p ?x))))
                   (forall (?y - block) (when (p ?y) (probabilistic 0.5 (oneof (q ?y) (r))))))))
)";

const auto* const mixed_problem = R"(
(define (problem mixed-1) (:domain mixed) (:objects b1 b2 - block) (:init (p b1) (p b2)) (:goal (r)))
)";

/// Nested choices of both kinds flatten into a set of distributions: the first outcome is one distribution (0.3 / 0.7);
/// the second has, per block, a probabilistic effect whose nonempty outcome is a oneof (two strategies each), the blocks
/// independent: 2 x 2 distributions of 4 joint outcomes each, some of them merged (equal states).
TEST(TyrTests, OutcomeSuccessorsOfNestedChoices)
{
    auto parser = formalism::Parser(std::string(mixed_domain), std::nullopt);
    const auto successors = both_successors(parser.parse_task(std::string(mixed_problem), std::nullopt));
    ASSERT_EQ(successors.size(), 2);
    expect_distributions_sum_to_one(successors);
    for (const auto& successor : successors)
    {
        EXPECT_EQ(successor.distributions.size(), 5) << successor.label;
        // The first outcome's distribution: two outcomes.
        EXPECT_TRUE(std::ranges::any_of(successor.distributions, [](const auto& distribution) { return distribution.first == 2; }));
    }
}

/// The deterministic API rejects an action with outcomes instead of applying it without them.
TEST(TyrTests, DeterministicSuccessorsRejectOutcomes)
{
    const auto domain = benchmark("nondeterministic", "ipc2008-faults");
    auto parser = formalism::Parser(domain / "domain.pddl");
    const auto task = planning::Task<LiftedTag>::create(parser.parse_task(domain / "test-1.pddl"));
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<planning::AxiomEvaluator<LiftedTag>>(task, execution_context);
    auto state_repository = std::make_shared<planning::StateRepository<LiftedTag>>(task);
    auto generator = planning::SuccessorGenerator<LiftedTag>(task, execution_context);
    const auto initial_node = generator.get_initial_node(*state_repository, *axiom_evaluator);
    EXPECT_THROW(static_cast<void>(generator.get_successor_nodes(initial_node, *state_repository, *axiom_evaluator)), std::invalid_argument);
}
}  // namespace
}  // namespace tyr::tests::planning_outcome_successors
