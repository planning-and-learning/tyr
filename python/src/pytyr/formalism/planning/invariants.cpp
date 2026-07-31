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

#include "tyr/formalism/planning/invariants/invariants.hpp"

#include "bindings.hpp"
#include "tyr/formalism/unification/formatter.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::planning::invariant
{

namespace
{
template<typename T>
void bind_substitution_function(nb::module_& m, const std::string& name)
{
    using V = unification::SubstitutionFunction<T>;

    auto cls = nb::class_<V>(m, name.c_str())
                   .def(nb::init<>())
                   .def(nb::init<std::vector<ParameterIndex>>(), "parameters"_a)
                   .def_static("from_range", &V::from_range, "offset"_a, "size"_a)
                   .def("size", &V::size)
                   .def_prop_ro("parameters", &V::parameters)
                   .def("contains_parameter", &V::contains_parameter, "parameter"_a)
                   .def("is_bound", &V::is_bound, "parameter"_a)
                   .def("is_unbound", &V::is_unbound, "parameter"_a)
                   .def("has_binding", &V::has_binding, "parameter"_a)
                   .def(
                       "get",
                       [](const V& self, ParameterIndex p) -> std::optional<T>
                       {
                           const auto* slot = self.try_get(p);
                           if (slot == nullptr)
                               throw nb::key_error("parameter not in substitution domain");
                           return *slot;
                       },
                       "parameter"_a)
                   .def(
                       "set",
                       [](V& self, ParameterIndex p, std::optional<T> value)
                       {
                           auto* slot = self.try_get(p);
                           if (slot == nullptr)
                               throw nb::key_error("parameter not in substitution domain");
                           *slot = std::move(value);
                       },
                       "parameter"_a,
                       "value"_a)
                   .def(
                       "assign",
                       [](V& self, ParameterIndex p, const T& value)
                       {
                           if (!self.contains_parameter(p))
                               throw nb::key_error("parameter not in substitution domain");
                           return self.assign(p, value);
                       },
                       "parameter"_a,
                       "value"_a)
                   .def(
                       "assign_or_check",
                       [](V& self, ParameterIndex p, const T& value)
                       {
                           if (!self.contains_parameter(p))
                               throw nb::key_error("parameter not in substitution domain");
                           return self.assign_or_check(p, value);
                       },
                       "parameter"_a,
                       "value"_a)
                   .def(
                       "clear",
                       [](V& self, ParameterIndex p)
                       {
                           auto* slot = self.try_get(p);
                           if (slot == nullptr)
                               throw nb::key_error("parameter not in substitution domain");
                           *slot = std::nullopt;
                       },
                       "parameter"_a)
                   .def("reset", &V::reset)
                   .def("items",
                        [](const V& self)
                        {
                            std::vector<std::pair<ParameterIndex, T>> result;
                            self.for_each_binding([&](ParameterIndex p, const T& value) { result.emplace_back(p, value); });
                            return result;
                        })
                   .def("__contains__", [](const V& self, ParameterIndex p) { return self.contains_parameter(p); }, "parameter"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_invariant(nb::module_& m, const std::string& name)
{
    using V = Invariant;

    auto cls = nb::class_<V>(m, name.c_str())
                   .def_ro("num_rigid_variables", &V::num_rigid_variables)
                   .def_ro("num_counted_variables", &V::num_counted_variables)
                   .def_ro("atoms", &V::atoms)
                   .def_ro("predicates", &V::predicates);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_invariant_match_data(nb::module_& m, const std::string& name)
{
    using V = InvariantMatchData;
    using ObjectSubstitution = unification::SubstitutionFunction<ygg::Index<Object>>;

    nb::class_<V>(m, name.c_str())
        .def(nb::init<Invariant, std::vector<ObjectSubstitution>>(), "invariant"_a, "rigid_variable_bindings"_a)
        .def_prop_ro("invariant", &V::invariant, nb::rv_policy::reference_internal)
        .def_prop_ro("rigid_variable_bindings", &V::rigid_variable_bindings);
}

void bind_query_workspace(nb::module_& m, const std::string& name)
{
    using V = QueryWorkspace;

    nb::class_<V>(m, name.c_str())  //
        .def(nb::init<>());
}

void bind_query_result_match(nb::module_& m, const std::string& name)
{
    using V = QueryResult::Match;

    nb::class_<V>(m, name.c_str())  //
        .def_ro("invariant_index", &V::invariant_index)
        .def_ro("binding_index", &V::binding_index);
}

void bind_query_result(nb::module_& m, const std::string& name)
{
    using V = QueryResult;

    nb::class_<V>(m, name.c_str())  //
        .def(nb::init<>())
        .def_rw("matches", &V::matches);
}

void bind_matcher(nb::module_& m, const std::string& name)
{
    using V = Matcher;
    using MatchDataList = InvariantMatchDataList;

    nb::class_<V>(m, name.c_str())
        .def(nb::init<MatchDataList>(), "invariant_match_data"_a)
        .def("query",
             nb::overload_cast<const MutableAtom<FluentTag>&, QueryResult&, QueryWorkspace&>(&V::query, nb::const_),
             "atom"_a,
             "result"_a,
             "workspace"_a)
        .def("query", nb::overload_cast<GroundAtomView<FluentTag>, QueryResult&, QueryWorkspace&>(&V::query, nb::const_), "atom"_a, "result"_a, "workspace"_a)
        .def("query", nb::overload_cast<const MutableAtom<FluentTag>&>(&V::query, nb::const_), "atom"_a)
        .def("query", nb::overload_cast<GroundAtomView<FluentTag>>(&V::query, nb::const_), "atom"_a);
}
}

/**
 * bind_invariants
 */

void bind_invariants(nb::module_& m)
{
    bind_invariant(m, "Invariant");
    bind_substitution_function<ygg::Index<Object>>(m, "ObjectSubstitution");
    bind_substitution_function<ygg::Data<Term>>(m, "TermSubstitution");
    bind_invariant_match_data(m, "InvariantMatchData");
    bind_query_workspace(m, "InvariantQueryWorkspace");
    bind_query_result_match(m, "InvariantQueryResultMatch");
    bind_query_result(m, "InvariantQueryResult");
    bind_matcher(m, "InvariantMatcher");

    m.def("synthesize_invariants", &invariant::synthesize_invariants, "domain"_a);
}
}
