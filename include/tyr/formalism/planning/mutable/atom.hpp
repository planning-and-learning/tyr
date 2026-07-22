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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_ATOM_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_ATOM_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{
template<FactKind T>
struct MutableAtom : ygg::comparison::Mixin<MutableAtom<T>>
{
    PredicateView<T> predicate;
    std::vector<ygg::Data<Term>> terms;

    MutableAtom() = default;
    MutableAtom(PredicateView<T> predicate_, std::vector<ygg::Data<Term>> terms_) : predicate(predicate_), terms(std::move(terms_)) {}
    MutableAtom(PredicateView<T> predicate_, const TermViewList& terms_) : predicate(predicate_), terms()
    {
        for (const auto& term : terms_)
            terms.push_back(term.get_data());
    }
    MutableAtom(AtomView<T> element_) : predicate(element_.get_predicate()), terms()
    {
        for (const auto& term : element_.get_terms())
            terms.push_back(term.get_data());
    }
    MutableAtom(GroundAtomView<T> element) : predicate(element.get_predicate()), terms()
    {
        for (const auto& object : element.get_objects())
            terms.push_back(ygg::Data<Term>(object.get_index()));
    }

    auto identifying_members() const noexcept { return std::tie(predicate, terms); }
};

template<FactKind T>
using MutableAtomList = std::vector<MutableAtom<T>>;
}

namespace tyr::formalism::unification
{
template<FactKind T>
struct structure_traits<tyr::formalism::planning::MutableAtom<T>>
{
    using Type = tyr::formalism::planning::MutableAtom<T>;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F&& f)
    {
        if (lhs.predicate != rhs.predicate)
            return false;

        if (lhs.terms.size() != rhs.terms.size())
            return false;

        return tyr::formalism::unification::zip_terms(lhs.terms, rhs.terms, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F&& f)
    {
        return Type(value.predicate, tyr::formalism::unification::transform_terms(value.terms, f));
    }
};

}

#endif
