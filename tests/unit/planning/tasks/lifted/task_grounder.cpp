#include "planning/parser.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <cstddef>
#include <filesystem>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests::planning_task_grounder
{
namespace
{
std::filesystem::path benchmark(const std::string& family, const std::string& domain)
{ return std::filesystem::path(BENCHMARKS_DIR) / family / "tests" / domain; }

/// The number of choices of a ground task's actions, and whether each has as many probabilities as outcomes (or none).
std::size_t ground_choices(const std::string& family, const std::string& name, bool probabilistic)
{
    const auto domain = benchmark(family, name);
    const auto execution_context = ygg::ExecutionContext::create(1);
    auto parser = tyr::formalism::Parser(domain / "domain.pddl");
    const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(parser.parse_task(domain / "test-1.pddl"));
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    EXPECT_TRUE(ground);
    if (!ground)
        return 0;

    auto count = std::size_t { 0 };
    for (const auto action : ground->get_task().get_ground_actions())
        for (const auto choice : action.get_choices())
        {
            ++count;
            EXPECT_EQ(choice.get_probabilities().size(), probabilistic ? choice.get_outcomes().size() : 0);
        }
    return count;
}

/// Grounding keeps every choice of nature: a oneof's outcomes, a probabilistic effect's with their probabilities.
TEST(TyrTests, TaskGrounderGroundsOneOfChoices) { EXPECT_GT(ground_choices("nondeterministic", "ipc2008-faults", false), 0); }

TEST(TyrTests, TaskGrounderGroundsProbabilisticChoices) { EXPECT_GT(ground_choices("probabilistic", "ippc2004-bw-nc-pc-nr", true), 0); }

/// A task whose choices nest both ways: a probabilistic effect in a oneof outcome, and a oneof in a probabilistic outcome
/// under a forall and when (loki's normal form flattens only nests of one kind).
const auto mixed_domain = task_fixture("mixed/domain.pddl");

const auto mixed_problem = task_fixture("mixed/mixed-1.pddl");

/// Nested choices are translated scoped into the choices that hold them and grounded per binding of their variables.
TEST(TyrTests, TaskGrounderGroundsNestedChoices)
{
    auto parser = tyr::formalism::Parser(mixed_domain);
    const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(parser.parse_task(mixed_problem));
    const auto action = lifted->get_domain().get_domain().get_actions()[0];
    ASSERT_EQ(action.get_choices().size(), 1);
    const auto outcomes = action.get_choices()[0].get_outcomes();
    ASSERT_EQ(outcomes.size(), 2);
    EXPECT_EQ(outcomes[0].get_choices().size(), 1);
    EXPECT_EQ(outcomes[1].get_choices().size(), 1);

    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    const auto ground_actions = ground->get_task().get_ground_actions();
    ASSERT_EQ(ground_actions.size(), 2);
    for (const auto ground_action : ground_actions)
    {
        ASSERT_EQ(ground_action.get_choices().size(), 1);
        const auto ground_outcomes = ground_action.get_choices()[0].get_outcomes();
        ASSERT_EQ(ground_outcomes.size(), 2);
        // The first outcome: (q ?x) and a probabilistic effect of two outcomes.
        EXPECT_EQ(ground_outcomes[0].get_effects().size(), 1);
        ASSERT_EQ(ground_outcomes[0].get_choices().size(), 1);
        EXPECT_EQ(ground_outcomes[0].get_choices()[0].get_probabilities().size(), 2);
        // The second outcome: one probabilistic effect per binding of ?y, its nonempty outcome a oneof.
        ASSERT_EQ(ground_outcomes[1].get_choices().size(), 2);
        for (const auto per_block : ground_outcomes[1].get_choices())
        {
            ASSERT_EQ(per_block.get_outcomes().size(), 2);
            EXPECT_EQ(per_block.get_probabilities().size(), 2);
            auto nested_oneofs = std::size_t { 0 };
            for (const auto outcome : per_block.get_outcomes())
                for (const auto nested : outcome.get_choices())
                {
                    ++nested_oneofs;
                    EXPECT_TRUE(nested.get_probabilities().empty());
                    EXPECT_EQ(nested.get_outcomes().size(), 2);
                }
            EXPECT_EQ(nested_oneofs, 1);
        }
    }
}
}  // namespace
}  // namespace tyr::tests::planning_task_grounder
