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

#include <array>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tyr/planning/algorithms/iw/novelty_table.hpp>
#include <tyr/planning/algorithms/iw/pruning_strategy.hpp>
#include <vector>

namespace tyr::tests
{

TEST(TyrTests, TyrPlanningIwRankTupleUsesStableCombinadicRanks)
{
    EXPECT_EQ(planning::iw::rank_tuple<3>(std::span<const ygg::uint_t> {}), 0);

    const auto atom_0 = std::array<ygg::uint_t, 1> { 0 };
    const auto atom_3 = std::array<ygg::uint_t, 1> { 3 };
    const auto tuple_0_1 = std::array<ygg::uint_t, 2> { 0, 1 };
    const auto tuple_0_2 = std::array<ygg::uint_t, 2> { 0, 2 };
    const auto tuple_1_2 = std::array<ygg::uint_t, 2> { 1, 2 };
    const auto tuple_0_3 = std::array<ygg::uint_t, 2> { 0, 3 };
    const auto tuple_1_3 = std::array<ygg::uint_t, 2> { 1, 3 };
    const auto tuple_2_3 = std::array<ygg::uint_t, 2> { 2, 3 };
    const auto tuple_0_1_2 = std::array<ygg::uint_t, 3> { 0, 1, 2 };
    const auto tuple_0_1_4 = std::array<ygg::uint_t, 3> { 0, 1, 4 };

    EXPECT_EQ(planning::iw::rank_tuple<3>(atom_0), 0);
    EXPECT_EQ(planning::iw::rank_tuple<3>(atom_3), 3);

    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_0_1), 0);
    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_0_2), 1);
    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_1_2), 2);
    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_0_3), 3);
    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_1_3), 4);
    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_2_3), 5);

    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_0_1_2), 0);
    EXPECT_EQ(planning::iw::rank_tuple<3>(tuple_0_1_4), 4);
}

TEST(TyrTests, TyrPlanningIwDynamicNoveltyTableGrowsWithoutRemapping)
{
    auto table = planning::iw::DynamicNoveltyTable<3>();

    const auto atom_0 = std::array<ygg::uint_t, 1> { 0 };
    const auto atom_100 = std::array<ygg::uint_t, 1> { 100 };
    const auto tuple_0_1 = std::array<ygg::uint_t, 2> { 0, 1 };
    const auto tuple_1_2 = std::array<ygg::uint_t, 2> { 1, 2 };
    const auto tuple_0_1_2 = std::array<ygg::uint_t, 3> { 0, 1, 2 };
    const auto tuple_10_20_30 = std::array<ygg::uint_t, 3> { 10, 20, 30 };

    EXPECT_EQ(table.get_num_bits(1), 0);
    EXPECT_EQ(table.get_num_bits(2), 0);
    EXPECT_EQ(table.get_num_bits(3), 0);

    EXPECT_TRUE(table.insert(atom_0));
    EXPECT_EQ(table.get_num_bits(1), planning::iw::rank_tuple<3>(atom_0) + 1);
    EXPECT_FALSE(table.insert(atom_0));
    EXPECT_TRUE(table.test(atom_0));

    EXPECT_TRUE(table.insert(tuple_0_1));
    EXPECT_EQ(table.get_num_bits(2), planning::iw::rank_tuple<3>(tuple_0_1) + 1);
    EXPECT_FALSE(table.insert(tuple_0_1));

    EXPECT_TRUE(table.insert(tuple_1_2));
    EXPECT_EQ(table.get_num_bits(2), planning::iw::rank_tuple<3>(tuple_1_2) + 1);
    EXPECT_FALSE(table.insert(tuple_1_2));

    EXPECT_TRUE(table.insert(tuple_0_1_2));
    EXPECT_EQ(table.get_num_bits(3), planning::iw::rank_tuple<3>(tuple_0_1_2) + 1);
    EXPECT_FALSE(table.insert(tuple_0_1_2));

    EXPECT_TRUE(table.insert(atom_100));
    EXPECT_EQ(table.get_num_bits(1), planning::iw::rank_tuple<3>(atom_100) + 1);
    EXPECT_FALSE(table.insert(atom_0));
    EXPECT_FALSE(table.insert(tuple_0_1));
    EXPECT_FALSE(table.insert(tuple_1_2));
    EXPECT_FALSE(table.insert(tuple_0_1_2));

    EXPECT_TRUE(table.insert(tuple_10_20_30));
    EXPECT_EQ(table.get_num_bits(3), planning::iw::rank_tuple<3>(tuple_10_20_30) + 1);
    EXPECT_FALSE(table.insert(tuple_10_20_30));
    EXPECT_FALSE(table.insert(tuple_0_1_2));
}

TEST(TyrTests, TyrPlanningIwDynamicNoveltyTableSeparatesArityBuckets)
{
    auto table = planning::iw::DynamicNoveltyTable<2>();

    const auto atom_1 = std::array<ygg::uint_t, 1> { 1 };
    const auto tuple_0_2 = std::array<ygg::uint_t, 2> { 0, 2 };

    ASSERT_EQ(planning::iw::rank_tuple<2>(atom_1), planning::iw::rank_tuple<2>(tuple_0_2));

    EXPECT_TRUE(table.insert(atom_1));
    EXPECT_TRUE(table.insert(tuple_0_2));
    EXPECT_FALSE(table.insert(atom_1));
    EXPECT_FALSE(table.insert(tuple_0_2));
}

TEST(TyrTests, TyrPlanningIwDynamicNoveltyTableReportsArity)
{
    auto table = planning::iw::DynamicNoveltyTable<2>();
    EXPECT_EQ(table.get_arity(), 2);
}

TEST(TyrTests, TyrPlanningIwForEachTupleGeneratesCanonicalTuplesUpToArity)
{
    const auto atoms = planning::iw::AtomIndexList { 1, 3, 5 };
    auto tuple = std::array<ygg::uint_t, 2> {};
    auto generated = std::vector<planning::iw::AtomIndexList> {};

    planning::iw::for_each_tuple<2>(atoms, tuple, [&](std::span<const ygg::uint_t> item) { generated.emplace_back(item.begin(), item.end()); });

    const auto expected = std::vector<planning::iw::AtomIndexList> {
        { 1 }, { 3 }, { 5 }, { 1, 3 }, { 1, 5 }, { 3, 5 },
    };

    EXPECT_EQ(generated, expected);
}

TEST(TyrTests, TyrPlanningIwForEachTupleRespectsRuntimeArity)
{
    const auto atoms = planning::iw::AtomIndexList { 1, 3, 5 };
    auto tuple = std::array<ygg::uint_t, 3> {};
    auto generated = std::vector<planning::iw::AtomIndexList> {};

    planning::iw::for_each_tuple<3>(atoms, 2, tuple, [&](std::span<const ygg::uint_t> item) { generated.emplace_back(item.begin(), item.end()); });

    const auto expected = std::vector<planning::iw::AtomIndexList> {
        { 1 }, { 3 }, { 5 }, { 1, 3 }, { 1, 5 }, { 3, 5 },
    };

    EXPECT_EQ(generated, expected);
}

TEST(TyrTests, TyrPlanningIwForEachTupleWithAddedAtomsGeneratesOnlyTuplesContainingAddedAtoms)
{
    const auto added = planning::iw::AtomIndexList { 2, 5 };
    const auto kept = planning::iw::AtomIndexList { 1, 4 };
    auto added_tuple = std::array<ygg::uint_t, 2> {};
    auto kept_tuple = std::array<ygg::uint_t, 2> {};
    auto tuple = std::array<ygg::uint_t, 2> {};
    auto generated = std::vector<planning::iw::AtomIndexList> {};

    planning::iw::for_each_tuple_with_added_atoms<2>(added,
                                                     kept,
                                                     added_tuple,
                                                     kept_tuple,
                                                     tuple,
                                                     [&](std::span<const ygg::uint_t> item) { generated.emplace_back(item.begin(), item.end()); });

    const auto expected = std::vector<planning::iw::AtomIndexList> {
        { 2 }, { 5 }, { 1, 2 }, { 2, 4 }, { 1, 5 }, { 4, 5 }, { 2, 5 },
    };

    EXPECT_EQ(generated, expected);
}

TEST(TyrTests, TyrPlanningIwForEachTupleWithAddedAtomsRespectsRuntimeArity)
{
    const auto added = planning::iw::AtomIndexList { 2, 5 };
    const auto kept = planning::iw::AtomIndexList { 1, 4 };
    auto added_tuple = std::array<ygg::uint_t, 3> {};
    auto kept_tuple = std::array<ygg::uint_t, 3> {};
    auto tuple = std::array<ygg::uint_t, 3> {};
    auto generated = std::vector<planning::iw::AtomIndexList> {};

    planning::iw::for_each_tuple_with_added_atoms<3>(added,
                                                     kept,
                                                     2,
                                                     added_tuple,
                                                     kept_tuple,
                                                     tuple,
                                                     [&](std::span<const ygg::uint_t> item) { generated.emplace_back(item.begin(), item.end()); });

    const auto expected = std::vector<planning::iw::AtomIndexList> {
        { 2 }, { 5 }, { 1, 2 }, { 2, 4 }, { 1, 5 }, { 4, 5 }, { 2, 5 },
    };

    EXPECT_EQ(generated, expected);
}

TEST(TyrTests, TyrPlanningIwNoveltyPruningStrategyChecksRuntimeArity)
{
    auto pruning_strategy = planning::iw::NoveltyPruningStrategy<GroundTag>(2);
    EXPECT_EQ(pruning_strategy.get_max_arity(), 2);

    EXPECT_THROW((planning::iw::NoveltyPruningStrategy<GroundTag>(planning::iw::MaxArity + 1)), std::invalid_argument);
}

}
