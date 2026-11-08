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
#include "datalog/binding_utils.hpp"
#include "planning/binding_utils.hpp"
#include "pytyr/module.hpp"

#include <tyr/formalism/atom_data.hpp>
#include <tyr/formalism/atom_view.hpp>

/// Atoms: their indices and data are the same for datalog and planning, bound once here (and built from either's views);
/// their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F>
void bind_atom_data(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<Atom<T, F>>>(m, name + "Index");

    using V = urd::data<Atom<T, F>>;
    auto cls = nb::class_<V>(m, (name + "Data").c_str());
    if constexpr (std::same_as<T, LiftedTag>)
    {
        cls.def(urd::python::init_from_views<datalog::PredicateView<F>, const datalog::TermViewList&>(), "predicate"_a, "terms"_a);
        cls.def(urd::python::init_from_views<planning::PredicateView<F>, const planning::TermViewList&>(), "predicate"_a, "terms"_a);
        cls.def(nb::init<urd::index<Predicate<F>>, TermList>(), "predicate"_a, "terms"_a);
    }
    else
    {
        cls.def(urd::python::init_from_views<datalog::PredicateRowView<F>>(), "binding"_a);
        cls.def(urd::python::init_from_views<planning::PredicateRowView<F>>(), "binding"_a);
        cls.def(nb::init<ygg::RowIndex<Predicate<F>>>(), "binding"_a);
    }
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind T, FactKind F, typename C>
void bind_atom_view(nb::module_& m, nb::class_<C>& repository, const std::string& name)
{
    using V = urd::index_view<Atom<T, F>, C>;
    auto cls = nb::class_<V>(m, name.c_str());
    cls.def("index", &V::index);
    cls.def("get_predicate", &V::get_predicate);
    if constexpr (std::same_as<T, LiftedTag>)
    {
        cls.def("get_terms", &V::get_terms);
    }
    else
    {
        cls.def("get_row", &V::get_row);
        cls.def("get_objects", &V::get_objects);
    }
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);

    ygg::bind_insert<Atom<T, F>>(repository);
}
}  // namespace

void bind_atom(nb::module_& m)
{
    bind_atom_data<LiftedTag, StaticTag>(m, "StaticAtom");
    bind_atom_data<LiftedTag, FluentTag>(m, "FluentAtom");
    bind_atom_data<LiftedTag, DerivedTag>(m, "DerivedAtom");
    bind_atom_data<GroundTag, StaticTag>(m, "StaticGroundAtom");
    bind_atom_data<GroundTag, FluentTag>(m, "FluentGroundAtom");
    bind_atom_data<GroundTag, DerivedTag>(m, "DerivedGroundAtom");
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_atom(nb::module_& m, RepositoryBinding& repository)
{
    bind_atom_view<LiftedTag, StaticTag>(m, repository, "StaticAtom");
    bind_atom_view<LiftedTag, FluentTag>(m, repository, "FluentAtom");
    bind_atom_view<GroundTag, StaticTag>(m, repository, "StaticGroundAtom");
    bind_atom_view<GroundTag, FluentTag>(m, repository, "FluentGroundAtom");
}

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_atom(nb::module_& m, RepositoryBinding& repository)
{
    bind_atom_view<LiftedTag, StaticTag>(m, repository, "StaticAtom");
    bind_atom_view<LiftedTag, FluentTag>(m, repository, "FluentAtom");
    bind_atom_view<LiftedTag, DerivedTag>(m, repository, "DerivedAtom");
    bind_atom_view<GroundTag, StaticTag>(m, repository, "StaticGroundAtom");
    bind_atom_view<GroundTag, FluentTag>(m, repository, "FluentGroundAtom");
    bind_atom_view<GroundTag, DerivedTag>(m, repository, "DerivedGroundAtom");
}

}  // namespace tyr::formalism::planning
