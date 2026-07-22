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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_LITERAL_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_LITERAL_HPP_

#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace tyr::formalism::planning
{
template<FactKind T>
struct MutableLiteral : ygg::comparison::Mixin<MutableLiteral<T>>
{
    MutableAtom<T> atom;
    bool polarity;

    MutableLiteral() = default;
    MutableLiteral(MutableAtom<T> atom, bool polarity) : atom(std::move(atom)), polarity(polarity) {}
    MutableLiteral(LiteralView<T> element) : atom(element.get_atom()), polarity(element.get_polarity()) {}

    auto identifying_members() const noexcept { return std::tie(atom, polarity); }
};

template<FactKind T>
using MutableLiteralList = std::vector<MutableLiteral<T>>;
}

namespace tyr::formalism::unification
{
template<FactKind T>
struct structure_traits<tyr::formalism::planning::MutableLiteral<T>>
{
    using Type = tyr::formalism::planning::MutableLiteral<T>;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F&& f)
    {
        if (lhs.polarity != rhs.polarity)
            return false;

        return tyr::formalism::unification::zip_terms(lhs.atom, rhs.atom, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F&& f)
    {
        return Type(tyr::formalism::unification::transform_terms(value.atom, f), value.polarity);
    }
};
}

#endif
