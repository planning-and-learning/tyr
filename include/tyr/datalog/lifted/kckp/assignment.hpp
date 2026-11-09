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

#ifndef TYR_DATALOG_ASSIGNMENT_HPP_
#define TYR_DATALOG_ASSIGNMENT_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"

#include <limits>
#include <yggdrasil/core/config.hpp>
#include <urd/elements/flat.hpp>

namespace tyr::datalog
{

/**
 * EmptyAssignment
 */

struct EmptyAssignment
{
    static constexpr ygg::uint_t rank = 0;
};

/**
 * VertexAssignment
 */

struct VertexAssignment
{
    formalism::ParameterIndex index;
    urd::index<formalism::Object> object;

    VertexAssignment() : index(), object() {}

    VertexAssignment(formalism::ParameterIndex index_, urd::index<formalism::Object> object_) : index(index_), object(object_) {}

    /// Assigned: an object is set (a default assignment has none).
    [[nodiscard]] inline bool is_valid() const noexcept { return object != urd::index<formalism::Object>(); }
};

/**
 * EdgeAssignment
 */

/// @brief Encapsulate assignment of objects to variables of atoms.
struct EdgeAssignment
{
    formalism::ParameterIndex first_index;
    urd::index<formalism::Object> first_object;
    formalism::ParameterIndex second_index;
    urd::index<formalism::Object> second_object;

    EdgeAssignment() : first_index(), first_object(), second_index(), second_object() {}

    EdgeAssignment(formalism::ParameterIndex first_index_,
                   urd::index<formalism::Object> first_object_,
                   formalism::ParameterIndex second_index_,
                   urd::index<formalism::Object> second_object_) :
        first_index(first_index_),
        first_object(first_object_),
        second_index(second_index_),
        second_object(second_object_)
    {
    }

    [[nodiscard]] inline bool is_valid() const noexcept
    {
        return (first_index < second_index) && (first_object != urd::index<formalism::Object>()) && (second_object != urd::index<formalism::Object>());
    }
};
}

#endif