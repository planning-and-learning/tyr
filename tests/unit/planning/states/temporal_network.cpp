#include "tyr/planning/states/storages/temporal_network.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>

namespace tyr::tests::planning_temporal_network
{
namespace
{
using tyr::planning::TemporalNetwork;

/// Durations, release times and orderings propagate to the earliest times; latest times stay unbounded without deadlines.
TEST(TyrTests, TemporalNetworkPropagatesEarliestTimes)
{
    auto network = TemporalNetwork();
    const auto start = network.add_timepoint();
    const auto end = network.add_timepoint();
    ASSERT_TRUE(network.constrain_between(start, end, 2, 5));  // a duration in [2, 5]
    ASSERT_TRUE(network.constrain(start, 0, -1));              // the start at time 1 or later
    EXPECT_DOUBLE_EQ(network.earliest(start), 1);
    EXPECT_DOUBLE_EQ(network.earliest(end), 3);
    EXPECT_EQ(network.latest(end), TemporalNetwork::infinity);

    // A happening after the end, separated by epsilon.
    const auto next = network.add_timepoint();
    ASSERT_TRUE(network.constrain(next, end, -TemporalNetwork::epsilon));
    EXPECT_NEAR(network.earliest(next), 3 + TemporalNetwork::epsilon, 1e-12);
}

/// A constraint that contradicts the network is rejected and leaves it unchanged.
TEST(TyrTests, TemporalNetworkRejectsInconsistency)
{
    auto network = TemporalNetwork();
    const auto start = network.add_timepoint();
    const auto end = network.add_timepoint();
    ASSERT_TRUE(network.constrain_between(start, end, 2, 5));
    const auto before = std::array { network.distance(start, end), network.distance(end, start) };
    EXPECT_FALSE(network.constrain(start, end, 1));  // shorter than its minimum duration
    EXPECT_DOUBLE_EQ(network.distance(start, end), before[0]);
    EXPECT_DOUBLE_EQ(network.distance(end, start), before[1]);
    // A deadline before the earliest end.
    ASSERT_TRUE(network.constrain(start, 0, -1));
    EXPECT_FALSE(network.constrain(0, end, 2));
}

/// Projecting out a timepoint keeps what the network implies about the others.
TEST(TyrTests, TemporalNetworkProjectsExactly)
{
    auto network = TemporalNetwork();
    const auto a = network.add_timepoint();
    const auto b = network.add_timepoint();
    const auto c = network.add_timepoint();
    ASSERT_TRUE(network.constrain_between(a, b, 1, 2));
    ASSERT_TRUE(network.constrain_between(b, c, 3, 4));
    const auto kept = std::array<std::size_t, 3> { 0, a, c };
    network.project(kept);
    ASSERT_EQ(network.size(), 3);
    // c - a in [4, 6], through the projected-out b.
    EXPECT_DOUBLE_EQ(network.distance(1, 2), 6);
    EXPECT_DOUBLE_EQ(network.distance(2, 1), -4);
}
}  // namespace
}  // namespace tyr::tests::planning_temporal_network
