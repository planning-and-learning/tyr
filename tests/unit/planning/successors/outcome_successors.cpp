#include "planning/parser.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/successor_view.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <urd/elements/view.hpp>
#include <utility>
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
    auto state_repository = planning::StateRepository<Kind>::create(task);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    const auto initial_node = planning::get_initial_node(*state_repository, *axiom_evaluator);

    auto result = std::vector<Summary> {};
    const auto summarize = [&](const auto& successor)
    {
        auto summary = Summary { "advance", {} };
        if (const auto binding = successor.label().binding())
            urd::visit_view(*binding,
                            [&](const auto& row)
                            {
                                // Its relation in whichever form the row holds it; its objects through the row's repository.
                                urd::visit_view(row.relation(),
                                                [&](const auto& relation)
                                                {
                                                    summary.label = relation.name().str();
                                                    for (std::size_t column = 0; column < relation.arity(); ++column)
                                                        summary.label +=
                                                            " "
                                                            + urd::resolve(row.template get<urd::index<formalism::Object>>(column), row.context()).name().str();
                                                });
                            });
        for (const auto distribution : successor.distributions())
        {
            auto sum = 0.0;
            for (const auto outcome : distribution.outcomes())
                sum += outcome.probability();
            summary.distributions.emplace_back(distribution.outcomes().size(), sum);
        }
        std::ranges::sort(summary.distributions);
        result.push_back(std::move(summary));
        return true;
    };
    planning::for_each_successor<planning::GeneralTag>(
        generator,
        initial_node,
        [&](const planning::TransitionLabel<Kind>& /*transition*/, planning::SuccessorBuilder<planning::GeneralTag, Kind>& successor)
        {
            // After the three steps (its states closed and registered), the same successor.
            const auto built = summarize(urd::resolve(std::as_const(successor), std::as_const(*state_repository)));
            planning::find_outcomes(*state_repository, successor);
            EXPECT_TRUE(planning::extend_outcomes(generator, *state_repository, *axiom_evaluator, successor));
            planning::register_outcomes(*state_repository, successor);
            summarize(urd::resolve(std::as_const(successor), std::as_const(*state_repository)));
            EXPECT_EQ(result[result.size() - 2], result.back());
            result.pop_back();
            return built;
        });
    std::ranges::sort(result);
    return result;
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

constexpr const char* mixed_domain = "mixed/domain.pddl";  // under task_fixture

constexpr const char* mixed_problem = "mixed/mixed-1.pddl";  // under task_fixture

/// Nested choices of both kinds flatten into a set of distributions: the first outcome is one distribution (0.3 / 0.7);
/// the second has, per block, a probabilistic effect whose nonempty outcome is a oneof (two strategies each), the blocks
/// independent: 2 x 2 distributions of 4 joint outcomes each, some of them merged (equal states).
TEST(TyrTests, OutcomeSuccessorsOfNestedChoices)
{
    auto parser = formalism::Parser(task_fixture(mixed_domain));
    const auto successors = both_successors(parser.parse_task(task_fixture(mixed_problem)));
    ASSERT_EQ(successors.size(), 2);
    expect_distributions_sum_to_one(successors);
    for (const auto& successor : successors)
    {
        EXPECT_EQ(successor.distributions.size(), 5) << successor.label;
        // The first outcome's distribution: two outcomes.
        EXPECT_TRUE(std::ranges::any_of(successor.distributions, [](const auto& distribution) { return distribution.first == 2; }));
    }
}

/// What a task's effects ask of an environment, as its generator computes them.
template<TaskKind Kind>
planning::TaskFeatures features_of(const planning::TaskPtr<Kind>& task)
{ return planning::SuccessorGenerator<Kind>(task, ygg::ExecutionContext::create(1)).features(); }

/// Whether environment E gives the task its successors (require_environment does not throw).
template<planning::Environment E>
bool fits(const planning::TaskPtr<LiftedTag>& task)
{
    const auto generator = planning::SuccessorGenerator<LiftedTag>(task, ygg::ExecutionContext::create(1));
    try
    {
        planning::require_environment<E>(generator);
        return true;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
}

planning::TaskPtr<LiftedTag> benchmark_task(const std::string& family, const std::string& name)
{
    const auto domain = benchmark(family, name);
    auto parser = formalism::Parser(domain / "domain.pddl");
    return planning::Task<LiftedTag>::create(parser.parse_task(domain / "test-1.pddl"));
}

/// An environment gives a task its successors where nature does no more than the environment models: a oneof needs an
/// adversary, a probabilistic effect chance, both together an adversary and chance.
TEST(TyrTests, EnvironmentsFollowATasksEffects)
{
    const auto oneof = benchmark_task("nondeterministic", "ipc2008-faults");
    EXPECT_TRUE(features_of(oneof).oneof);
    EXPECT_FALSE(features_of(oneof).probabilistic);
    EXPECT_FALSE(fits<planning::DeterministicTag>(oneof));
    EXPECT_TRUE(fits<planning::NondeterministicTag>(oneof));
    EXPECT_FALSE(fits<planning::ProbabilisticTag>(oneof));
    EXPECT_TRUE(fits<planning::GeneralTag>(oneof));

    const auto probabilistic = benchmark_task("probabilistic", "ippc2004-bw-nc-pc-nr");
    EXPECT_FALSE(features_of(probabilistic).oneof);
    EXPECT_TRUE(features_of(probabilistic).probabilistic);
    EXPECT_FALSE(fits<planning::DeterministicTag>(probabilistic));
    EXPECT_FALSE(fits<planning::NondeterministicTag>(probabilistic));
    EXPECT_TRUE(fits<planning::ProbabilisticTag>(probabilistic));
    EXPECT_TRUE(fits<planning::GeneralTag>(probabilistic));

    auto parser = formalism::Parser(task_fixture(mixed_domain));
    const auto mixed = planning::Task<LiftedTag>::create(parser.parse_task(task_fixture(mixed_problem)));
    EXPECT_TRUE(features_of(mixed).oneof);
    EXPECT_TRUE(features_of(mixed).probabilistic);
    EXPECT_FALSE(fits<planning::DeterministicTag>(mixed));
    EXPECT_FALSE(fits<planning::NondeterministicTag>(mixed));
    EXPECT_FALSE(fits<planning::ProbabilisticTag>(mixed));
    EXPECT_TRUE(fits<planning::GeneralTag>(mixed));

    const auto classical = benchmark_task("classical", "gripper");
    EXPECT_FALSE(features_of(classical).oneof || features_of(classical).probabilistic || features_of(classical).temporal);
    EXPECT_TRUE(fits<planning::DeterministicTag>(classical));
    EXPECT_TRUE(fits<planning::NondeterministicTag>(classical));
    EXPECT_TRUE(fits<planning::ProbabilisticTag>(classical));
    EXPECT_TRUE(fits<planning::GeneralTag>(classical));
}

/// The planning objects of a task, and its initial node.
template<TaskKind Kind>
struct Components
{
    ygg::ExecutionContextPtr execution_context;
    std::shared_ptr<planning::AxiomEvaluator<Kind>> axiom_evaluator;
    planning::StateRepositoryPtr<Kind> state_repository;
    planning::SuccessorGenerator<Kind> generator;
    planning::OwnedNodeBuilderView<Kind> initial_node;

    explicit Components(const planning::TaskPtr<Kind>& task) :
        execution_context(ygg::ExecutionContext::create(1)),
        axiom_evaluator(std::make_shared<planning::AxiomEvaluator<Kind>>(task, execution_context)),
        state_repository(planning::StateRepository<Kind>::create(task)),
        generator(task, execution_context),
        initial_node(planning::get_initial_node(*state_repository, *axiom_evaluator))
    {
    }
};

/// The registered states of the successors of a classical task's initial node agree across the environments: a
/// deterministic successor state per transition, one outcome under an adversary or chance alone, and in the general
/// environment one distribution of one outcome. The enumeration's options prune, arrange and stop.
template<TaskKind Kind>
void expect_environments_agree(const planning::TaskPtr<Kind>& task)
{
    auto c = Components<Kind>(task);
    using StateIndex = urd::index<planning::State<Kind>>;

    auto deterministic = std::vector<StateIndex> {};
    const auto keep_state = [&](const planning::TransitionLabel<Kind>& transition, planning::SuccessorState<Kind>& successor)
    {
        EXPECT_EQ(transition.kind, planning::TransitionKind::APPLY);
        EXPECT_TRUE(transition.binding.has_value());
        planning::find_successor(*c.state_repository, c.initial_node, successor);
        planning::extend_successor(*c.axiom_evaluator, c.initial_node, successor);
        const auto node = planning::register_successor(*c.state_repository, c.initial_node, successor);
        deterministic.push_back(node.state->index);
        return true;
    };
    EXPECT_TRUE(planning::for_each_successor<planning::DeterministicTag>(c.generator, c.initial_node, keep_state));
    ASSERT_GT(deterministic.size(), 1);
    const auto all = deterministic;

    // Every step at once: labeled nodes, their states registered and their labels interned, or (without interning)
    // closed under the axioms only.
    auto kept = std::vector<StateIndex> {};
    for (const auto& labeled : planning::get_successors<planning::DeterministicTag>(c.generator, *c.state_repository, *c.axiom_evaluator, c.initial_node))
        kept.push_back(labeled.node().state().pack().index());
    EXPECT_EQ(kept, all);
    const auto built = planning::get_successors<planning::DeterministicTag>(c.generator, *c.state_repository, *c.axiom_evaluator, c.initial_node, false);
    EXPECT_EQ(built.size(), all.size());
    for (const auto& labeled : built)
        EXPECT_TRUE(labeled.node().state().pack().index().is_none());

    // accept prunes by label, before a successor is generated.
    auto options = planning::SuccessorOptions<Kind> {};
    options.accept = [](planning::ActionBindingView<Kind> /*binding*/) { return false; };
    deterministic.clear();
    EXPECT_TRUE(planning::for_each_successor<planning::DeterministicTag>(c.generator, c.initial_node, keep_state, options));
    EXPECT_TRUE(deterministic.empty());

    // arrange sees every label and leaves the positions to generate, in order: here the transitions in reverse.
    options = planning::SuccessorOptions<Kind> {};
    auto labels_seen = std::size_t { 0 };
    options.arrange = [&](std::span<const planning::TransitionLabel<Kind>> labels, std::vector<std::size_t>& order)
    {
        labels_seen = labels.size();
        std::ranges::reverse(order);
    };
    deterministic.clear();
    EXPECT_TRUE(planning::for_each_successor<planning::DeterministicTag>(c.generator, c.initial_node, keep_state, options));
    EXPECT_EQ(labels_seen, all.size());
    EXPECT_TRUE(std::ranges::equal(deterministic, all | std::views::reverse));

    // An arrangement that keeps one generates one.
    options.arrange = [](std::span<const planning::TransitionLabel<Kind>> /*labels*/, std::vector<std::size_t>& order) { order.resize(1); };
    deterministic.clear();
    EXPECT_TRUE(planning::for_each_successor<planning::DeterministicTag>(c.generator, c.initial_node, keep_state, options));
    EXPECT_EQ(deterministic, std::vector<StateIndex> { all.front() });

    // stop ends the enumeration; so does a callback that returns false.
    options = planning::SuccessorOptions<Kind> {};
    options.stop = [] { return true; };
    deterministic.clear();
    EXPECT_FALSE(planning::for_each_successor<planning::DeterministicTag>(c.generator, c.initial_node, keep_state, options));
    EXPECT_TRUE(deterministic.empty());
    auto visited = std::size_t { 0 };
    EXPECT_FALSE(planning::for_each_successor<planning::DeterministicTag>(
        c.generator,
        c.initial_node,
        [&](const planning::TransitionLabel<Kind>& /*transition*/, planning::SuccessorState<Kind>& /*successor*/)
        {
            ++visited;
            return false;
        }));
    EXPECT_EQ(visited, 1);

    // The general environment: one distribution of one outcome, registered by the axiom step.
    auto general = std::vector<StateIndex> {};
    EXPECT_TRUE(planning::for_each_successor<planning::GeneralTag>(
        c.generator,
        c.initial_node,
        [&](const planning::TransitionLabel<Kind>& /*transition*/, planning::SuccessorBuilder<planning::GeneralTag, Kind>& successor)
        {
            planning::find_outcomes(*c.state_repository, successor);
            EXPECT_TRUE(planning::extend_outcomes(c.generator, *c.state_repository, *c.axiom_evaluator, successor));
            planning::register_outcomes(*c.state_repository, successor);
            EXPECT_EQ(successor.distributions.size(), 1);
            EXPECT_EQ(successor.distributions.front()->outcomes.size(), 1);
            general.push_back(successor.distributions.front()->outcomes.front()->node.state->index);
            return true;
        }));
    EXPECT_EQ(general, all);

    // An adversary or chance alone: one outcome.
    const auto outcome_states = [&]<planning::OutcomeEnvironment E>(E /*environment*/)
    {
        auto states = std::vector<StateIndex> {};
        for (const auto& successor : planning::get_successors<E>(c.generator, *c.state_repository, *c.axiom_evaluator, c.initial_node))
        {
            EXPECT_EQ(successor.outcomes().size(), 1);
            states.push_back(successor.outcomes()[0].node().state().pack().index());
        }
        return states;
    };
    EXPECT_EQ(outcome_states(planning::NondeterministicTag {}), all);
    EXPECT_EQ(outcome_states(planning::ProbabilisticTag {}), all);
}

TEST(TyrTests, SuccessorsOfAClassicalTaskAgreeAcrossEnvironments)
{
    const auto lifted = benchmark_task("classical", "gripper");
    expect_environments_agree<LiftedTag>(lifted);
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    expect_environments_agree<GroundTag>(ground);
}

/// The number of outcomes of each successor of the task's initial node in an environment whose successors hold them, and
/// the sum of their probabilities where chance draws them (their count where an adversary picks).
template<planning::OutcomeEnvironment E, TaskKind Kind>
std::vector<std::pair<std::size_t, double>> outcome_counts(const planning::TaskPtr<Kind>& task)
{
    auto c = Components<Kind>(task);
    auto result = std::vector<std::pair<std::size_t, double>> {};
    for (const auto& successor : planning::get_successors<E>(c.generator, *c.state_repository, *c.axiom_evaluator, c.initial_node))
    {
        auto total = 0.0;
        for (const auto outcome : successor.outcomes())
        {
            EXPECT_FALSE(outcome.failed());
            if constexpr (planning::ChanceEnvironment<E>)
                total += outcome.probability();
            else
                total += 1;
        }
        result.emplace_back(successor.outcomes().size(), total);
    }
    std::ranges::sort(result);
    return result;
}

/// A oneof gives an adversary several outcomes, without probabilities, alike ground and lifted.
TEST(TyrTests, NondeterministicSuccessorsHoldAnAdversarysOutcomes)
{
    const auto lifted = benchmark_task("nondeterministic", "ipc2008-faults");
    const auto counts = outcome_counts<planning::NondeterministicTag, LiftedTag>(lifted);
    ASSERT_FALSE(counts.empty());
    EXPECT_GT(counts.back().first, 1);
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    EXPECT_EQ((outcome_counts<planning::NondeterministicTag, GroundTag>(ground)), counts);
}

/// A probabilistic effect gives chance several outcomes whose probabilities sum to 1, alike ground and lifted.
TEST(TyrTests, ProbabilisticSuccessorsHoldChancesOutcomes)
{
    const auto lifted = benchmark_task("probabilistic", "ippc2004-bw-nc-pc-nr");
    const auto counts = outcome_counts<planning::ProbabilisticTag, LiftedTag>(lifted);
    ASSERT_FALSE(counts.empty());
    EXPECT_GT(counts.back().first, 1);
    for (const auto& [size, total] : counts)
        EXPECT_NEAR(total, 1.0, 1e-9);
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    EXPECT_EQ((outcome_counts<planning::ProbabilisticTag, GroundTag>(ground)).size(), counts.size());
}

/// The deterministic environment rejects an action with outcomes instead of applying it without them, and so does an
/// environment whose shape a task's choices do not have.
TEST(TyrTests, EnvironmentsRejectOutcomesTheyDoNotModel)
{
    auto oneof = Components<LiftedTag>(benchmark_task("nondeterministic", "ipc2008-faults"));
    const auto ignore_state = [](const planning::TransitionLabel<LiftedTag>& /*transition*/, planning::SuccessorState<LiftedTag>& /*successor*/)
    { return true; };
    EXPECT_THROW(static_cast<void>(planning::for_each_successor<planning::DeterministicTag>(oneof.generator, oneof.initial_node, ignore_state)),
                 std::invalid_argument);
    const auto ignore_chance = [](const planning::TransitionLabel<LiftedTag>& /*transition*/,
                                  planning::SuccessorBuilder<planning::ProbabilisticTag, LiftedTag>& /*successor*/) { return true; };
    EXPECT_THROW(static_cast<void>(planning::for_each_successor<planning::ProbabilisticTag>(oneof.generator, oneof.initial_node, ignore_chance)),
                 std::logic_error);

    auto probabilistic = Components<LiftedTag>(benchmark_task("probabilistic", "ippc2004-bw-nc-pc-nr"));
    const auto ignore_adversary = [](const planning::TransitionLabel<LiftedTag>& /*transition*/,
                                     planning::SuccessorBuilder<planning::NondeterministicTag, LiftedTag>& /*successor*/) { return true; };
    EXPECT_THROW(
        static_cast<void>(planning::for_each_successor<planning::NondeterministicTag>(probabilistic.generator, probabilistic.initial_node, ignore_adversary)),
        std::logic_error);
}
}  // namespace
}  // namespace tyr::tests::planning_outcome_successors
