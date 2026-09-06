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
    using StaticToStaticAtomMapping = ygg::UnorderedMap<formalism::datalog::AtomView<GroundTag, formalism::StaticTag>,
                                                        formalism::planning::AtomView<GroundTag, formalism::StaticTag>>;
    using FluentToFluentAtomMapping = ygg::UnorderedMap<formalism::datalog::PredicateBindingView<formalism::FluentTag>,
                                                        formalism::planning::AtomView<GroundTag, formalism::FluentTag>>;
    using FluentToDerivedAtomMapping = ygg::UnorderedMap<formalism::datalog::AtomView<GroundTag, formalism::FluentTag>,
                                                         formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>;
    using StaticToStaticFunctionTermMapping = ygg::UnorderedMap<formalism::datalog::FunctionTermView<GroundTag, formalism::StaticTag>,
                                                                formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag>>;
    using FluentToFluentFunctionTermMapping = ygg::UnorderedMap<formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag>,
                                                                formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag>>;

    StaticToStaticAtomMapping static_to_static_atom;
    FluentToFluentAtomMapping fluent_to_fluent_atom;
    FluentToDerivedAtomMapping fluent_to_derived_atom;
    StaticToStaticFunctionTermMapping static_to_static_fterm;
    FluentToFluentFunctionTermMapping fluent_to_fluent_fterm;
};

template<>
struct P2DTranslationContext<GroundTag>
{
    using StaticToStaticAtomMapping = ygg::UnorderedMap<formalism::planning::AtomView<GroundTag, formalism::StaticTag>,
                                                        formalism::datalog::AtomView<GroundTag, formalism::StaticTag>>;
    using FluentToFluentAtomMapping = ygg::UnorderedMap<formalism::planning::AtomView<GroundTag, formalism::FluentTag>,
                                                        formalism::datalog::AtomView<GroundTag, formalism::FluentTag>>;
    using DerivedToFluentAtomMapping = ygg::UnorderedMap<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>,
                                                         formalism::datalog::AtomView<GroundTag, formalism::FluentTag>>;
    using StaticToStaticFunctionTermMapping = ygg::UnorderedMap<formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag>,
                                                                formalism::datalog::FunctionTermView<GroundTag, formalism::StaticTag>>;
    using FluentToFluentFunctionTermMapping = ygg::UnorderedMap<formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag>,
                                                                formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag>>;

    StaticToStaticAtomMapping static_to_static_atom;
    FluentToFluentAtomMapping fluent_to_fluent_atom;
    DerivedToFluentAtomMapping derived_to_fluent_atom;
    StaticToStaticFunctionTermMapping static_to_static_fterm;
    FluentToFluentFunctionTermMapping fluent_to_fluent_fterm;
};

}

#endif
