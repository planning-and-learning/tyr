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

// The members of the repository, which the library's other sources only declare (tyr/formalism/repository.hpp): with
// every insert's preparation in sight (interning.hpp). Split over this source (its own members and its relations' keys),
// repository_builder.cpp (its builder's construction) and repository_symbols_{0,1,2}.cpp (a third of its symbols' keys and its builder's types each), so that
// no one source holds them all (brokkr's template rule T5). Each compiled on its own, outside the unity batches
// (src/CMakeLists.txt).

#include "tyr/formalism/repository.hpp"

#include "tyr/formalism/interning.hpp"

#include <cstddef>
#include <yggdrasil/formalism/builder_instantiations.hpp>
#include <yggdrasil/formalism/repository_impl.hpp>
#include <yggdrasil/formalism/repository_instantiations.hpp>

// The counts the sources' ranges cover: the symbols (repository_symbols_*.cpp), the relations (a Schema and its row
// families, here) and the builder's types (repository_symbols_*.cpp).
constexpr auto symbol_count = std::size_t { 70 };
constexpr auto relation_count = std::size_t { 9 };
constexpr auto builder_type_count = std::size_t { 79 };
static_assert(boost::mp11::mp_size<tyr::formalism::Repository::SymbolTypes>::value == symbol_count);
static_assert(boost::mp11::mp_size<tyr::formalism::Repository::RelationTypes>::value == relation_count);
static_assert(boost::mp11::mp_size<tyr::formalism::Builder::Types>::value == builder_type_count);
YGG_FORMALISM_REPOSITORY_MEMBERS(, tyr::formalism::Repository)
YGG_FORMALISM_PART_KEYS(, tyr::formalism::Repository, 1, 0, 10)
