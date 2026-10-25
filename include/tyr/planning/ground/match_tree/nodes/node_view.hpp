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
#include "tyr/formalism/planning/boolean_operator_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"

#include <utility>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
template<typename Tag, ::tyr::planning::match_tree::Context<Tag> C>
class View<ygg::Data<::tyr::planning::match_tree::Node<Tag>>, C> : public ygg::DataViewBase<::tyr::planning::match_tree::Node<Tag>, C>
{
public:
    View(const ygg::Data<::tyr::planning::match_tree::Node<Tag>>& handle, const C& context) noexcept :
        ygg::DataViewBase<::tyr::planning::match_tree::Node<Tag>, C>(handle, context)
    {
    }

    auto get_variant() const noexcept { return ygg::make_view(this->get_data().variant, this->get_context()); }
};
}

namespace ygg
{
template<typename Tag, ygg::formalism::SymbolContextFor<::tyr::planning::match_tree::AtomSelectorNode<Tag>> C>
class View<ygg::Index<::tyr::planning::match_tree::AtomSelectorNode<Tag>>, C> : public ygg::IndexViewBase<::tyr::planning::match_tree::AtomSelectorNode<Tag>, C>
{
public:
    View(ygg::Index<::tyr::planning::match_tree::AtomSelectorNode<Tag>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::planning::match_tree::AtomSelectorNode<Tag>, C>(handle, context)
    {
    }

    auto get_atom() const noexcept { return ygg::make_view(this->get_data().atom, this->get_context().get_formalism_repository()); }
    auto get_true_child() const noexcept { return ygg::make_view(this->get_data().true_child, this->get_context()); }
    auto get_false_child() const noexcept { return ygg::make_view(this->get_data().false_child, this->get_context()); }
    auto get_dontcare_child() const noexcept { return ygg::make_view(this->get_data().dontcare_child, this->get_context()); }
};
}

namespace ygg
{
template<typename Tag, ygg::formalism::SymbolContextFor<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>> C>
class View<ygg::Index<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>, C> :
    public ygg::IndexViewBase<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>, C>
{
public:
    View(ygg::Index<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>, C>(handle, context)
    {
    }

    auto get_constraint() const noexcept { return ygg::make_view(this->get_data().constraint, this->get_context().get_formalism_repository()); }
    auto get_true_child() const noexcept { return ygg::make_view(this->get_data().true_child, this->get_context()); }
    auto get_dontcare_child() const noexcept { return ygg::make_view(this->get_data().dontcare_child, this->get_context()); }
};
}

namespace ygg
{
template<typename Tag, ygg::formalism::SymbolContextFor<::tyr::planning::match_tree::ElementGeneratorNode<Tag>> C>
class View<ygg::Index<::tyr::planning::match_tree::ElementGeneratorNode<Tag>>, C> :
    public ygg::IndexViewBase<::tyr::planning::match_tree::ElementGeneratorNode<Tag>, C>
{
public:
    View(ygg::Index<::tyr::planning::match_tree::ElementGeneratorNode<Tag>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::planning::match_tree::ElementGeneratorNode<Tag>, C>(handle, context)
    {
    }

    auto get_elements() const noexcept { return ygg::make_view(this->get_data().elements, this->get_context().get_formalism_repository()); }
};
}

namespace ygg
{
template<typename Tag, ygg::formalism::SymbolContextFor<::tyr::planning::match_tree::VariableSelectorNode<Tag>> C>
class View<ygg::Index<::tyr::planning::match_tree::VariableSelectorNode<Tag>>, C> :
    public ygg::IndexViewBase<::tyr::planning::match_tree::VariableSelectorNode<Tag>, C>
{
public:
    View(ygg::Index<::tyr::planning::match_tree::VariableSelectorNode<Tag>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::planning::match_tree::VariableSelectorNode<Tag>, C>(handle, context)
    {
    }

    auto get_variable() const noexcept { return ygg::make_view(this->get_data().variable, this->get_context().get_formalism_repository()); }
    auto get_domain_children() const noexcept { return ygg::make_view(this->get_data().domain_children, this->get_context()); }
    auto get_dontcare_child() const noexcept { return ygg::make_view(this->get_data().dontcare_child, this->get_context()); }
};
}

namespace ygg
{
template<typename Tag, ygg::formalism::SymbolContextFor<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>> C>
class View<ygg::Index<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>, C> :
    public ygg::IndexViewBase<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>, C>
{
public:
    View(ygg::Index<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>, C>(handle, context)
    {
    }

    auto get_fact() const noexcept { return ygg::make_view(this->get_data().fact, this->get_context().get_formalism_repository()); }
    auto get_true_child() const noexcept { return ygg::make_view(this->get_data().true_child, this->get_context()); }
    auto get_dontcare_child() const noexcept { return ygg::make_view(this->get_data().dontcare_child, this->get_context()); }
};
}

#endif
