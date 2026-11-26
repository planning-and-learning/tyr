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
#include "pytyr/formalism/conversions.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/planning/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{

namespace
{
template<TaskKind T>
void bind_choice_effect_kind(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_index<Outcome<T>>(m);
    urd::python::bind_data<Outcome<T>>(m);
    urd::python::bind_view<OutcomeView<T>>(m);
    python::bind_insert<Outcome<T>>(repository);
    urd::python::bind_builder<Outcome<T>, Repository>(m);

    urd::python::bind_index<ChoiceEffect<T>>(m);
    urd::python::bind_data<ChoiceEffect<T>>(m);
    urd::python::bind_view<ChoiceEffectView<T>>(m);
    python::bind_insert<ChoiceEffect<T>>(repository);
    urd::python::bind_builder<ChoiceEffect<T>, Repository>(m);

    urd::python::bind_index<TemporalPart<T>>(m);
    urd::python::bind_data<TemporalPart<T>>(m);
    urd::python::bind_view<TemporalPartView<T>>(m);
    python::bind_insert<TemporalPart<T>>(repository);
    urd::python::bind_builder<TemporalPart<T>, Repository>(m);
}
}  // namespace

void bind_choice_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_choice_effect_kind<LiftedTag>(m, repository);
    bind_choice_effect_kind<GroundTag>(m, repository);

    urd::python::bind_index<TimedLiteral>(m);
    urd::python::bind_data<TimedLiteral>(m);
    urd::python::bind_view<TimedLiteralView>(m);
    python::bind_insert<TimedLiteral>(repository);
    urd::python::bind_builder<TimedLiteral, Repository>(m);
}

}  // namespace tyr::formalism
