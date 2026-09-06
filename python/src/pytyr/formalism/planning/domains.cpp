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

#include "bindings.hpp"
#include "tyr/analysis/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace nb = nanobind;
using namespace nb::literals;

namespace tyr::formalism::planning
{

namespace
{

template<typename C>
void bind_variable_domain_view(nb::module_& m, const char* name)
{
    using T = tyr::analysis::VariableDomainView<C>;

    auto cls = nb::class_<T>(m, name)  //
                   .def_ro("objects", &T::objects);
    ygg::add_print(cls);
}

template<typename Element, typename C>
void bind_simple_scoped_domain_view(nb::module_& m, const char* name)
{
    using T = tyr::analysis::SimpleScopedDomainView<Element, C>;

    auto cls = nb::class_<T>(m, name)  //
                   .def_ro("element", &T::element)
                   .def_ro("variable_domains", &T::payload);
    ygg::add_print(cls);
}

template<typename C>
void bind_conditional_effect_domain_view(nb::module_& m, const char* name)
{
    using T = tyr::analysis::ConditionalEffectDomainView<C>;

    auto cls = nb::class_<T>(m, name)
                   .def_ro("element", &T::element)
                   .def_prop_ro("condition_domain", [](const T& self) -> const auto& { return self.payload.condition_domain; })
                   .def_prop_ro("effect_domain", [](const T& self) -> const auto& { return self.payload.effect_domain; });
    ygg::add_print(cls);
}

template<typename C>
void bind_action_domain_view(nb::module_& m, const char* name)
{
    using T = tyr::analysis::ActionDomainView<C>;

    auto cls = nb::class_<T>(m, name)
                   .def_ro("element", &T::element)
                   .def_prop_ro("precondition_domain", [](const T& self) -> const auto& { return self.payload.precondition_domain; })
                   .def_prop_ro("effect_domains", [](const T& self) -> const auto& { return self.payload.effect_domains; });
    ygg::add_print(cls);
}

void bind_task_variable_domains_view(nb::module_& m, const char* name)
{
    using T = tyr::analysis::TaskVariableDomainsView;

    auto cls = nb::class_<T>(m, name)  //
                   .def_ro("static_predicate_domains", &T::static_predicate_domains)
                   .def_ro("fluent_predicate_domains", &T::fluent_predicate_domains)
                   .def_ro("derived_predicate_domains", &T::derived_predicate_domains)
                   .def_ro("static_function_domains", &T::static_function_domains)
                   .def_ro("fluent_function_domains", &T::fluent_function_domains)
                   .def_ro("action_domains", &T::action_domains)
                   .def_ro("axiom_domains", &T::axiom_domains);
    ygg::add_print(cls);
}

}  // namespace

void bind_variable_domains(nb::module_& m)
{
    using PlanningC = ::tyr::formalism::planning::Repository;

    // Basic variable-domain views
    bind_variable_domain_view<PlanningC>(m, "VariableDomain");

    // Planning scoped views
    bind_simple_scoped_domain_view<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>, PlanningC>(m, "AxiomDomain");
    bind_simple_scoped_domain_view<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>, PlanningC>(m, "ConjunctiveConditionDomain");
    bind_simple_scoped_domain_view<::tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>, PlanningC>(m, "ConjunctiveEffectDomain");

    // Composite planning views
    bind_conditional_effect_domain_view<PlanningC>(m, "ConditionalEffectDomain");
    bind_action_domain_view<PlanningC>(m, "ActionDomain");

    // Top-level view containers
    bind_task_variable_domains_view(m, "TaskVariableDomains");
}

}
