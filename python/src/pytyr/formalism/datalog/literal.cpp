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

#include <tyr/formalism/datalog/literal_data.hpp>
#include <tyr/formalism/datalog/literal_index.hpp>
#include <tyr/formalism/datalog/literal_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<TaskKind T, FactKind F>
void bind_literal_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = Literal<T, F>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        cls.def(nb::init<AtomView<T, F>, bool>(), "atom"_a, "polarity"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = LiteralView<T, F>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        cls.def("get_atom", &V::get_atom, nb::keep_alive<0, 1>());
        cls.def("get_polarity", &V::get_polarity);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_literal(nb::module_& m, RepositoryBinding& repository)
{
    bind_literal_kind<LiftedTag, StaticTag>(m, repository, "StaticLiteral");
    bind_literal_kind<LiftedTag, FluentTag>(m, repository, "FluentLiteral");
    bind_literal_kind<GroundTag, StaticTag>(m, repository, "StaticGroundLiteral");
    bind_literal_kind<GroundTag, FluentTag>(m, repository, "FluentGroundLiteral");
}

}  // namespace tyr::formalism::datalog
