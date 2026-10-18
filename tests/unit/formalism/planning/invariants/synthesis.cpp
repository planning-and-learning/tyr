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

#include <algorithm>
#include <boost/json.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <iterator>
#include <set>
#include <string>
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

tyr::formalism::planning::MutableInvariant parse_invariant(tyr::formalism::planning::Repository& repository, const boost::json::object& invariant_object)
{
    auto atoms = std::vector<tyr::formalism::planning::MutableAtom<tyr::formalism::FluentTag>> {};
    for (const auto& atom_value : ygg::common::as_array(invariant_object, "atoms", "invariant"))
    {
        const auto& atom_object = ygg::common::as_object(atom_value, "atom");
        atoms.push_back(atom(repository, ygg::common::as_string(atom_object, "predicate", "atom"), parse_parameters(atom_object)));
    }

    return tyr::formalism::planning::MutableInvariant(ygg::common::as_size(invariant_object, "num_rigid_variables", "invariant"),
                                                      ygg::common::as_size(invariant_object, "num_counted_variables", "invariant"),
                                                      std::move(atoms));
}

std::vector<tyr::formalism::planning::MutableInvariant>
parse_invariants(tyr::formalism::planning::Repository& repository, const boost::json::object& case_object, const char* key)
{
    auto result = std::vector<tyr::formalism::planning::MutableInvariant> {};
    for (const auto& invariant_value : ygg::common::as_array(case_object, key, "case"))
        result.push_back(parse_invariant(repository, ygg::common::as_object(invariant_value, "invariant")));
    return result;
}

/// The atoms in text form, sorted. Both sides are canonical (MutableInvariant::canonicalize() numbers the rigid variables
/// by predicate names), but live in different repositories, so atoms are compared by text rather than by index.
std::string invariant_text(const tyr::formalism::planning::MutableInvariant& invariant)
{
    auto atoms = std::vector<std::string> {};
    for (const auto& atom : invariant.atoms)
        atoms.push_back(fmt::format("{}", atom));
    std::ranges::sort(atoms);
    return fmt::format("{} {} {{{}}}", invariant.num_rigid_variables, invariant.num_counted_variables, fmt::join(atoms, ", "));
}

template<typename Invariants>
std::set<std::string> invariant_texts(const Invariants& invariants)
{
    auto result = std::set<std::string> {};
    for (const auto& invariant : invariants)
        result.insert(invariant_text(tyr::formalism::planning::MutableInvariant(invariant)));
    return result;
}

/// The expected invariants of the classical and IPC cases are tyr's own, each proven by Fast Downward's balance check
/// (fixture_generation.py).
void expect_invariant_sets_equal(const tyr::formalism::planning::InvariantViewList& actual, const tyr::formalism::planning::MutableInvariantList& expected)
{
    const auto actual_texts = invariant_texts(actual);
    const auto expected_texts = invariant_texts(expected);
    auto only_actual = std::vector<std::string> {};
    auto only_expected = std::vector<std::string> {};
    std::ranges::set_difference(actual_texts, expected_texts, std::back_inserter(only_actual));
    std::ranges::set_difference(expected_texts, actual_texts, std::back_inserter(only_expected));
    EXPECT_TRUE(only_actual.empty() && only_expected.empty())
        << "only tyr: " << fmt::format("{}", fmt::join(only_actual, " ")) << "\nonly expected: " << fmt::format("{}", fmt::join(only_expected, " "));
}

void check_suite(const std::filesystem::path& fixture)
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / fixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
    {
        const auto& case_object = ygg::common::as_object(case_value, "case");
        const auto name = ygg::common::as_string(case_object, "name", "case");

        SCOPED_TRACE(name);

        auto parser =
            make_test_parser(ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "domain_file", "case")));
        const auto domain = parser.get_domain();
        const auto actual = tyr::formalism::planning::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository());

        auto lifted_task =
            parser.parse_task(ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "task_file", "case")));
        auto& repository = *lifted_task.get_repository();

        expect_invariant_sets_equal(actual, parse_invariants(repository, case_object, "invariants"));
    }
}
}

TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesis) { check_suite("tests/fixtures/formalism/invariants/synthesis.json"); }

/// One domain file per IPC domain directory (up to five for directories with a domain file per problem).
TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesisIpc) { check_suite("tests/fixtures/formalism/invariants/ipc.json"); }
}
