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

#include "tyr/formalism/planning/invariants/synthesis.hpp"

#include "planning/parser.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/invariants/ordering.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/mutable/formatter.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <iterator>
#include <set>
#include <string>
#include <yggdrasil/formatting/cista_formatters.hpp>
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
    auto predicate_builder =
        ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>> { std::string(predicate_name), static_cast<ygg::uint_t>(parameters.size()) };
    const auto predicate = repo.insert(predicate_builder).first;

    auto terms = std::vector<ygg::Data<tyr::formalism::Term>> {};
    for (const auto i : parameters)
        terms.emplace_back(tyr::formalism::ParameterIndex(i));

    return tyr::formalism::planning::MutableAtom<tyr::formalism::FluentTag>(predicate, terms);
}

auto intern_invariant(tyr::formalism::planning::Repository& repository, const tyr::formalism::planning::MutableInvariant& invariant)
{
    auto data = ygg::Data<tyr::formalism::planning::Invariant>(invariant.num_rigid_variables, invariant.num_counted_variables, {});
    for (const auto& atom : invariant.atoms)
    {
        auto terms = ygg::DataList<tyr::formalism::Term> {};
        for (const auto& term : atom.terms)
            terms.push_back(term);
        auto atom_data = ygg::Data<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::FluentTag>>(atom.predicate.get_index(), std::move(terms));
        data.atoms.push_back(tyr::formalism::planning::insert(repository, atom_data).first.get_index());
    }
    return tyr::formalism::planning::insert(repository, data).first;
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

/// The atoms in text form, sorted. Both sides live in different repositories, so atoms are compared by text rather than
/// by index.
std::string invariant_text(size_t num_rigid_variables, size_t num_counted_variables, std::vector<std::string> atoms)
{
    std::ranges::sort(atoms);
    return fmt::format("{} {} {{{}}}", num_rigid_variables, num_counted_variables, fmt::join(atoms, ", "));
}

std::string invariant_text(const tyr::formalism::planning::MutableInvariant& invariant)
{
    auto atoms = std::vector<std::string> {};
    for (const auto& atom : invariant.atoms)
        atoms.push_back(fmt::format("{}", atom));
    return invariant_text(invariant.num_rigid_variables, invariant.num_counted_variables, std::move(atoms));
}

/// Formats the atoms like MutableAtom's formatter, so both sides give the same text.
std::string invariant_text(tyr::formalism::planning::InvariantView invariant)
{
    auto atoms = std::vector<std::string> {};
    for (const auto atom : invariant.get_atoms())
    {
        auto terms = std::vector<std::string> {};
        for (const auto term : atom.get_terms())
            terms.push_back(fmt::format("{}", term.get_data()));
        atoms.push_back(fmt::format("({} {})", atom.get_predicate().get_name(), fmt::join(terms, " ")));
    }
    return invariant_text(invariant.get_num_rigid_variables(), invariant.get_num_counted_variables(), std::move(atoms));
}

template<typename Invariants>
std::set<std::string> invariant_texts(const Invariants& invariants)
{
    auto result = std::set<std::string> {};
    for (const auto& invariant : invariants)
        result.insert(invariant_text(invariant));
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
        // Synthesizing again finds the interned invariants.
        EXPECT_EQ(tyr::formalism::planning::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository()), actual);

        auto lifted_task =
            parser.parse_task(ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "task_file", "case")));
        auto& repository = *lifted_task.get_repository();

        expect_invariant_sets_equal(actual, parse_invariants(repository, case_object, "invariants"));
    }
}

/// The greedy orders over the synthesized invariants equal the recorded ones (generate_orders.py), invariant by invariant.
void check_orders(const std::filesystem::path& fixture)
{
    namespace invariant = tyr::formalism::planning::invariant;
    const auto criteria = std::vector<std::pair<const char*, invariant::GreedyCriterion>> { { "max_group_size", invariant::GreedyCriterion::MaxGroupSize },
                                                                                            { "max_coverage", invariant::GreedyCriterion::MaxCoverage } };

    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / fixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
    {
        const auto& case_object = ygg::common::as_object(case_value, "case");
        SCOPED_TRACE(ygg::common::as_string(case_object, "name", "case"));

        auto parser =
            make_test_parser(ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "domain_file", "case")));
        const auto domain = parser.get_domain();
        auto& repository = *domain.get_repository();
        const auto invariants = invariant::synthesize_invariants(domain.get_domain(), repository);

        const auto& orders = ygg::common::as_object(case_object, "orders", "case");
        for (const auto& [key, criterion] : criteria)
        {
            SCOPED_TRACE(key);
            // The order depends on the set of invariants, not on their order.
            EXPECT_EQ(invariant::compute_invariant_order_greedy(tyr::formalism::planning::InvariantViewList(invariants.rbegin(), invariants.rend()), criterion),
                      invariant::compute_invariant_order_greedy(invariants, criterion));
            auto actual = std::vector<std::string> {};
            for (const auto inv : invariant::compute_invariant_order_greedy(invariants, criterion))
                actual.push_back(invariant_text(inv));
            auto expected = std::vector<std::string> {};
            for (const auto& inv : parse_invariants(repository, orders, key))
                expected.push_back(invariant_text(inv));
            EXPECT_EQ(actual, expected);
        }
    }
}
}

TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesis) { check_suite("tests/fixtures/formalism/invariants/synthesis.json"); }

/// One domain file per IPC domain directory (up to five for directories with a domain file per problem).
TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesisIpc) { check_suite("tests/fixtures/formalism/invariants/ipc.json"); }

/// Every domain of ipc.json and synthesis.json, so that any change to an order shows up in review.
TEST(TyrTests, TyrFormalismPlanningInvariantOrders) { check_orders("tests/fixtures/formalism/invariants/orders.json"); }

TEST(TyrTests, TyrFormalismPlanningInvariantOrderCountsDistinctVariables)
{
    namespace planning = tyr::formalism::planning;
    namespace invariant = planning::invariant;
    auto repository = planning::RepositoryFactory().create();
    auto object_data = ygg::Data<tyr::formalism::Object>(std::string("fixed"));
    const auto object = repository.insert(object_data).first;
    auto repeated_atom = atom(repository, "repeated", { 0, 1, 1, 1 });
    repeated_atom.terms.back() = ygg::Data<tyr::formalism::Term>(object.get_index());

    const auto repeated = intern_invariant(repository, planning::MutableInvariant(1, 1, { repeated_atom }));
    const auto linear = intern_invariant(repository, planning::MutableInvariant(1, 1, { atom(repository, "p", { 0, 1 }), atom(repository, "q", { 0, 1 }) }));
    const auto quadratic = intern_invariant(repository, planning::MutableInvariant(1, 2, { atom(repository, "pair", { 0, 1, 2 }) }));
    const auto expected = planning::InvariantViewList { quadratic, linear, repeated };
    for (const auto criterion : { invariant::GreedyCriterion::MaxGroupSize, invariant::GreedyCriterion::MaxCoverage })
    {
        EXPECT_EQ(invariant::compute_invariant_order_greedy({ repeated, linear, quadratic }, criterion), expected);
        EXPECT_EQ(invariant::compute_invariant_order_greedy({ quadratic, linear, repeated }, criterion), expected);
    }
}

TEST(TyrTests, TyrFormalismPlanningInvariantOrderMultipliesCoverage)
{
    namespace planning = tyr::formalism::planning;
    namespace invariant = planning::invariant;
    auto repository = planning::RepositoryFactory().create();
    const auto linear = intern_invariant(repository, planning::MutableInvariant(2, 1, { atom(repository, "linear", { 0, 1, 2 }) }));
    const auto quadratic = intern_invariant(repository, planning::MutableInvariant(1, 2, { atom(repository, "quadratic", { 0, 1, 2 }) }));

    EXPECT_EQ(invariant::compute_invariant_order_greedy({ linear, quadratic }, invariant::GreedyCriterion::MaxGroupSize),
              (planning::InvariantViewList { quadratic, linear }));
    // Both cover n^3 atoms, so coverage ties go to the invariant interned first.
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ quadratic, linear }, invariant::GreedyCriterion::MaxCoverage),
              (planning::InvariantViewList { linear, quadratic }));
}

TEST(TyrTests, TyrFormalismPlanningInvariantOrderRecomputesResidualPolynomial)
{
    namespace planning = tyr::formalism::planning;
    namespace invariant = planning::invariant;
    auto repository = planning::RepositoryFactory().create();
    const auto shared = atom(repository, "shared", { 0, 1 });
    const auto residual = intern_invariant(repository, planning::MutableInvariant(0, 2, { shared, atom(repository, "tail", { 0 }) }));
    const auto winner = intern_invariant(repository, planning::MutableInvariant(0, 2, { shared, atom(repository, "pair", { 0, 1 }) }));
    const auto linear = intern_invariant(repository, planning::MutableInvariant(0, 1, { atom(repository, "p", { 0 }), atom(repository, "q", { 0 }) }));
    const auto singleton = intern_invariant(repository, planning::MutableInvariant(0, 2, { shared, atom(repository, "fixed", {}) }));

    for (const auto criterion : { invariant::GreedyCriterion::MaxGroupSize, invariant::GreedyCriterion::MaxCoverage })
        EXPECT_EQ(invariant::compute_invariant_order_greedy({ residual, winner, linear, singleton }, criterion),
                  (planning::InvariantViewList { winner, linear, residual }));
}
}
