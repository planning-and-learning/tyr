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

#ifndef TYR_PLANNING_STATIC_STATE_HPP_
#define TYR_PLANNING_STATIC_STATE_HPP_

#include "tyr/formalism/atom_view.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/function_term_view.hpp"

#include <functional>
#include <limits>
#include <ranges>
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

class StaticState
{
public:
    StaticState(formalism::planning::AtomListView<GroundTag, formalism::StaticTag> atoms,
                formalism::planning::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values);

    template<TaskKind Kind>
    explicit StaticState(formalism::planning::TaskView<Kind> task);

    [[nodiscard]] bool test(urd::index<formalism::Atom<GroundTag, formalism::StaticTag>> index) const
    { return m_atoms.contains(index); }
    [[nodiscard]] ygg::float_t get(urd::index<formalism::FunctionTerm<GroundTag, formalism::StaticTag>> index) const noexcept
    { return m_numeric_variables.visit(index, std::identity {}, std::numeric_limits<ygg::float_t>::quiet_NaN()); }

    [[nodiscard]] bool test(formalism::planning::AtomView<GroundTag, formalism::StaticTag> atom) const { return test(atom.index()); }
    [[nodiscard]] ygg::float_t get(formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> fterm) const noexcept
    { return get(fterm.index()); }

    /// In ascending order.
    [[nodiscard]] auto get_atoms() const noexcept { return std::views::all(m_atoms); }
    [[nodiscard]] auto get_fterm_values() const noexcept
    {
        return urd::transform(m_numeric_variables, [](const auto& entry) { return std::pair(entry.first, entry.second); });
    }

private:
    urd::Store<urd::set<urd::index<formalism::Atom<GroundTag, formalism::StaticTag>>>, urd::dense, urd::sorted> m_atoms;
    urd::Store<urd::map<urd::index<formalism::FunctionTerm<GroundTag, formalism::StaticTag>>, ygg::float_t>, urd::dense> m_numeric_variables;
};

}

#endif
