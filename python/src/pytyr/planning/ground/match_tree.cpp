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

#include "module.hpp"

#include <tyr/formalism/planning/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/planning/ground/match_tree/nodes/atom_data.hpp>
#include <tyr/planning/ground/match_tree/nodes/constraint_data.hpp>
#include <tyr/planning/ground/match_tree/nodes/generator_data.hpp>
#include <tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp>
#include <tyr/planning/ground/match_tree/nodes/variable_data.hpp>
#include <tyr/planning/ground/match_tree/repository.hpp>
#include <urd/format.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

namespace
{
/// The node records of the match trees over elements Tag (ground actions or axioms).
template<typename Tag>
void bind_match_tree_nodes(nb::module_& m)
{
    urd::python::bind_index<match_tree::AtomSelectorNode<Tag>>(m);
    urd::python::bind_data<match_tree::AtomSelectorNode<Tag>>(m);
    urd::python::bind_builder<match_tree::AtomSelectorNode<Tag>, match_tree::Repository<Tag>>(m);
    urd::python::bind_view<urd::index_view<match_tree::AtomSelectorNode<Tag>, match_tree::Repository<Tag>>>(m);
    urd::python::bind_index<match_tree::VariableSelectorNode<Tag>>(m);
    urd::python::bind_data<match_tree::VariableSelectorNode<Tag>>(m);
    urd::python::bind_builder<match_tree::VariableSelectorNode<Tag>, match_tree::Repository<Tag>>(m);
    urd::python::bind_view<urd::index_view<match_tree::VariableSelectorNode<Tag>, match_tree::Repository<Tag>>>(m);
    urd::python::bind_index<match_tree::NegativeFactSelectorNode<Tag>>(m);
    urd::python::bind_data<match_tree::NegativeFactSelectorNode<Tag>>(m);
    urd::python::bind_builder<match_tree::NegativeFactSelectorNode<Tag>, match_tree::Repository<Tag>>(m);
    urd::python::bind_view<urd::index_view<match_tree::NegativeFactSelectorNode<Tag>, match_tree::Repository<Tag>>>(m);
    urd::python::bind_index<match_tree::NumericConstraintSelectorNode<Tag>>(m);
    urd::python::bind_data<match_tree::NumericConstraintSelectorNode<Tag>>(m);
    urd::python::bind_builder<match_tree::NumericConstraintSelectorNode<Tag>, match_tree::Repository<Tag>>(m);
    urd::python::bind_view<urd::index_view<match_tree::NumericConstraintSelectorNode<Tag>, match_tree::Repository<Tag>>>(m);
    urd::python::bind_index<match_tree::ElementGeneratorNode<Tag>>(m);
    urd::python::bind_data<match_tree::ElementGeneratorNode<Tag>>(m);
    urd::python::bind_builder<match_tree::ElementGeneratorNode<Tag>, match_tree::Repository<Tag>>(m);
    urd::python::bind_view<urd::index_view<match_tree::ElementGeneratorNode<Tag>, match_tree::Repository<Tag>>>(m);
}
}  // namespace

void bind_match_tree(nb::module_& m)
{
    bind_match_tree_nodes<formalism::Action<GroundTag>>(m);
    bind_match_tree_nodes<formalism::Axiom<GroundTag>>(m);
}

}  // namespace tyr::planning
