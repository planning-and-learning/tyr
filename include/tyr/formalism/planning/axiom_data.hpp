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

#ifndef TYR_FORMALISM_PLANNING_AXIOM_DATA_HPP_
#define TYR_FORMALISM_PLANNING_AXIOM_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/variable_index.hpp"

namespace ygg
{


template<>
struct Data<::tyr::formalism::planning::Axiom>
{
    ygg::Index<::tyr::formalism::planning::Axiom> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::planning::ConjunctiveCondition> body;
    ygg::Index<::tyr::formalism::planning::Atom<::tyr::formalism::DerivedTag>> head;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::planning::ConjunctiveCondition> body_,
         ygg::Index<::tyr::formalism::planning::Atom<::tyr::formalism::DerivedTag>> head_) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        head(head_)
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition>, C> body_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::Atom<::tyr::formalism::DerivedTag>>, C> head_) :
        index(),
        variables(),
        body(),
        head()
    {
        set(variables_, variables);
        set(body_, body);
        set(head_, head);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(variables);
        ygg::clear(body);
        ygg::clear(head);
    }

    auto cista_members() const noexcept { return std::tie(index, variables, body, head); }
    auto identifying_members() const noexcept { return std::tie(variables, body, head); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Axiom>);
}

#endif
