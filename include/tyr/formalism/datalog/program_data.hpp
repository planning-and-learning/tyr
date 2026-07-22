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
#include "tyr/formalism/datalog/ground_atom_index.hpp"
#include "tyr/formalism/datalog/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/ground_function_term_value_index.hpp"
#include "tyr/formalism/datalog/metric_index.hpp"
#include "tyr/formalism/datalog/program_index.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::datalog::Program>
{
    ygg::Index<::tyr::formalism::datalog::Program> index;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>> static_predicates;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> fluent_predicates;
    ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::StaticTag>> static_functions;
    ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::FluentTag>> fluent_functions;
    ygg::IndexList<::tyr::formalism::Object> objects;
    ygg::IndexList<::tyr::formalism::datalog::GroundAtom<::tyr::formalism::StaticTag>> static_atoms;
    ygg::IndexList<::tyr::formalism::datalog::GroundAtom<::tyr::formalism::FluentTag>> fluent_atoms;
    ygg::IndexList<::tyr::formalism::datalog::GroundFunctionTermValue<::tyr::formalism::StaticTag>> static_fterm_values;
    ygg::IndexList<::tyr::formalism::datalog::GroundFunctionTermValue<::tyr::formalism::FluentTag>> fluent_fterm_values;
    ::cista::optional<ygg::Index<::tyr::formalism::datalog::GroundConjunctiveCondition>> goal;
    ::cista::optional<ygg::Index<::tyr::formalism::datalog::Metric>> metric;
    ygg::IndexList<::tyr::formalism::datalog::Rule> rules;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::datalog::Program> index,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>> static_predicates,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> fluent_predicates,
         ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::StaticTag>> static_functions,
         ygg::IndexList<::tyr::formalism::Function<::tyr::formalism::FluentTag>> fluent_functions,
         ygg::IndexList<::tyr::formalism::Object> objects,
         ygg::IndexList<::tyr::formalism::datalog::GroundAtom<::tyr::formalism::StaticTag>> static_atoms,
         ygg::IndexList<::tyr::formalism::datalog::GroundAtom<::tyr::formalism::FluentTag>> fluent_atoms,
         ygg::IndexList<::tyr::formalism::datalog::GroundFunctionTermValue<::tyr::formalism::StaticTag>> static_fterm_values,
         ygg::IndexList<::tyr::formalism::datalog::GroundFunctionTermValue<::tyr::formalism::FluentTag>> fluent_fterm_values,
         ::cista::optional<ygg::Index<::tyr::formalism::datalog::GroundConjunctiveCondition>> goal,
         ::cista::optional<ygg::Index<::tyr::formalism::datalog::Metric>> metric,
         ygg::IndexList<::tyr::formalism::datalog::Rule> rules) :
        index(index),
        static_predicates(std::move(static_predicates)),
        fluent_predicates(std::move(fluent_predicates)),
        static_functions(std::move(static_functions)),
        fluent_functions(std::move(fluent_functions)),
        objects(std::move(objects)),
        static_atoms(std::move(static_atoms)),
        fluent_atoms(std::move(fluent_atoms)),
        static_fterm_values(std::move(static_fterm_values)),
        fluent_fterm_values(std::move(fluent_fterm_values)),
        goal(goal),
        metric(metric),
        rules(std::move(rules))
    {
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(static_predicates);
        ygg::clear(fluent_predicates);
        ygg::clear(static_functions);
        ygg::clear(fluent_functions);
        ygg::clear(objects);
        ygg::clear(static_atoms);
        ygg::clear(fluent_atoms);
        ygg::clear(static_fterm_values);
        ygg::clear(fluent_fterm_values);
        ygg::clear(goal);
        ygg::clear(metric);
        ygg::clear(rules);
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_predicates() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_predicates;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_predicates;
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
                        static_predicates,
                        fluent_predicates,
                        static_functions,
                        fluent_functions,
                        objects,
                        static_atoms,
                        fluent_atoms,
                        static_fterm_values,
                        fluent_fterm_values,
                        goal,
                        metric,
                        rules);
    }
    auto identifying_members() const noexcept
    {
        return std::tie(static_predicates,
                        fluent_predicates,
                        static_functions,
                        fluent_functions,
                        objects,
                        static_atoms,
                        fluent_atoms,
                        static_fterm_values,
                        fluent_fterm_values,
                        goal,
                        metric,
                        rules);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Program>);

}

#endif
