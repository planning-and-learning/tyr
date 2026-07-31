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

#include <tyr/formalism/datalog/atom_data.hpp>
#include <tyr/formalism/datalog/atom_index.hpp>
#include <tyr/formalism/datalog/atom_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<FactKind T>
void bind_atom_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<Atom<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<PredicateView<T>, const TermViewList&>(), "predicate"_a, "terms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_atom_view(nb::module_& m, const char* name)
{
    using V = AtomView<T>;
    auto cls =
        nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_predicate", &V::get_predicate, nb::keep_alive<0, 1>()).def("get_terms", &V::get_terms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_atom(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<Atom<StaticTag>>>(m, "StaticAtomIndex");
    ygg::bind_index<ygg::Index<Atom<FluentTag>>>(m, "FluentAtomIndex");

    bind_atom_data<StaticTag>(m, "StaticAtomData");
    bind_atom_data<FluentTag>(m, "FluentAtomData");

    bind_atom_view<StaticTag>(m, "StaticAtom");
    bind_atom_view<FluentTag>(m, "FluentAtom");

    repository.def("get_or_create", &get_or_create_data<Atom<StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<Atom<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
