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

#ifndef TYR_FORMALISM_PLANNING_GROUND_AXIOM_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_AXIOM_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/ground_atom_index.hpp"
#include "tyr/formalism/planning/ground_axiom_index.hpp"
#include "tyr/formalism/planning/ground_conjunctive_condition_index.hpp"

namespace ygg
{


template<>
struct Data<::tyr::formalism::planning::GroundAxiom>
{
    ygg::Index<::tyr::formalism::planning::GroundAxiom> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Axiom>> binding;
    ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> body;
    ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> head;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Axiom>> binding_,
         ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> body_,
         ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> head_) :
        index(),
        binding(binding_),
        body(body_),
        head(head_)
    {
    }
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Axiom>>, C> binding_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition>, C> body_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>>, C> head_) :
        index(),
        binding(),
        body(),
        head()
    {
        set(binding_, binding);
        set(body_, body);
        set(head_, head);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(binding);
        ygg::clear(body);
        ygg::clear(head);
    }

    auto cista_members() const noexcept { return std::tie(index, binding, body, head); }
    auto identifying_members() const noexcept { return std::tie(binding); }
};

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundAxiom>);
}

#endif
