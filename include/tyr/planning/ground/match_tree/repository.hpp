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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_REPOSITORY_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_REPOSITORY_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_builder.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_builder.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_builder.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_builder.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_builder.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"

#include <boost/mp11.hpp>
#include <memory>
#include <urd/elements/flat.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/interning.hpp>
#include <yggdrasil/formalism/nested.hpp>
#include <yggdrasil/formalism/symbol_context.hpp>

namespace tyr::planning::match_tree
{

template<typename Tag>
class Repository;

/// The pools of a match tree's builders and scratch records, interned into its Repository.
template<typename Tag>
using BuilderOf = boost::mp11::mp_apply<ygg::formalism::BuilderStorage, boost::mp11::mp_push_front<RepositoryTypes<Tag>, Repository<Tag>>>;
using GroundActionBuilder = BuilderOf<formalism::Action<GroundTag>>;
using GroundAxiomBuilder = BuilderOf<formalism::Axiom<GroundTag>>;

using ygg::formalism::checkout;

/// A match tree's nodes, over the planning repository holding the formalism elements they refer to (its atoms,
/// constraints, facts and variables: nested, read only).
template<typename Tag>
class Repository :
    public ygg::formalism::SymbolContext<Repository<Tag>, RepositoryTypes<Tag>>,
    public ygg::formalism::Nests<Repository<Tag>, ygg::formalism::Nested<const formalism::Repository>>
{
private:
    using Base = ygg::formalism::SymbolContext<Repository<Tag>, RepositoryTypes<Tag>>;
    using NestsBase = ygg::formalism::Nests<Repository<Tag>, ygg::formalism::Nested<const formalism::Repository>>;

    ygg::uint_t m_index;

public:
    explicit Repository(ygg::uint_t index, std::shared_ptr<const formalism::Repository> formalism_repository) :
        Base(),
        NestsBase(std::move(formalism_repository)),
        m_index(index)
    {
    }

    Repository(const Repository& other) = delete;
    Repository& operator=(const Repository& other) = delete;
    Repository(Repository&& other) = delete;
    Repository& operator=(Repository&& other) = delete;
    ~Repository() = default;

    /// The repository's number, as urd's views ask of a context.
    [[nodiscard]] std::size_t layer() const noexcept { return m_index; }

    using Base::find;
    using Base::insert;
    using Base::view;
    using NestsBase::get;
};

}

#endif
