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

#ifndef TYR_FORMALISM_UNIFICATION_STRUCTURE_TRAITS_HPP_
#define TYR_FORMALISM_UNIFICATION_STRUCTURE_TRAITS_HPP_

#include "tyr/formalism/term_data.hpp"

#include <concepts>
#include <utility>

namespace tyr::formalism::unification
{

template<typename T>
struct structure_traits;

/// @brief Base case for unification: structures that are directly represented by terms.
template<>
struct structure_traits<ygg::Data<Term>>
{
    template<typename F>
    static bool zip_terms(const ygg::Data<Term>& lhs, const ygg::Data<Term>& rhs, F&& f)
    { return f(lhs, rhs); }

    template<typename F>
    static ygg::Data<Term> transform_terms(const ygg::Data<Term>& value, F&& f)
    { return f(value); }
};

/// @brief Free helper function to zip terms and template argument deduction.
template<typename T, typename F>
bool zip_terms(const T& lhs, const T& rhs, F&& f);

/// @brief Free helper function to transform terms and template argument deduction.
template<typename T, typename F>
T transform_terms(const T& value, F&& f);

namespace detail
{
struct ZipProbe
{
    bool operator()(const ygg::Data<Term>&, const ygg::Data<Term>&) const { return true; }
};

struct TransformProbe
{
    ygg::Data<Term> operator()(const ygg::Data<Term>& term) const { return term; }
};
}  // namespace detail

template<typename T>
concept TermUnifiableStructure = requires(const T& lhs, const T& rhs) {
    { structure_traits<T>::zip_terms(lhs, rhs, detail::ZipProbe {}) } -> std::same_as<bool>;
    { structure_traits<T>::transform_terms(lhs, detail::TransformProbe {}) } -> std::same_as<T>;
};

}  // namespace tyr::formalism::unification

#endif