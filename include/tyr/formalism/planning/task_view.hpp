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

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind Kind, typename V>
struct view_mixin<::tyr::formalism::planning::Task<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    const auto& get_name() const noexcept { return self().data().name; }
    auto get_domain() const noexcept { return self().template member<&urd::record_of<V>::domain>(); }
    auto get_derived_predicates() const noexcept { return self().template member<&urd::record_of<V>::derived_predicates>(); }
    auto get_objects() const noexcept { return self().template member<&urd::record_of<V>::objects>(); }
    [[nodiscard]] size_t get_num_objects() const noexcept { return get_domain().get_constants().size() + get_objects().size(); }
    template<::tyr::formalism::FactKind T>
    auto get_atoms() const noexcept
    { return urd::resolve(self().data().template get_atoms<T>(), self().context()); }
    template<::tyr::formalism::FactKind T>
    auto get_fterm_values() const noexcept
    { return urd::resolve(self().data().template get_fterm_values<T>(), self().context()); }
    auto get_auxiliary_fterm_value() const noexcept { return self().template member<&urd::record_of<V>::auxiliary_fterm_value>(); }
    auto get_goal() const noexcept { return self().template member<&urd::record_of<V>::goal>(); }
    auto get_metric() const noexcept { return self().template member<&urd::record_of<V>::metric>(); }
    auto get_axioms() const noexcept { return self().template member<&urd::record_of<V>::axioms>(); }

    auto get_fluent_variables() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::fluent_variables>(); }
    auto get_fluent_facts() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::fluent_facts>(); }
    auto get_ground_actions() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::ground_actions>(); }
    auto get_ground_axioms() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::ground_axioms>(); }
};
}  // namespace urd

#endif
