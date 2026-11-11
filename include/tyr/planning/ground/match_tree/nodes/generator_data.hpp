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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_DATA_HPP_

#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"

#include <optional>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace urd
{
template<typename Tag>
struct data<::tyr::planning::match_tree::ElementGeneratorNode<Tag>>
{
    urd::index<::tyr::planning::match_tree::ElementGeneratorNode<Tag>> index;
    urd::flat_vector<urd::index<Tag>> elements;

    data() = default;
    data(urd::flat_vector<urd::index<Tag>> elements_) : index(), elements(std::move(elements_)) {}
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::elements> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::elements> {}; }
};
}

#endif
