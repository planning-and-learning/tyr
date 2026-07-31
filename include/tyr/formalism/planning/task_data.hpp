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

#ifndef TYR_FORMALISM_PLANNING_TASK_DATA_HPP_
#define TYR_FORMALISM_PLANNING_TASK_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/domain_index.hpp"
#include "tyr/formalism/planning/ground_atom_index.hpp"
#include "tyr/formalism/planning/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/ground_function_term_value_index.hpp"
#include "tyr/formalism/planning/metric_index.hpp"
#include "tyr/formalism/planning/task_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

namespace ygg
{


template<>
struct Data<::tyr::formalism::planning::Task>
{
    ygg::Index<::tyr::formalism::planning::Task> index;
    ::cista::offset::string name;
    ygg::Index<::tyr::formalism::planning::Domain> domain;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>> derived_predicates;
    ygg::IndexList<::tyr::formalism::Object> objects;
    ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> static_atoms;
    ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::FluentTag>> fluent_atoms;
    ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>> static_fterm_values;
    ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::FluentTag>> fluent_fterm_values;
    ::cista::optional<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value;
    ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> goal;
    ::cista::optional<ygg::Index<::tyr::formalism::planning::Metric>> metric;
    ygg::IndexList<::tyr::formalism::planning::Axiom> axioms;

    Data() = default;
    Data(::cista::offset::string name_,
         ygg::Index<::tyr::formalism::planning::Domain> domain_,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>> derived_predicates_,
         ygg::IndexList<::tyr::formalism::Object> objects_,
         ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> static_atoms_,
         ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::FluentTag>> fluent_atoms_,
         ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>> static_fterm_values_,
         ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::FluentTag>> fluent_fterm_values_,
         ::cista::optional<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value_,
         ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> goal_,
         ::cista::optional<ygg::Index<::tyr::formalism::planning::Metric>> metric_,
         ygg::IndexList<::tyr::formalism::planning::Axiom> axioms_) :
        index(),
        name(std::move(name_)),
        domain(domain_),
        derived_predicates(std::move(derived_predicates_)),
        objects(std::move(objects_)),
        static_atoms(std::move(static_atoms_)),
        fluent_atoms(std::move(fluent_atoms_)),
        static_fterm_values(std::move(static_fterm_values_)),
        fluent_fterm_values(std::move(fluent_fterm_values_)),
        auxiliary_fterm_value(auxiliary_fterm_value_),
        goal(goal_),
        metric(metric_),
        axioms(std::move(axioms_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::string& name_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::Domain>, C> domain_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>, C>>& derived_predicates_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Object>, C>>& objects_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>>, C>>& static_atoms_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::FluentTag>>, C>>& fluent_atoms_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>>, C>>& static_fterm_values_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::FluentTag>>, C>>& fluent_fterm_values_,
         const std::optional<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>, C>>& auxiliary_fterm_value_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition>, C> goal_,
         const std::optional<::ygg::View<ygg::Index<::tyr::formalism::planning::Metric>, C>>& metric_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Axiom>, C>>& axioms_) :
        index(),
        name(name_),
        domain(),
        derived_predicates(),
        objects(),
        static_atoms(),
        fluent_atoms(),
        static_fterm_values(),
        fluent_fterm_values(),
        auxiliary_fterm_value(),
        goal(),
        metric(),
        axioms()
    {
        set(domain_, domain);
        set(derived_predicates_, derived_predicates);
        set(objects_, objects);
        set(static_atoms_, static_atoms);
        set(fluent_atoms_, fluent_atoms);
        set(static_fterm_values_, static_fterm_values);
        set(fluent_fterm_values_, fluent_fterm_values);
        set(auxiliary_fterm_value_, auxiliary_fterm_value);
        set(goal_, goal);
        set(metric_, metric);
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
        ygg::clear(domain);
        ygg::clear(derived_predicates);
        ygg::clear(objects);
        ygg::clear(static_atoms);
        ygg::clear(fluent_atoms);
        ygg::clear(static_fterm_values);
        ygg::clear(fluent_fterm_values);
        ygg::clear(auxiliary_fterm_value);
        ygg::clear(goal);
        ygg::clear(metric);
        ygg::clear(axioms);
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_atoms() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_atoms;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_atoms;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_fterm_values() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_fterm_values;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_fterm_values;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    auto cista_members() const noexcept
    {
        return std::tie(index,
                        name,
                        domain,
                        derived_predicates,
                        objects,
                        static_atoms,
                        fluent_atoms,
                        static_fterm_values,
                        fluent_fterm_values,
                        auxiliary_fterm_value,
                        goal,
                        metric,
                        axioms);
    }
    auto identifying_members() const noexcept
    {
        return std::tie(name,
                        domain,
                        derived_predicates,
                        objects,
                        static_atoms,
                        fluent_atoms,
                        static_fterm_values,
                        fluent_fterm_values,
                        auxiliary_fterm_value,
                        goal,
                        metric,
                        axioms);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Task>);
}

#endif
