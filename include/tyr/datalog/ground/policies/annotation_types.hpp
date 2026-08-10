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

#ifndef TYR_DATALOG_GROUND_POLICIES_ANNOTATION_TYPES_HPP_
#define TYR_DATALOG_GROUND_POLICIES_ANNOTATION_TYPES_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <optional>
#include <utility>

namespace tyr::datalog
{

template<>
struct NumericSupportKey<GroundTag>
{
    using type = ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>;
};

template<::tyr::formalism::RelationKind R>
struct WitnessRuleKey<GroundTag, R>
{
    using type = ::tyr::formalism::datalog::GroundRuleView<R>;
};

template<>
struct NumericIntervalBindingParts<GroundTag>
{
    using Binding = NumericSupportKeyT<GroundTag>;
    using Relation = ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>;
    using Key = ygg::Index<::tyr::formalism::Row>;

    static Relation get_relation(Binding binding) noexcept { return binding.get_function().get_index(); }
    static Key get_key(Binding binding) noexcept { return binding.get_row().get_index().row; }
};

/// Ground queue updates are cleared, written once, and consumed immediately; use a multi-entry store if updates are ever batched.
class GroundDeltaPredicateAnnotations
{
public:
    using Key = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;

    void clear() noexcept { m_entry.reset(); }

    void insert_or_assign(Key key, Annotation<GroundTag> annotation) { m_entry.emplace(key, std::move(annotation)); }

    const Annotation<GroundTag>* find(Key key) const noexcept { return m_entry && m_entry->first == key ? &m_entry->second : nullptr; }

private:
    std::optional<std::pair<Key, Annotation<GroundTag>>> m_entry;
};

/// Ground numeric updates follow the same single-entry queue transaction.
class GroundDeltaFunctionAnnotations
{
public:
    using Binding = NumericSupportKeyT<GroundTag>;
    using Entry = NumericIntervalAnnotation<GroundTag>;

    void clear() noexcept { m_entry.reset(); }

    void insert(Binding binding, ygg::ClosedInterval<ygg::float_t> interval, Annotation<GroundTag, ::tyr::formalism::FunctionTag> annotation)
    {
        if (!empty(interval))
            m_entry.emplace(binding, Entry { interval, std::move(annotation) });
    }

    const Annotation<GroundTag, ::tyr::formalism::FunctionTag>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        return m_entry && m_entry->first == binding && m_entry->second.interval == interval ? &m_entry->second.annotation : nullptr;
    }

private:
    std::optional<std::pair<Binding, Entry>> m_entry;
};

}

#endif
