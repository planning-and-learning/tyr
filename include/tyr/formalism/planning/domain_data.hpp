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

#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_index.hpp"
#include "tyr/formalism/planning/task_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::Domain>
{
    ygg::Index<::tyr::formalism::planning::Domain> index;
    ::cista::offset::string name;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>> static_predicates;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> fluent_predicates;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>> derived_predicates;
    ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::StaticTag>> static_functions;
    ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::FluentTag>> fluent_functions;
    ::cista::optional<ygg::Index<::tyr::formalism::Function<::tyr::formalism::AuxiliaryTag>>> auxiliary_function;
    ygg::IndexList<::tyr::formalism::Object> constants;
    ygg::IndexList<::tyr::formalism::planning::Action<::tyr::LiftedTag>> actions;
    ygg::IndexList<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> axioms;

    Data() = default;
    Data(::cista::offset::string name_,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>> static_predicates_,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> fluent_predicates_,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>> derived_predicates_,
         ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::StaticTag>> static_functions_,
         ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::FluentTag>> fluent_functions_,
         ::cista::optional<ygg::Index<::tyr::formalism::Function<::tyr::formalism::AuxiliaryTag>>> auxiliary_function_,
         ygg::IndexList<::tyr::formalism::Object> constants_,
         ygg::IndexList<::tyr::formalism::planning::Action<::tyr::LiftedTag>> actions_,
         ygg::IndexList<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> axioms_) :
        index(),
        name(std::move(name_)),
        static_predicates(std::move(static_predicates_)),
        fluent_predicates(std::move(fluent_predicates_)),
        derived_predicates(std::move(derived_predicates_)),
        static_functions(std::move(static_functions_)),
        fluent_functions(std::move(fluent_functions_)),
        auxiliary_function(auxiliary_function_),
        constants(std::move(constants_)),
        actions(std::move(actions_)),
        axioms(std::move(axioms_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::string& name_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>, C>>& static_predicates_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>, C>>& fluent_predicates_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>, C>>& derived_predicates_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>, C>>& static_functions_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>, C>>& fluent_functions_,
         const std::optional<::ygg::View<ygg::Index<::tyr::formalism::Function<::tyr::formalism::AuxiliaryTag>>, C>>& auxiliary_function_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Object>, C>>& constants_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Action<::tyr::LiftedTag>>, C>>& actions_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>, C>>& axioms_) :
        index(),
        name(name_),
        static_predicates(),
        fluent_predicates(),
        derived_predicates(),
        static_functions(),
        fluent_functions(),
        auxiliary_function(),
        constants(),
        actions(),
        axioms()
    {
        set(static_predicates_, static_predicates);
        set(fluent_predicates_, fluent_predicates);
        set(derived_predicates_, derived_predicates);
        set(static_functions_, static_functions);
        set(fluent_functions_, fluent_functions);
        set(auxiliary_function_, auxiliary_function);
        set(constants_, constants);
        set(actions_, actions);
        set(axioms_, axioms);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(name);
        ygg::clear(static_predicates);
        ygg::clear(fluent_predicates);
        ygg::clear(derived_predicates);
        ygg::clear(static_functions);
        ygg::clear(fluent_functions);
        ygg::clear(auxiliary_function);
        ygg::clear(constants);
        ygg::clear(actions);
        ygg::clear(axioms);
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_predicates() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_predicates;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_predicates;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return derived_predicates;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_functions() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_functions;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_functions;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    auto cista_members() const noexcept
    {
        return std::tie(index,
                        name,
                        static_predicates,
                        fluent_predicates,
                        derived_predicates,
                        static_functions,
                        fluent_functions,
                        auxiliary_function,
                        constants,
                        actions,
                        axioms);
    }
    auto identifying_members() const noexcept
    {
        return std::tie(name,
                        static_predicates,
                        fluent_predicates,
                        derived_predicates,
                        static_functions,
                        fluent_functions,
                        auxiliary_function,
                        constants,
                        actions,
                        axioms);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Domain>);
}

#endif
