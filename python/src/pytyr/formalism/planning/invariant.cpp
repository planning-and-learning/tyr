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
#include "binding_utils.hpp"
#include "bindings.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <tyr/formalism/planning/invariant_data.hpp>
#include <tyr/formalism/planning/invariant_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
void bind_invariant_data(nb::module_& m, const char* name)
{
    using namespace nb::literals;
    using V = ygg::Data<Invariant>;
    auto cls = nb::class_<V>(m, name).def(nb::init<ygg::uint_t, ygg::uint_t, const AtomViewList<LiftedTag, FluentTag>>(),
                                          "num_rigid_variables"_a,
                                          "num_counted_variables"_a,
                                          "atoms"_a);
    cls.def(nb::init<ygg::uint_t, ygg::uint_t, ygg::IndexList<Atom<LiftedTag, FluentTag>>>(), "num_rigid_variables"_a, "num_counted_variables"_a, "atoms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_invariant_view(nb::module_& m, const char* name)
{
    using V = InvariantView;
    auto cls = nb::class_<V>(m, name)
                   .def("get_index", &V::get_index)
                   .def("get_num_rigid_variables", &V::get_num_rigid_variables)
                   .def("get_num_counted_variables", &V::get_num_counted_variables)
                   .def("get_atoms", &V::get_atoms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_invariant(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<Invariant>>(m, "InvariantIndex");

    bind_invariant_data(m, "InvariantData");

    bind_invariant_view(m, "Invariant");

    bind_insert<Invariant>(repository);
}

}  // namespace tyr::formalism::planning
