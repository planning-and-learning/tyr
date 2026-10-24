/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_FORMALISM_DATALOG_INTERNING_HPP_
#define TYR_FORMALISM_DATALOG_INTERNING_HPP_

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <yggdrasil/formalism/interning.hpp>

/// Insertion into the datalog repository: records are canonicalized before they are interned. Kept out of
/// repository.hpp, so code that only reads does not compile the canonicalization.
namespace tyr::formalism::datalog
{

using ygg::formalism::insert;

template<typename T>
void prepare_for_insert(Repository&, ygg::Data<T>& data)
{ canonicalize(data); }

}

#endif
