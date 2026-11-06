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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_NEGATIVE_FACT_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_NEGATIVE_FACT_DATA_HPP_

#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"

#include <optional>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <urd/elements/flat.hpp>

namespace urd
{
template<typename Tag>
struct data<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>
{
    urd::index<::tyr::planning::match_tree::NegativeFactSelectorNode<Tag>> index;
    urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact;
    ::cista::optional<::tyr::planning::match_tree::Node<Tag>> true_child;
    ::cista::optional<::tyr::planning::match_tree::Node<Tag>> dontcare_child;

    data() = default;
    data(urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact_,
         ::cista::optional<::tyr::planning::match_tree::Node<Tag>> true_child_,
         ::cista::optional<::tyr::planning::match_tree::Node<Tag>> dontcare_child_) :
        index(),
        fact(fact_),
        true_child(std::move(true_child_)),
        dontcare_child(std::move(dontcare_child_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::fact, &data::true_child, &data::dontcare_child> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::fact, &data::true_child, &data::dontcare_child> {}; }
};
}

#endif
