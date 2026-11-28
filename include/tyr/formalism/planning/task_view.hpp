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

#ifndef TYR_FORMALISM_PLANNING_TASK_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_TASK_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/base.h>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind Kind, typename V>
struct view_mixin<::tyr::formalism::Task<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    decltype(auto) name() const noexcept { return self().template member<&urd::record_of<V>::name>(); }

    /// What it requires of a planner, as its parser set them from what it uses.
    decltype(auto) requirements() const noexcept { return self().template member<&urd::record_of<V>::requirements>(); }

    auto domain() const noexcept { return self().template member<&urd::record_of<V>::domain>(); }

    auto derived_predicates() const noexcept { return self().template member<&urd::record_of<V>::derived_predicates>(); }

    auto objects() const noexcept { return self().template member<&urd::record_of<V>::objects>(); }

    auto static_atoms() const noexcept { return self().template member<&urd::record_of<V>::static_atoms>(); }

    auto fluent_atoms() const noexcept { return self().template member<&urd::record_of<V>::fluent_atoms>(); }

    auto derived_atoms() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::derived_atoms>(); }

    auto static_fterm_values() const noexcept { return self().template member<&urd::record_of<V>::static_fterm_values>(); }

    auto fluent_fterm_values() const noexcept { return self().template member<&urd::record_of<V>::fluent_fterm_values>(); }

    auto auxiliary_fterm_value() const noexcept { return self().template member<&urd::record_of<V>::auxiliary_fterm_value>(); }

    auto goal() const noexcept { return self().template member<&urd::record_of<V>::goal>(); }

    auto metric() const noexcept { return self().template member<&urd::record_of<V>::metric>(); }

    auto axioms() const noexcept { return self().template member<&urd::record_of<V>::axioms>(); }

    /// Timed initial literals (PDDL 2.2), by time.
    auto timed_literals() const noexcept { return self().template member<&urd::record_of<V>::timed_literals>(); }

    auto fluent_variables() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::fluent_variables>(); }

    auto fluent_facts() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::fluent_facts>(); }

    auto ground_actions() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::ground_actions>(); }

    auto ground_axioms() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::ground_axioms>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind T>
    auto atoms() const noexcept
    { return self().template member<::tyr::formalism::task_atoms_of<T, Kind>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind T>
    auto fterm_values() const noexcept
    { return self().template member<::tyr::formalism::task_fterm_values_of<T, Kind>>(); }

    [[nodiscard]] size_t num_objects() const noexcept { return domain().constants().size() + objects().size(); }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"num_objects", &view_mixin::num_objects>> {}; }
};
}  // namespace urd

/// A task as a PDDL problem: (define (problem name) its domain, objects, initial state, goal and metric, one per line); a ground task is written as the problem
/// it grounds.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Task<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Task<T>, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

#endif
