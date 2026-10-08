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

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/planning/state_storage/iterators.hpp"

#include <boost/dynamic_bitset.hpp>
#include <limits>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>

namespace tyr::planning
{

class StaticState
{
public:
    StaticState(formalism::planning::AtomListView<GroundTag, formalism::StaticTag> atoms,
                formalism::planning::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values);

    template<TaskKind Kind>
    explicit StaticState(formalism::planning::TaskView<Kind> task);

    bool test(ygg::Index<formalism::planning::Atom<GroundTag, formalism::StaticTag>> index) const
    {
        return ygg::test(ygg::uint_t(index), m_atoms);
    }
    ygg::float_t get(ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::StaticTag>> index) const noexcept
    {
        return ygg::get(ygg::uint_t(index), m_numeric_variables, std::numeric_limits<ygg::float_t>::quiet_NaN());
    }

    bool test(formalism::planning::AtomView<GroundTag, formalism::StaticTag> atom) const { return test(atom.get_index()); }
    ygg::float_t get(formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> fterm) const noexcept { return get(fterm.get_index()); }

    auto get_atoms() const noexcept { return AtomRange<formalism::StaticTag>(m_atoms); }
    auto get_fterm_values() const noexcept { return FunctionTermValueRange<formalism::StaticTag>(m_numeric_variables); }

private:
    boost::dynamic_bitset<> m_atoms;
    std::vector<ygg::float_t> m_numeric_variables;
};

}

#endif
