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

#ifndef TYR_FORMALISM_DATALOG_REPOSITORY_HPP_
#define TYR_FORMALISM_DATALOG_REPOSITORY_HPP_

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/indices.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <cassert>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/buffer/declarations.hpp>
#include <yggdrasil/buffer/indexed_hash_set.hpp>
#include <yggdrasil/buffer/segmented_buffer.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/interning.hpp>
#include <yggdrasil/formalism/relation_repository.hpp>
#include <yggdrasil/formalism/repository.hpp>
#include <yggdrasil/formalism/repository_factory.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::datalog
{

using Builder = ygg::ApplyTypeListT<ygg::formalism::BuilderStorage, BuilderTypes>;

using ygg::formalism::checkout;
using ygg::formalism::insert;

template<typename T>
void prepare_for_insert(Repository&, ygg::Data<T>& data)
{
    canonicalize(data);
}

/// Packs the bindings of relation g by the positions of their objects in the sorted argument domains; call before its first binding.
/// A no-op for relations whose bindings are interned.
template<typename Relation>
void set_binding_layout(Repository& repository, ygg::Index<Relation> g, std::span<const std::vector<ygg::Index<formalism::Object>>> domains)
{
    if constexpr (!Repository::stores_bindings<Relation>)
        repository.get_relation_repository<Relation>().set_layout(g, domains);
}

template<typename Relation>
bool has_binding_layout(const Repository& repository, ygg::Index<Relation> g) noexcept
{
    if constexpr (!Repository::stores_bindings<Relation>)
        return repository.get_relation_repository<Relation>().find_layout(g) != nullptr;
    else
        return false;
}

/// Gives relation target in destination the binding layout of relation source, which must have one.
template<typename Relation>
void copy_binding_layout(const Repository& source_repository, ygg::Index<Relation> source, Repository& destination, ygg::Index<Relation> target)
{
    if constexpr (!Repository::stores_bindings<Relation>)
        destination.get_relation_repository<Relation>().set_layout(target, *source_repository.get_relation_repository<Relation>().find_layout(source));
}

template<RelationKind R>
std::optional<RuleView<GroundTag, R>> find_ground_rule(RuleBindingIndex<R> binding, const Repository& repository)
{
    auto rule = ygg::Data<Rule<GroundTag, R>> {};
    rule.binding = binding;
    return repository.find(rule);
}

template<RelationKind R>
std::optional<RuleView<GroundTag, R>> find_ground_rule(RuleBindingView<R> binding)
{
    return find_ground_rule(binding.get_index(), binding.get_context());
}

}

#endif
