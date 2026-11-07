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

#include <tyr/formalism/literal_data.hpp>
#include <tyr/formalism/literal_view.hpp>

/// Literals: their indices and data are the same for datalog and planning, bound once here (and built from either's
/// views); their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F>
void bind_literal_data(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<Literal<T, F>>>(m, name + "Index");

    using V = urd::data<Literal<T, F>>;
    auto cls = nb::class_<V>(m, (name + "Data").c_str());
    cls.def(urd::python::init_from_views<datalog::AtomView<T, F>, bool>(), "atom"_a, "polarity"_a);
    cls.def(urd::python::init_from_views<planning::AtomView<T, F>, bool>(), "atom"_a, "polarity"_a);
    cls.def(nb::init<urd::index<Atom<T, F>>, bool>(), "atom"_a, "polarity"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind T, FactKind F, typename C>
void bind_literal_view(nb::module_& m, nb::class_<C>& repository, const std::string& name)
{
    using V = urd::index_view<Literal<T, F>, C>;
    auto cls = nb::class_<V>(m, name.c_str());
    cls.def("index", &V::index);
    cls.def("get_atom", &V::get_atom);
    cls.def("get_polarity", &V::get_polarity);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);

    ygg::bind_insert<Literal<T, F>>(repository);
}
}  // namespace

void bind_literal(nb::module_& m)
{
    bind_literal_data<LiftedTag, StaticTag>(m, "StaticLiteral");
    bind_literal_data<LiftedTag, FluentTag>(m, "FluentLiteral");
    bind_literal_data<LiftedTag, DerivedTag>(m, "DerivedLiteral");
    bind_literal_data<GroundTag, StaticTag>(m, "StaticGroundLiteral");
    bind_literal_data<GroundTag, FluentTag>(m, "FluentGroundLiteral");
    bind_literal_data<GroundTag, DerivedTag>(m, "DerivedGroundLiteral");
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_literal(nb::module_& m, RepositoryBinding& repository)
{
    bind_literal_view<LiftedTag, StaticTag>(m, repository, "StaticLiteral");
    bind_literal_view<LiftedTag, FluentTag>(m, repository, "FluentLiteral");
    bind_literal_view<GroundTag, StaticTag>(m, repository, "StaticGroundLiteral");
    bind_literal_view<GroundTag, FluentTag>(m, repository, "FluentGroundLiteral");
}

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_literal(nb::module_& m, RepositoryBinding& repository)
{
    bind_literal_view<LiftedTag, StaticTag>(m, repository, "StaticLiteral");
    bind_literal_view<LiftedTag, FluentTag>(m, repository, "FluentLiteral");
    bind_literal_view<LiftedTag, DerivedTag>(m, repository, "DerivedLiteral");
    bind_literal_view<GroundTag, StaticTag>(m, repository, "StaticGroundLiteral");
    bind_literal_view<GroundTag, FluentTag>(m, repository, "FluentGroundLiteral");
    bind_literal_view<GroundTag, DerivedTag>(m, repository, "DerivedGroundLiteral");
}

}  // namespace tyr::formalism::planning
