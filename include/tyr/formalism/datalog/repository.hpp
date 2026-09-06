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
#include <tuple>
#include <type_traits>
#include <utility>
#include <yggdrasil/buffer/declarations.hpp>
#include <yggdrasil/buffer/indexed_hash_set.hpp>
#include <yggdrasil/buffer/segmented_buffer.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/relation_repository.hpp>
#include <yggdrasil/formalism/repository.hpp>
#include <yggdrasil/formalism/repository_factory.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::datalog
{

using Builder = ygg::ApplyTypeListT<ygg::formalism::BuilderStorage, BuilderTypes>;

template<typename T>
[[nodiscard]] auto checkout(Builder& builder)
{
    auto data = builder.template get_builder<T>();
    data->clear();
    return data;
}

template<typename T>
[[nodiscard]] auto get_or_create(Repository& repository, ygg::Data<T>& data)
{
    canonicalize(data);
    return repository.get_or_create(data);
}

template<RelationKind R>
std::optional<RuleView<::tyr::GroundTag, R>> find_ground_rule(RuleBindingView<R> binding)
{
    auto rule = ygg::Data<Rule<::tyr::GroundTag, R>> {};
    rule.binding = binding.get_index();
    return binding.get_context().find(rule);
}

}

#endif
