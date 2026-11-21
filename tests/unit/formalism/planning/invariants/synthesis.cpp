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
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/parameters_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/invariant_data.hpp"
#include "tyr/formalism/planning/invariants/ordering.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <iterator>
#include <set>
#include <string>
#include <urd/elements/clone.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::tests
{

namespace
{
urd::Store<urd::sequence<int>> parse_parameters(const boost::json::object& atom_object)
{
    auto result = urd::Store<urd::sequence<int>> {};
    for (const auto& parameter : ygg::common::as_array(atom_object, "parameters", "atom"))
        result.push_back(static_cast<int>(ygg::common::as_size(parameter, "atom.parameters entry")));
    return result;
}

/// An atom of the predicate (interned, of the parameters' arity) over these parameters, as a record.
auto atom(tyr::formalism::Repository& repo, std::string_view predicate_name, const urd::Store<urd::sequence<int>>& parameters)
{
    auto predicate_builder =
        urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>> { std::string(predicate_name), static_cast<ygg::uint_t>(parameters.size()) };
    const auto predicate = repo.insert(predicate_builder).first;

    auto result = urd::data<tyr::formalism::Atom<tyr::LiftedTag, tyr::formalism::FluentTag>>();
    result.predicate = predicate.index();
    for (const auto i : parameters)
        result.terms.push_back(tyr::formalism::Term(tyr::formalism::ParameterIndex(i)));
    return result;
}

/// The invariant of these atoms interned, each atom a copy (urd::clone), its parameters named as synthesis names them:
/// ?v0, ?v1, ... for its rigid, then its counted variables.
template<typename... Atoms>
auto intern_invariant(tyr::formalism::Repository& repository, std::size_t num_rigid_variables, std::size_t num_counted_variables, const Atoms&... atoms)
{
    auto data = urd::data<tyr::formalism::Invariant>(num_rigid_variables, num_counted_variables, {});
    auto parameters = urd::data<tyr::formalism::Parameters> {};
    for (std::size_t position = 0; position < num_rigid_variables + num_counted_variables; ++position)
    {
        auto variable = urd::data<tyr::formalism::Variable>(urd::flat_string(fmt::format("?v{}", position)));
        parameters.variables.push_back(tyr::formalism::insert(repository, variable).first.index());
    }
    const auto parameters_index = tyr::formalism::insert(repository, parameters).first.index();
    const auto add = [&](const auto& atom)
    {
        auto atom_data = urd::clone(atom);
        atom_data.parameters = parameters_index;
        data.atoms.push_back(tyr::formalism::insert(repository, atom_data).first.index());
    };
    (add(atoms), ...);
    return tyr::formalism::insert(repository, data).first;
}

/// The atoms in text form, sorted. Both sides live in different repositories, so atoms are compared by text rather than
/// by index.
std::string invariant_text(size_t num_rigid_variables, size_t num_counted_variables, urd::Store<urd::sequence<std::string>> atoms)
{
    std::ranges::sort(atoms);
    return fmt::format("{} {} {{{}}}", num_rigid_variables, num_counted_variables, fmt::join(atoms, ", "));
}

/// An atom's text: its predicate and its terms, as `(name ?0 ?1)`.
template<typename Terms>
std::string atom_text(std::string_view predicate_name, const Terms& terms)
{ return fmt::format("({} {})", predicate_name, fmt::join(terms, " ")); }

/// An expected invariant of a fixture (num_rigid_variables, num_counted_variables, atoms of a predicate over parameters),
/// as text.
std::string invariant_text(const boost::json::object& invariant_object)
{
    auto atoms = urd::Store<urd::sequence<std::string>> {};
    for (const auto& atom_value : ygg::common::as_array(invariant_object, "atoms", "invariant"))
    {
        const auto& atom_object = ygg::common::as_object(atom_value, "atom");
        auto terms = urd::Store<urd::sequence<tyr::formalism::Term>> {};
        for (const auto i : parse_parameters(atom_object))
            terms.emplace_back(tyr::formalism::ParameterIndex(i));
        atoms.push_back(atom_text(ygg::common::as_string(atom_object, "predicate", "atom"), terms));
    }
    return invariant_text(ygg::common::as_size(invariant_object, "num_rigid_variables", "invariant"),
                          ygg::common::as_size(invariant_object, "num_counted_variables", "invariant"),
                          std::move(atoms));
}

/// The expected invariants listed under `key`, as text.
urd::Store<urd::sequence<std::string>> parse_invariant_texts(const boost::json::object& case_object, const char* key)
{
    auto result = urd::Store<urd::sequence<std::string>> {};
    for (const auto& invariant_value : ygg::common::as_array(case_object, key, "case"))
        result.push_back(invariant_text(ygg::common::as_object(invariant_value, "invariant")));
    return result;
}

/// An interned invariant as text, its atoms formatted as the fixtures' are.
std::string invariant_text(tyr::formalism::InvariantView invariant)
{
    auto atoms = urd::Store<urd::sequence<std::string>> {};
    for (const auto atom : invariant.get_atoms())
    {
        auto terms = urd::Store<urd::sequence<tyr::formalism::Term>> {};
        for (const auto term : atom.get_terms())
            terms.push_back(urd::stored<tyr::formalism::Term>(term));
        atoms.push_back(atom_text(atom.get_predicate().get_name().view(), terms));
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
void expect_invariant_sets_equal(const tyr::formalism::InvariantViewList& actual, const urd::Store<urd::sequence<std::string>>& expected)
{
    const auto actual_texts = invariant_texts(actual);
    const auto expected_texts = std::set<std::string>(expected.begin(), expected.end());
    auto only_actual = urd::Store<urd::sequence<std::string>> {};
    auto only_expected = urd::Store<urd::sequence<std::string>> {};
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
        const auto actual = tyr::formalism::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository());
        // Synthesizing again finds the interned invariants.
        EXPECT_EQ(tyr::formalism::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository()), actual);

        expect_invariant_sets_equal(actual, parse_invariant_texts(case_object, "invariants"));
    }
}

/// The greedy orders over the synthesized invariants equal the recorded ones (generate_orders.py), invariant by invariant.
void check_orders(const std::filesystem::path& fixture)
{
    namespace invariant = tyr::formalism::invariant;
    using Criterion = invariant::InvariantOrderCriterion;
    const auto criteria = urd::Store<urd::sequence<std::pair<const char*, invariant::InvariantOrderOptions>>> {
        { "min_stolen_predicates",
          invariant::InvariantOrderOptions { { Criterion::MinStolenPredicates, Criterion::MaxResidualAtoms, Criterion::Lexicographic } } },
        { "lexicographic", invariant::InvariantOrderOptions { { Criterion::Lexicographic } } },
    };

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
            EXPECT_EQ(invariant::compute_invariant_order_greedy(tyr::formalism::InvariantViewList(invariants.rbegin(), invariants.rend()), criterion),
                      invariant::compute_invariant_order_greedy(invariants, criterion));
            auto actual = urd::Store<urd::sequence<std::string>> {};
            for (const auto inv : invariant::compute_invariant_order_greedy(invariants, criterion))
                actual.push_back(invariant_text(inv));
            EXPECT_EQ(actual, parse_invariant_texts(orders, key));
        }
    }
}
}

TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesis) { check_suite("tests/support/data/formalism/invariants/synthesis.json"); }

/// One domain file per IPC domain directory (up to five for directories with a domain file per problem).
TEST(TyrTests, TyrFormalismPlanningInvariantsSynthesisIpc) { check_suite("tests/support/data/formalism/invariants/ipc.json"); }

/// Every domain of ipc.json and synthesis.json, so that any change to an order shows up in review.
TEST(TyrTests, TyrFormalismPlanningInvariantOrders) { check_orders("tests/support/data/formalism/invariants/orders.json"); }

TEST(TyrTests, TyrFormalismPlanningInvariantOrderTakesFewestStolenPredicatesFirst)
{
    namespace planning = tyr::formalism;
    namespace invariant = planning::invariant;
    auto repository = planning::Repository();
    const auto s1 = atom(repository, "s1", { 0, 1 });
    const auto s2 = atom(repository, "s2", { 0, 1 });
    // greedy steals s1 and s2, each of the others only one of them.
    const auto greedy = intern_invariant(repository, 1, 1, s1, s2, atom(repository, "a", { 0, 1 }));
    const auto first = intern_invariant(repository, 1, 1, s1, atom(repository, "b", { 0, 1 }));
    const auto second = intern_invariant(repository, 1, 1, s2, atom(repository, "c", { 0, 1 }));

    const auto order = invariant::compute_invariant_order_greedy({ greedy, first, second });
    ASSERT_EQ(order.size(), 3);
    EXPECT_NE(order.front(), greedy);
    // The order depends on the set of invariants, not on their order.
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ second, first, greedy }), order);
}

TEST(TyrTests, TyrFormalismPlanningInvariantOrderPrefersMoreResidualAtomsOnStolenTies)
{
    namespace planning = tyr::formalism;
    namespace invariant = planning::invariant;
    using Criterion = invariant::InvariantOrderCriterion;
    auto repository = planning::Repository();
    const auto free = atom(repository, "free", { 0 });
    const auto alone = intern_invariant(repository, 0, 1, free);
    const auto pair = intern_invariant(repository, 0, 1, free, atom(repository, "loaded", { 0 }));

    // Both steal free from each other. More residual atoms keeps free and loaded in one variable; alone, then without
    // mutex information, follows. The text agrees here: the pair's atoms, one per line, sort before alone's closing ].
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ alone, pair }), (planning::InvariantViewList { pair, alone }));
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ alone, pair },
                                                        invariant::InvariantOrderOptions { { Criterion::MinStolenPredicates, Criterion::Lexicographic } }),
              (planning::InvariantViewList { pair, alone }));
}

TEST(TyrTests, TyrFormalismPlanningInvariantOrderPutsResidualsWithoutMutexInformationLast)
{
    namespace planning = tyr::formalism;
    namespace invariant = planning::invariant;
    auto repository = planning::Repository();
    const auto shared = atom(repository, "shared", { 0, 1 });
    const auto taker = intern_invariant(repository, 0, 2, shared, atom(repository, "pair", { 0, 1 }));
    // Once taker takes shared, singleton keeps one atom without counted variables, residual one with them.
    const auto singleton = intern_invariant(repository, 0, 2, shared, atom(repository, "fixed", {}));
    const auto residual = intern_invariant(repository, 0, 2, shared, atom(repository, "tail", { 0 }));
    // Without criteria the input order decides, so taker goes first; singleton is not left out but comes last.
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ taker, singleton, residual }, invariant::InvariantOrderOptions { {} }),
              (planning::InvariantViewList { taker, residual, singleton }));
}

TEST(TyrTests, TyrFormalismPlanningInvariantOrderCriteriaCompose)
{
    namespace planning = tyr::formalism;
    namespace invariant = planning::invariant;
    using Criterion = invariant::InvariantOrderCriterion;
    auto repository = planning::Repository();
    const auto p = intern_invariant(repository, 0, 1, atom(repository, "p", { 0 }), atom(repository, "q", { 0 }));
    const auto r = intern_invariant(repository, 0, 1, atom(repository, "r", { 0 }), atom(repository, "s", { 0 }));

    // No criterion: the first remaining invariant in the input.
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ r, p }, invariant::InvariantOrderOptions { {} }), (planning::InvariantViewList { r, p }));
    // Neither steals a predicate, so MinStolenPredicates alone keeps both and the input order decides.
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ r, p }, invariant::InvariantOrderOptions { { Criterion::MinStolenPredicates } }),
              (planning::InvariantViewList { r, p }));
    EXPECT_EQ(invariant::compute_invariant_order_greedy({ r, p }, invariant::InvariantOrderOptions { { Criterion::Lexicographic } }),
              invariant::compute_invariant_order_greedy({ p, r }, invariant::InvariantOrderOptions { { Criterion::Lexicographic } }));
    // Random is reproducible from its seed.
    for (const auto seed : { std::uint64_t { 0 }, std::uint64_t { 1 }, std::uint64_t { 42 } })
    {
        const auto random = invariant::InvariantOrderOptions { { Criterion::Random }, seed };
        EXPECT_EQ(invariant::compute_invariant_order_greedy({ p, r }, random), invariant::compute_invariant_order_greedy({ p, r }, random));
        EXPECT_EQ(invariant::compute_invariant_order_greedy({ p, r }, random).size(), 2);
    }
}
}
