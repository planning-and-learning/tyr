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

#ifndef TYR_FORMALISM_FDR_CONTEXT_HPP_
#define TYR_FORMALISM_FDR_CONTEXT_HPP_

#include <yggdrasil/buffer/declarations.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/containers/associative_containers.hpp>
#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <vector>

namespace tyr::formalism::planning
{

class FDRContext
{
public:
    // Construct without mutexes.
    explicit FDRContext(RepositoryPtr context);

    // Construct with ground mutexes.
    FDRContext(const std::vector<GroundAtomViewList<FluentTag>>& mutexes, RepositoryPtr context);

    // Construct with binary ground mutexes.
    FDRContext(const GroundAtomViewList<FluentTag>& all_atoms, RepositoryPtr context);

    // Copy the FDRContext.
    FDRContext(const FDRContext& other, Builder& builder, RepositoryPtr context);

    FDRFactView<FluentTag> get_fact(GroundAtomView<FluentTag> atom);

    std::optional<FDRFactView<FluentTag>> get_fact(GroundAtomView<FluentTag> atom) const;

    const FDRVariableViewList<FluentTag>& get_variables() const noexcept;

private:
    RepositoryPtr m_context;
    ygg::Data<FDRVariable<FluentTag>> m_builder;
    FDRVariableViewList<FluentTag> m_variables;
    ygg::UnorderedMap<GroundAtomView<FluentTag>, FDRFactView<FluentTag>> m_mapping;
};

}

#endif
