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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_NODE_VIEW_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_NODE_VIEW_HPP_

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"

#include <utility>
#include <urd/elements/flat.hpp>

namespace urd
{
template<typename Tag, typename V>
struct view_mixin<::tyr::planning::match_tree::AtomSelectorNode<Tag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_atom() const noexcept { return urd::resolve(self().data().atom, self().context().get_formalism_repository()); }
    auto get_true_child() const noexcept { return self().template get<&urd::record_of<V>::true_child>(); }
    auto get_false_child() const noexcept { return self().template get<&urd::record_of<V>::false_child>(); }
    auto get_dontcare_child() const noexcept { return self().template get<&urd::record_of<V>::dontcare_child>(); }
};

template<typename Tag, typename V>
struct view_mixin<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_constraint() const noexcept { return urd::resolve(self().data().constraint, self().context().get_formalism_repository()); }
    auto get_true_child() const noexcept { return self().template get<&urd::record_of<V>::true_child>(); }
    auto get_dontcare_child() const noexcept { return self().template get<&urd::record_of<V>::dontcare_child>(); }
};

template<typename Tag, typename V>
struct view_mixin<::tyr::planning::match_tree::ElementGeneratorNode<Tag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_elements() const noexcept { return urd::resolve(self().data().elements, self().context().get_formalism_repository()); }
};

template<typename Tag, typename V>
struct view_mixin<::tyr::planning::match_tree::VariableSelectorNode<Tag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_variable() const noexcept { return urd::resolve(self().data().variable, self().context().get_formalism_repository()); }
    auto get_domain_children() const noexcept { return self().template get<&urd::record_of<V>::domain_children>(); }
    auto get_dontcare_child() const noexcept { return self().template get<&urd::record_of<V>::dontcare_child>(); }
};

template<typename Tag, typename V>
struct view_mixin<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_fact() const noexcept { return urd::resolve(self().data().fact, self().context().get_formalism_repository()); }
    auto get_true_child() const noexcept { return self().template get<&urd::record_of<V>::true_child>(); }
    auto get_dontcare_child() const noexcept { return self().template get<&urd::record_of<V>::dontcare_child>(); }
};
}  // namespace urd

#endif
