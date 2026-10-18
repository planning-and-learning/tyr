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
#include "planning/parser.hpp"
#include "tyr/formalism/formalism.hpp"
#include "tyr/planning/planning.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <string>

namespace tyr::tests
{
namespace
{
using formalism::planning::AtomViewList;
using formalism::planning::invariant::compute_all_mutex_groups;
using formalism::planning::invariant::compute_mutex_groups_in_order;
using formalism::planning::invariant::synthesize_invariants;

const auto ferry_directory = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry";
}

TEST(TyrFormalismPlanningInvariantInterningTest, SynthesisIsIdempotent)
{
    const auto domain = make_test_parser(ferry_directory / "domain.pddl").get_domain();

    const auto first = synthesize_invariants(domain.get_domain(), *domain.get_repository());
    const auto second = synthesize_invariants(domain.get_domain(), *domain.get_repository());

    ASSERT_FALSE(first.empty());
    ASSERT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
        EXPECT_EQ(first[i].get_index(), second[i].get_index());
    EXPECT_TRUE(formalism::planning::to_string(first.front()).starts_with("Invariant"));
}

TEST(TyrFormalismPlanningInvariantInterningTest, MutexGroupsNameTheirInvariantAndObjects)
{
    auto parser = make_test_parser(ferry_directory / "domain.pddl");
    const auto domain = parser.get_domain();
    const auto invariants = synthesize_invariants(domain.get_domain(), *domain.get_repository());
    auto lifted = planning::Task<LiftedTag>::create(parser.parse_task(ferry_directory / "test-1.pddl"));
    auto context = ygg::ExecutionContext::create(1);
    const auto result = lifted->instantiate_ground_task(*context, planning::GroundTaskInstantiationOptions {});
    ASSERT_NE(result.task, nullptr);

    const auto ground = result.task->get_task();
    auto all_atoms = AtomViewList<GroundTag, formalism::FluentTag> {};
    for (const auto atom : ground.get_atoms<formalism::FluentTag>())
        all_atoms.push_back(atom);
    auto initial_atoms = AtomViewList<GroundTag, formalism::FluentTag> {};
    for (const auto fact : ground.get_fluent_facts())
        if (const auto atom = fact.get_atom())
            initial_atoms.push_back(*atom);

    const auto groups = compute_all_mutex_groups(initial_atoms, all_atoms, invariants);
    ASSERT_FALSE(groups.empty());
    for (const auto& group : groups)
    {
        ASSERT_TRUE(group.invariant.has_value());
        EXPECT_NE(std::ranges::find(invariants, *group.invariant), invariants.end());
        EXPECT_EQ(group.objects.size(), group.invariant->get_num_rigid_variables());
        EXPECT_GE(group.atoms.size(), 2U);
        // Every atom mentions the objects bound to the rigid variables.
        for (const auto atom : group.atoms)
        {
            const auto objects = atom.get_objects();
            for (const auto object : group.objects)
                EXPECT_NE(std::ranges::find(objects, object), objects.end());
        }
    }

    // Atoms of no listed invariant are single and name no invariant.
    for (const auto& group : compute_mutex_groups_in_order(initial_atoms, all_atoms, {}))
    {
        EXPECT_FALSE(group.invariant.has_value());
        EXPECT_TRUE(group.objects.empty());
        EXPECT_EQ(group.atoms.size(), 1U);
    }
}

}
