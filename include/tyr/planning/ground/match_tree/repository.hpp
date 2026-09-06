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

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
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
#include <optional>
#include <utility>
#include <yggdrasil/core/type_list.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::planning::match_tree
{

using GroundActionBuilder = ygg::ApplyTypeListT<ygg::formalism::BuilderStorage, RepositoryTypes<::tyr::formalism::planning::Action<::tyr::GroundTag>>>;
using GroundAxiomBuilder = ygg::ApplyTypeListT<ygg::formalism::BuilderStorage, RepositoryTypes<::tyr::formalism::planning::Axiom<::tyr::GroundTag>>>;

template<typename T>
[[nodiscard]] auto checkout(GroundActionBuilder& builder)
{
    auto data = builder.template get_builder<T>();
    data->clear();
    return data;
}

template<typename T>
[[nodiscard]] auto checkout(GroundAxiomBuilder& builder)
{
    auto data = builder.template get_builder<T>();
    data->clear();
    return data;
}

template<typename Tag>
class Repository
{
private:
    using SymbolRepository = ygg::ApplyTypeListT<::ygg::formalism::SymbolRepository, RepositoryTypes<Tag>>;

    const ::tyr::formalism::planning::Repository& m_formalism_repository;
    SymbolRepository m_repository;
    ygg::uint_t m_index;

public:
    explicit Repository(ygg::uint_t index, const ::tyr::formalism::planning::Repository& formalism_repository) :
        m_formalism_repository(formalism_repository),
        m_repository(),
        m_index(index)
    {
    }
    Repository(const Repository& other) = delete;
    Repository& operator=(const Repository& other) = delete;
    Repository(Repository&& other) = delete;
    Repository& operator=(Repository&& other) = delete;

    const auto& get_index() const noexcept { return m_index; }

    const ::tyr::formalism::planning::Repository& get_formalism_repository() const noexcept { return m_formalism_repository; }

    template<typename T>
    std::optional<ygg::View<ygg::Index<T>, Repository>> find(const ygg::Data<T>& builder) const noexcept
    {
        if (const auto view_or_nullopt = m_repository.find(builder))
            return ygg::make_view(view_or_nullopt->get_handle(), *this);

        return std::nullopt;
    }

    template<typename T>
    std::pair<ygg::View<ygg::Index<T>, Repository>, bool> get_or_create(ygg::Data<T>& builder)
    {
        const auto [view, success] = m_repository.get_or_create(builder);
        return std::make_pair(ygg::make_view(view.get_handle(), *this), success);
    }

    /// @brief Access the element with the given index.
    template<typename T>
    const ygg::Data<T>& operator[](ygg::Index<T> index) const noexcept
    {
        assert(index != ygg::Index<T>::max() && "Unassigned index.");
        return m_repository[index];
    }

    template<typename T>
    const ygg::Data<T>& front() const
    {
        return m_repository.template front<T>();
    }

    /// @brief Get the number of stored elements.
    template<typename T>
    size_t size() const noexcept
    {
        return m_repository.template size<T>();
    }

    /// @brief Clear the repository but keep memory allocated.
    void clear() noexcept { m_repository.clear(); }
};

static_assert(RepositoryConcept<Repository<::tyr::formalism::planning::Action<::tyr::GroundTag>>, ::tyr::formalism::planning::Action<::tyr::GroundTag>>);

static_assert(Context<Repository<::tyr::formalism::planning::Action<::tyr::GroundTag>>, ::tyr::formalism::planning::Action<::tyr::GroundTag>>);

template<typename Tag, typename T>
[[nodiscard]] auto get_or_create(Repository<Tag>& repository, ygg::Data<T>& data)
{
    canonicalize(data);
    return repository.get_or_create(data);
}

}

#endif
