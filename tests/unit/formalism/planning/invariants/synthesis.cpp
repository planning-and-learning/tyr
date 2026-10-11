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

#include <boost/json.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::tests
{

namespace
{
std::vector<int> parse_parameters(const boost::json::object& atom_object)
{
    auto result = std::vector<int> {};
    for (const auto& parameter : ygg::common::as_array(atom_object, "parameters", "atom"))
        result.push_back(static_cast<int>(ygg::common::as_size(parameter, "atom.parameters entry")));
    return result;
}

auto atom(tyr::formalism::planning::Repository& repo, std::string_view predicate_name, const std::vector<int>& parameters)
{
    auto predicate_builder = ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>> { std::string(predicate_name), ygg::uint_t(parameters.size()) };
    const auto predicate = repo.insert(predicate_builder).first;

    auto terms = std::vector<ygg::Data<tyr::formalism::Term>> {};
    for (const auto i : parameters)
        terms.push_back(ygg::Data<tyr::formalism::Term>(tyr::formalism::ParameterIndex(i)));

    return tyr::formalism::planning::MutableAtom<tyr::formalism::FluentTag>(predicate, terms);
}

tyr::formalism::planning::invariant::Invariant parse_invariant(tyr::formalism::planning::Repository& repository, const boost::json::object& invariant_object)
{
    auto atoms = std::vector<tyr::formalism::planning::MutableAtom<tyr::formalism::FluentTag>> {};
    for (const auto& atom_value : ygg::common::as_array(invariant_object, "atoms", "invariant"))
    {
        const auto& atom_object = ygg::common::as_object(atom_value, "atom");
        atoms.push_back(atom(repository, ygg::common::as_string(atom_object, "predicate", "atom"), parse_parameters(atom_object)));
    }

    return tyr::formalism::planning::invariant::Invariant(ygg::common::as_size(invariant_object, "num_rigid_variables", "invariant"),
                                                          ygg::common::as_size(invariant_object, "num_counted_variables", "invariant"),
                                                          std::move(atoms));
}

std::vector<tyr::formalism::planning::invariant::Invariant> parse_invariants(tyr::formalism::planning::Repository& repository,
                                                                             const boost::json::object& case_object)
{
    auto result = std::vector<tyr::formalism::planning::invariant::Invariant> {};
    for (const auto& invariant_value : ygg::common::as_array(case_object, "invariants", "case"))
        result.push_back(parse_invariant(repository, ygg::common::as_object(invariant_value, "invariant")));
    return result;
}

void expect_invariant_sets_equal(std::vector<tyr::formalism::planning::invariant::Invariant> actual,
                                 std::vector<tyr::formalism::planning::invariant::Invariant> expected)
{
    EXPECT_EQ(actual.size(), expected.size());
    EXPECT_TRUE(std::is_permutation(actual.begin(), actual.end(), expected.begin(), expected.end()));
}
}

TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesis)
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / "tests/fixtures/formalism/invariants/synthesis.json");
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
    {
        const auto& case_object = ygg::common::as_object(case_value, "case");
        const auto name = ygg::common::as_string(case_object, "name", "case");

        SCOPED_TRACE(name);

        auto lifted_task =
            make_test_parser(ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "domain_file", "case")))
                .parse_task(ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "task_file", "case")));
        auto& repository = *lifted_task.get_repository();

        auto actual = tyr::formalism::planning::invariant::synthesize_invariants(lifted_task.get_task().get_domain());
        auto expected = parse_invariants(repository, case_object);

        expect_invariant_sets_equal(actual, expected);
    }
}

}
