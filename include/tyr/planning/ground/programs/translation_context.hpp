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

#ifndef TYR_PLANNING_GROUND_PROGRAMS_TRANSLATION_CONTEXT_HPP_
#define TYR_PLANNING_GROUND_PROGRAMS_TRANSLATION_CONTEXT_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/programs/translation_context.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

template<>
struct D2PTranslationContext<GroundTag>
{
    ygg::UnorderedMap<formalism::datalog::AtomView<GroundTag, formalism::StaticTag>, formalism::planning::AtomView<GroundTag, formalism::StaticTag>>
        static_to_static_atom;
    ygg::UnorderedMap<formalism::datalog::PredicateBindingView<formalism::FluentTag>, formalism::planning::AtomView<GroundTag, formalism::FluentTag>>
        fluent_to_fluent_atom;
    ygg::UnorderedMap<formalism::datalog::AtomView<GroundTag, formalism::FluentTag>, formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>
        fluent_to_derived_atom;
    ygg::UnorderedMap<formalism::datalog::FunctionTermView<GroundTag, formalism::StaticTag>,
                      formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag>>
        static_to_static_fterm;
    ygg::UnorderedMap<formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag>,
                      formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag>>
        fluent_to_fluent_fterm;
};

template<>
struct P2DTranslationContext<GroundTag>
{
    using FluentToFluentAtomMapping =
        ygg::UnorderedMap<formalism::planning::AtomView<GroundTag, formalism::FluentTag>, formalism::datalog::AtomView<GroundTag, formalism::FluentTag>>;
    using FluentToFluentFunctionTermMapping = ygg::UnorderedMap<formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag>,
                                                                formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag>>;

    ygg::UnorderedMap<formalism::planning::AtomView<GroundTag, formalism::StaticTag>, formalism::datalog::AtomView<GroundTag, formalism::StaticTag>>
        static_to_static_atom;
    FluentToFluentAtomMapping fluent_to_fluent_atom;
    ygg::UnorderedMap<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>, formalism::datalog::AtomView<GroundTag, formalism::FluentTag>>
        derived_to_fluent_atom;
    ygg::UnorderedMap<formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag>,
                      formalism::datalog::FunctionTermView<GroundTag, formalism::StaticTag>>
        static_to_static_fterm;
    FluentToFluentFunctionTermMapping fluent_to_fluent_fterm;
};
}

#endif
