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

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <optional>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{

template<::tyr::TaskKind T>
struct data<::tyr::formalism::datalog::Program<T>>
{
    urd::index<::tyr::formalism::datalog::Program<T>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>> static_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>> fluent_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>> static_functions;
    urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>> fluent_functions;
    urd::flat_vector<urd::index<::tyr::formalism::Object>> objects;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values;
    urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values;
    urd::flat_optional<urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>> goal;
    urd::flat_vector<urd::index<::tyr::formalism::datalog::Rule<T>>> rules;

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>> static_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>> fluent_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>> static_functions_,
         urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>> fluent_functions_,
         urd::flat_vector<urd::index<::tyr::formalism::Object>> objects_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values_,
         urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values_,
         urd::flat_optional<urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>> goal_,
         urd::flat_vector<urd::index<::tyr::formalism::datalog::Rule<T>>> rules_) :
        index(),
        static_predicates(std::move(static_predicates_)),
        fluent_predicates(std::move(fluent_predicates_)),
        static_functions(std::move(static_functions_)),
        fluent_functions(std::move(fluent_functions_)),
        objects(std::move(objects_)),
        static_atoms(std::move(static_atoms_)),
        fluent_atoms(std::move(fluent_atoms_)),
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

    auto& get_rules() noexcept { return rules; }
    const auto& get_rules() const noexcept { return rules; }

    template<::tyr::formalism::FactKind F>
    const auto& get_predicates() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, F, &data::static_predicates, &data::fluent_predicates>(*this); }

    template<::tyr::formalism::FactKind F>
    const auto& get_functions() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, F, &data::static_functions, &data::fluent_functions>(*this); }

    template<::tyr::formalism::FactKind F>
    const auto& get_atoms() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, F, &data::static_atoms, &data::fluent_atoms>(*this); }

    template<::tyr::formalism::FactKind F>
    const auto& get_fterm_values() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, F, &data::static_fterm_values, &data::fluent_fterm_values>(*this); }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&data::index,
                            &data::static_predicates,
                            &data::fluent_predicates,
                            &data::static_functions,
                            &data::fluent_functions,
                            &data::objects,
                            &data::static_atoms,
                            &data::fluent_atoms,
                            &data::static_fterm_values,
                            &data::fluent_fterm_values,
                            &data::goal,
                            &data::rules> {};
    }
    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&data::static_predicates,
                            &data::fluent_predicates,
                            &data::static_functions,
                            &data::fluent_functions,
                            &data::objects,
                            &data::static_atoms,
                            &data::fluent_atoms,
                            &data::static_fterm_values,
                            &data::fluent_fterm_values,
                            &data::goal,
                            &data::rules> {};
    }
};

}

#endif
