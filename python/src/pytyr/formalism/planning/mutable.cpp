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

#include "mutable.hpp"

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<FactKind T>
void bind_atom(nb::module_& m, const std::string& name)
{
    using V = MutableAtom<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<PredicateView<T>, const TermViewList&>(), "predicate"_a, "terms"_a)
                   .def(nb::init<AtomView<T>>(), "atom"_a)
                   .def_rw("predicate", &V::predicate)
                   .def_rw("terms", &V::terms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_literal(nb::module_& m, const std::string& name)
{
    using V = MutableLiteral<T>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<MutableAtom<T>, bool>(), "atom"_a, "polarity"_a)
                   .def(nb::init<LiteralView<T>>(), "literal"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_condition(nb::module_& m, const std::string& name)
{
    using V = MutableConjunctiveCondition;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<size_t, ConjunctiveConditionView>(), "num_parent_variables"_a, "condition"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_effect(nb::module_& m, const std::string& name)
{
    using V = MutableConjunctiveEffect;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<size_t, size_t, ConjunctiveEffectView>(), "num_parent_variables"_a, "num_variables"_a, "effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conditional_effect(nb::module_& m, const std::string& name)
{
    using V = MutableConditionalEffect;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<size_t, ConditionalEffectView>(), "num_parent_variables"_a, "effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_action(nb::module_& m, const std::string& name)
{
    using V = MutableAction;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<ActionView>(), "action"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}

/**
 * bind_mutable
 */

void bind_mutable(nb::module_& m)
{
    bind_atom<StaticTag>(m, "MutableStaticAtom");
    bind_atom<FluentTag>(m, "MutableFluentAtom");
    bind_atom<DerivedTag>(m, "MutableDerivedAtom");
    bind_literal<StaticTag>(m, "MutableStaticLiteral");
    bind_literal<FluentTag>(m, "MutableFluentLiteral");
    bind_literal<DerivedTag>(m, "MutableDerivedLiteral");
    bind_conjunctive_condition(m, "MutableConjunctiveCondition");
    bind_conjunctive_effect(m, "MutableConjunctiveEffect");
    bind_conditional_effect(m, "MutableConditionalEffect");
    bind_action(m, "MutableAction");
}
}
