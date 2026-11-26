#include "tyr/formalism/planning/parser.hpp"

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/temporal_part_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <cstddef>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace tyr::tests::planning_parser
{
namespace
{
std::filesystem::path benchmark(const std::string& family, const std::string& domain)
{ return std::filesystem::path(BENCHMARKS_DIR) / family / "tests" / domain; }

/// A task of the classical tests parses.
TEST(TyrTests, ParserTranslatesAClassicalTask)
{
    const auto domain = benchmark("classical", "gripper");
    auto parser = tyr::formalism::Parser(domain / "domain.pddl");
    EXPECT_NO_THROW(static_cast<void>(parser.parse_task(domain / "test-1.pddl")));
}

/// A durative action becomes an action with a temporal part: its condition and effects are those at its start.
TEST(TyrTests, ParserTranslatesDurativeActions)
{
    const auto domain = benchmark("temporal", "ipc2002-depots-time-simple-automatic");
    auto parser = tyr::formalism::Parser(domain / "domain.pddl");
    EXPECT_NO_THROW(static_cast<void>(parser.parse_task(domain / "test-1.pddl")));
    const auto actions = parser.get_domain().get_domain().get_actions();
    ASSERT_FALSE(actions.empty());
    auto ending = 0;
    for (const auto action : actions)
    {
        const auto temporal_part = action.get_temporal_part();
        if (!temporal_part.has_value())
        {
            ADD_FAILURE() << "an instantaneous action in a domain of durative actions";
            continue;
        }
        ending += temporal_part->get_end_effects().empty() ? 0 : 1;
    }
    // Drive, drop and load have effects at their end (lift and unload only at their start).
    EXPECT_EQ(ending, 3);
}

/// What tyr does not support yet is rejected, not dropped: ?duration in an effect (it would have to be the duration
/// fixed at the start).
TEST(TyrTests, ParserRejectsDurationInEffects)
{ EXPECT_THROW(tyr::formalism::Parser(benchmark("temporal", "ipc2006-rovers-metric-time") / "domain.pddl"), std::invalid_argument); }

/// The choices of a domain's actions: their number, and the sum of the probabilities of each (0 for a oneof).
struct Choices
{
    std::size_t count = 0;
    std::vector<double> probability_sums;
};

Choices choices_of(const tyr::formalism::Parser& parser)
{
    auto result = Choices();
    for (const auto action : parser.get_domain().get_domain().get_actions())
        for (const auto choice : action.get_choices())
        {
            ++result.count;
            auto sum = 0.0;
            for (const auto probability : choice.get_probabilities())
                sum += probability;
            result.probability_sums.push_back(sum);
        }
    return result;
}

/// A oneof effect becomes a choice without probabilities.
TEST(TyrTests, ParserTranslatesOneOfEffectsIntoChoices)
{
    const auto domain = benchmark("nondeterministic", "ipc2008-faults");
    auto parser = tyr::formalism::Parser(domain / "domain.pddl");
    EXPECT_NO_THROW(static_cast<void>(parser.parse_task(domain / "test-1.pddl")));
    const auto choices = choices_of(parser);
    EXPECT_GT(choices.count, 0);
    for (const auto sum : choices.probability_sums)
        EXPECT_EQ(sum, 0.0);
}

/// A probabilistic effect becomes a choice whose probabilities add up to 1 (loki's normal form adds the empty outcome).
TEST(TyrTests, ParserTranslatesProbabilisticEffectsIntoChoices)
{
    const auto domain = benchmark("probabilistic", "ippc2004-bw-nc-pc-nr");
    auto parser = tyr::formalism::Parser(domain / "domain.pddl");
    EXPECT_NO_THROW(static_cast<void>(parser.parse_task(domain / "test-1.pddl")));
    const auto choices = choices_of(parser);
    EXPECT_GT(choices.count, 0);
    for (const auto sum : choices.probability_sums)
        EXPECT_NEAR(sum, 1.0, 1e-9);
}
}  // namespace
}  // namespace tyr::tests::planning_parser
