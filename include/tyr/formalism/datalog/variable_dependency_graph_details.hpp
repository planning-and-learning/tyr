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

#ifndef TYR_FORMALISM_DATALOG_VARIABLE_DEPENDENCY_GRAPH_DETAILS_HPP_
#define TYR_FORMALISM_DATALOG_VARIABLE_DEPENDENCY_GRAPH_DETAILS_HPP_

#include "tyr/formalism/declarations.hpp"

#include <cstddef>
#include <type_traits>
#include <urd/graph_types.hpp>
#include <urd/repr/graphs/bit_matrix.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace tyr::formalism::datalog::details
{

/// The literal dependency set of dep (unary or binary) for fact kind F and polarity P.
template<FactKind F, PolarityKind P, typename Dep>
auto& select_literal_dependency(Dep& dep) noexcept
{
    using D = std::remove_const_t<Dep>;
    using namespace boost::mp11;
    return tagged_member<mp_product<mp_list, StaticFluentTags, PositiveNegativeTags>,
                         mp_list<F, P>,
                         &D::static_positive_literal,
                         &D::static_negative_literal,
                         &D::fluent_positive_literal,
                         &D::fluent_negative_literal>(dep);
}

struct UnaryDependencies
{
    explicit UnaryDependencies(ygg::uint_t k_) : k(k_) {}

    /**
     * get_
     */

    template<FactKind F, PolarityKind P>
    [[nodiscard]] const auto& get_literal_dependency() const noexcept
    { return details::select_literal_dependency<F, P>(*this); }

    [[nodiscard]] const auto& get_numeric_dependency() const noexcept { return numeric_constraint; }

    /**
     * has_
     */

    template<FactKind F, PolarityKind P>
    [[nodiscard]] bool has_literal_dependency(ygg::uint_t p) const noexcept
    {
        assert(p < k);
        return get_literal_dependency<F, P>().contains(ParameterIndex { p });
    }

    [[nodiscard]] bool has_numeric_dependency(ygg::uint_t p) const noexcept
    {
        assert(p < k);
        return numeric_constraint.contains(ParameterIndex { p });
    }

    [[nodiscard]] bool has_dependency(ygg::uint_t p) const noexcept
    {
        return has_literal_dependency<StaticTag, PositiveTag>(p) || has_literal_dependency<StaticTag, NegativeTag>(p)
               || has_literal_dependency<FluentTag, PositiveTag>(p) || has_literal_dependency<FluentTag, NegativeTag>(p) || has_numeric_dependency(p);
    }

    ygg::uint_t k;

    urd::Store<urd::set<ParameterIndex>, urd::dense> static_positive_literal;
    urd::Store<urd::set<ParameterIndex>, urd::dense> static_negative_literal;
    urd::Store<urd::set<ParameterIndex>, urd::dense> fluent_positive_literal;
    urd::Store<urd::set<ParameterIndex>, urd::dense> fluent_negative_literal;
    urd::Store<urd::set<ParameterIndex>, urd::dense> numeric_constraint;
};

struct BinaryDependencies
{
    /// The k parameters of a condition, adjacent where they occur together in a literal (or constraint) of one kind.
    using Pairs = urd::Store<urd::graph<ParameterIndex>, urd::monotone, urd::undirected, urd::constant_adjacency>;

    /// Every member starts as the k parameters without edges: the first is built so, the others are copies of it.
    explicit BinaryDependencies(ygg::uint_t k_) :
        k(k_),
        static_positive_literal(urd::graph_data<std::pair<ParameterIndex, ParameterIndex>> { k_, {}, {} }),
        static_negative_literal(static_positive_literal),
        fluent_positive_literal(static_positive_literal),
        fluent_negative_literal(static_positive_literal),
        numeric_constraint(static_positive_literal)
    {
    }

    /**
     * get_
     */

    template<FactKind F, PolarityKind P>
    [[nodiscard]] const auto& get_literal_dependency() const noexcept
    { return details::select_literal_dependency<F, P>(*this); }

    [[nodiscard]] const auto& get_numeric_dependency() const noexcept { return numeric_constraint; }

    /**
     * has_
     */

    template<FactKind F, PolarityKind P>
    [[nodiscard]] bool has_literal_dependency(ygg::uint_t pi, ygg::uint_t pj) const noexcept
    { return get_literal_dependency<F, P>().adjacent(ParameterIndex { pi }, ParameterIndex { pj }); }

    [[nodiscard]] bool has_numeric_dependency(ygg::uint_t pi, ygg::uint_t pj) const noexcept
    { return numeric_constraint.adjacent(ParameterIndex { pi }, ParameterIndex { pj }); }

    [[nodiscard]] bool has_dependency(ygg::uint_t pi, ygg::uint_t pj) const noexcept
    {
        return has_literal_dependency<StaticTag, PositiveTag>(pi, pj) || has_literal_dependency<StaticTag, NegativeTag>(pi, pj)
               || has_literal_dependency<FluentTag, PositiveTag>(pi, pj) || has_literal_dependency<FluentTag, NegativeTag>(pi, pj)
               || has_numeric_dependency(pi, pj);
    }

    ygg::uint_t k;

    Pairs static_positive_literal;
    Pairs static_negative_literal;
    Pairs fluent_positive_literal;
    Pairs fluent_negative_literal;
    Pairs numeric_constraint;
};
}

#endif
