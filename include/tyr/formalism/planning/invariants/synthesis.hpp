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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_SYNTHESIS_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_SYNTHESIS_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"

namespace tyr::formalism::planning::invariant
{
/// The invariants of the domain (Helmert 2009, Section 5), interned into `repository`: the domain's repository, or a
/// descendant of it. Repositories that other repositories branched from must not change, so the first call for a
/// repository must come before any task is created from it. Synthesis is deterministic and interning finds existing
/// entries, so later calls add nothing.
InvariantViewList synthesize_invariants(DomainView domain, Repository& repository);
}

#endif
