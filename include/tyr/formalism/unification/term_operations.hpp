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

#ifndef TYR_FORMALISM_UNIFICATION_TERM_OPERATIONS_HPP_
#define TYR_FORMALISM_UNIFICATION_TERM_OPERATIONS_HPP_

#include "tyr/formalism/term_data.hpp"

namespace tyr::formalism::unification
{

inline bool is_parameter(const ygg::Data<Term>& term) { return holds_alternative<ParameterIndex>(term.variant); }

inline bool is_object(const ygg::Data<Term>& term) { return holds_alternative<ygg::Index<Object>>(term.variant); }

inline ParameterIndex get_parameter(const ygg::Data<Term>& term) { return std::get<ParameterIndex>(term.variant); }

inline ygg::Index<Object> get_object(const ygg::Data<Term>& term) { return std::get<ygg::Index<Object>>(term.variant); }

}

#endif
