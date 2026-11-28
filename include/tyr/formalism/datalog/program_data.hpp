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

#ifndef TYR_FORMALISM_DATALOG_PROGRAM_DATA_HPP_
#define TYR_FORMALISM_DATALOG_PROGRAM_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/tags.hpp>
#include <utility>

namespace urd
{

template<::tyr::TaskKind T>
struct data<::tyr::formalism::Program<T>>
{
    urd::key<::tyr::formalism::Program<T>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>> static_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>> fluent_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>> static_functions;
    urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>> fluent_functions;
    urd::flat_vector<urd::index<::tyr::formalism::Object>> objects;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values;
    urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values;
    urd::flat_optional<urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>>> goal;
    urd::flat_vector<urd::index<::tyr::formalism::Rule<T>>> rules;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>> static_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>> fluent_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>> static_functions_,
         urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>> fluent_functions_,
         urd::flat_vector<urd::index<::tyr::formalism::Object>> objects_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values_,
         urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values_,
         urd::flat_optional<urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>>> goal_,
         urd::flat_vector<urd::index<::tyr::formalism::Rule<T>>> rules_) :
        index(),
        static_predicates(std::move(static_predicates_)),
        fluent_predicates(std::move(fluent_predicates_)),
        derived_predicates(std::move(derived_predicates_)),
        static_functions(std::move(static_functions_)),
        fluent_functions(std::move(fluent_functions_)),
        objects(std::move(objects_)),
        static_atoms(std::move(static_atoms_)),
        fluent_atoms(std::move(fluent_atoms_)),
        derived_atoms(std::move(derived_atoms_)),
        static_fterm_values(std::move(static_fterm_values_)),
        fluent_fterm_values(std::move(fluent_fterm_values_)),
        goal(std::move(goal_)),
        rules(std::move(rules_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"static_predicates", &data::static_predicates, urd::sorted, urd::unique>,
                           urd::field<"fluent_predicates", &data::fluent_predicates, urd::sorted, urd::unique>,
                           urd::field<"derived_predicates", &data::derived_predicates, urd::sorted, urd::unique>,
                           urd::field<"static_functions", &data::static_functions, urd::sorted, urd::unique>,
                           urd::field<"fluent_functions", &data::fluent_functions, urd::sorted, urd::unique>,
                           urd::field<"objects", &data::objects, urd::sorted, urd::unique>,
                           urd::field<"static_atoms", &data::static_atoms, urd::sorted, urd::unique>,
                           urd::field<"fluent_atoms", &data::fluent_atoms, urd::sorted, urd::unique>,
                           urd::field<"derived_atoms", &data::derived_atoms, urd::sorted, urd::unique>,
                           urd::field<"static_fterm_values", &data::static_fterm_values, urd::sorted, urd::unique>,
                           urd::field<"fluent_fterm_values", &data::fluent_fterm_values, urd::sorted, urd::unique>,
                           urd::field<"goal", &data::goal>,
                           urd::field<"rules", &data::rules, urd::sorted, urd::unique>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Program<T>> key() const noexcept { return urd::key<::tyr::formalism::Program<T>>(index); }
};

}

namespace tyr::formalism
{
/// The member holding the F-predicates of a program.
template<FactKind F, TaskKind T>
inline constexpr auto program_predicates_of = tagged_member_pointer<StaticFluentDerivedTags,
                                                                    F,
                                                                    &urd::data<Program<T>>::static_predicates,
                                                                    &urd::data<Program<T>>::fluent_predicates,
                                                                    &urd::data<Program<T>>::derived_predicates>;

/// The member holding the F-functions of a program.
template<FactKind F, TaskKind T>
inline constexpr auto program_functions_of =
    tagged_member_pointer<StaticFluentTags, F, &urd::data<Program<T>>::static_functions, &urd::data<Program<T>>::fluent_functions>;

/// The member holding the F-atoms of a program.
template<FactKind F, TaskKind T>
inline constexpr auto program_atoms_of = tagged_member_pointer<StaticFluentDerivedTags,
                                                               F,
                                                               &urd::data<Program<T>>::static_atoms,
                                                               &urd::data<Program<T>>::fluent_atoms,
                                                               &urd::data<Program<T>>::derived_atoms>;

/// The member holding the F-function term values of a program.
template<FactKind F, TaskKind T>
inline constexpr auto program_fterm_values_of =
    tagged_member_pointer<StaticFluentTags, F, &urd::data<Program<T>>::static_fterm_values, &urd::data<Program<T>>::fluent_fterm_values>;
}

#endif
