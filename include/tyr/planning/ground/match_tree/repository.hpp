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

#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/ground/match_tree/canonicalization.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_view.hpp"

#include <cassert>
#include <utility>
#include <yggdrasil/core/type_list.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/interning.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::planning::match_tree
{

using GroundActionBuilder = ygg::ApplyTypeListT<ygg::formalism::BuilderStorage, RepositoryTypes<formalism::planning::Action<GroundTag>>>;
using GroundAxiomBuilder = ygg::ApplyTypeListT<ygg::formalism::BuilderStorage, RepositoryTypes<formalism::planning::Axiom<GroundTag>>>;

using ygg::formalism::checkout;
using ygg::formalism::insert;

template<typename Tag>
class Repository : public ygg::formalism::SymbolRepositoryBase<Repository<Tag>, RepositoryTypes<Tag>>
{
    using Base = ygg::formalism::SymbolRepositoryBase<Repository<Tag>, RepositoryTypes<Tag>>;

private:
    const formalism::planning::Repository& m_formalism_repository;
    ygg::uint_t m_index;

public:
    explicit Repository(ygg::uint_t index, const formalism::planning::Repository& formalism_repository) :
        m_formalism_repository(formalism_repository),
        m_index(index)
    {
    }
    Repository(const Repository& other) = delete;
    Repository& operator=(const Repository& other) = delete;
    Repository(Repository&& other) = delete;
    Repository& operator=(Repository&& other) = delete;

    const auto& get_index() const noexcept { return m_index; }

    const formalism::planning::Repository& get_formalism_repository() const noexcept { return m_formalism_repository; }

    /// @brief Access the element with the given index.
    template<typename T>
        requires ygg::formalism::SupportsSymbol<Repository, T>
    const ygg::Data<T>& operator[](ygg::Index<T> index) const noexcept
    {
        assert(index != ygg::Index<T>::max() && "Unassigned index.");
        return this->template at_local<T>(index);
    }

    template<typename T>
        requires ygg::formalism::SupportsSymbol<Repository, T>
    const Repository& get_canonical_context(ygg::Index<T>) const noexcept
    {
        return *this;
    }
};

static_assert(RepositoryConcept<Repository<formalism::planning::Action<GroundTag>>, formalism::planning::Action<GroundTag>>);

static_assert(Context<Repository<formalism::planning::Action<GroundTag>>, formalism::planning::Action<GroundTag>>);

template<typename Tag, typename T>
    requires ygg::formalism::SupportsSymbol<Repository<Tag>, T>
void prepare_for_insert(Repository<Tag>&, ygg::Data<T>& data)
{
    canonicalize(data);
}

}

#endif
