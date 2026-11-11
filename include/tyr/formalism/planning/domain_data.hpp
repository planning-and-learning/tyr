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

#ifndef TYR_FORMALISM_PLANNING_DOMAIN_DATA_HPP_
#define TYR_FORMALISM_PLANNING_DOMAIN_DATA_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <optional>
#include <string>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::Domain>
{
    urd::index<::tyr::formalism::planning::Domain> index;
    urd::flat_string name;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>> static_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>> fluent_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>> static_functions;
    urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>> fluent_functions;
    urd::flat_optional<urd::index<::tyr::formalism::Function<::tyr::formalism::AuxiliaryTag>>> auxiliary_function;
    urd::flat_vector<urd::index<::tyr::formalism::Object>> constants;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Action<::tyr::LiftedTag>>> actions;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>> axioms;

    data() = default;
    data(urd::flat_string name_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>> static_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>> fluent_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>> static_functions_,
         urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>> fluent_functions_,
         urd::flat_optional<urd::index<::tyr::formalism::Function<::tyr::formalism::AuxiliaryTag>>> auxiliary_function_,
         urd::flat_vector<urd::index<::tyr::formalism::Object>> constants_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Action<::tyr::LiftedTag>>> actions_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>> axioms_) :
        index(),
        name(std::move(name_)),
        static_predicates(std::move(static_predicates_)),
        fluent_predicates(std::move(fluent_predicates_)),
        derived_predicates(std::move(derived_predicates_)),
        static_functions(std::move(static_functions_)),
        fluent_functions(std::move(fluent_functions_)),
        auxiliary_function(std::move(auxiliary_function_)),
        constants(std::move(constants_)),
        actions(std::move(actions_)),
        axioms(std::move(axioms_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_predicates() const
    {
        return ::tyr::formalism::
            tagged_member<::tyr::formalism::StaticFluentDerivedTags, T, &data::static_predicates, &data::fluent_predicates, &data::derived_predicates>(*this);
    }

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_functions() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, T, &data::static_functions, &data::fluent_functions>(*this); }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&data::index,
                            &data::name,
                            &data::static_predicates,
                            &data::fluent_predicates,
                            &data::derived_predicates,
                            &data::static_functions,
                            &data::fluent_functions,
                            &data::auxiliary_function,
                            &data::constants,
                            &data::actions,
                            &data::axioms> {};
    }
    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&data::name,
                            &data::static_predicates,
                            &data::fluent_predicates,
                            &data::derived_predicates,
                            &data::static_functions,
                            &data::fluent_functions,
                            &data::auxiliary_function,
                            &data::constants,
                            &data::actions,
                            &data::axioms> {};
    }
};

}

#endif
