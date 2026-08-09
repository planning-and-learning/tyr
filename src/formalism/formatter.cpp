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

#include "tyr/formalism/formatter.hpp"

#include "tyr/formalism/datas.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace tyr::formalism
{

namespace
{

template<FactKind T>
std::string to_string_impl(const ygg::Data<Predicate<T>>& value)
{
    return fmt::format("{}/{}", value.name, value.arity);
}

template<FactKind T>
std::string to_string_impl(const ygg::Data<Function<T>>& value)
{
    return fmt::format("{}/{}", value.name, value.arity);
}

template<typename T>
std::string to_string_impl(const ygg::Data<RelationBinding<T>>& value)
{
    return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
}

}  // namespace

std::string to_string(const ygg::Data<Variable>& value) { return fmt::format("{}", value.name); }

std::string to_string(const ygg::Data<Object>& value) { return fmt::format("{}", value.name); }

std::string to_string(const ygg::Data<Term>& value) { return fmt::format("{}", value.value); }

std::string to_string(const ygg::Data<Predicate<StaticTag>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<Predicate<FluentTag>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<Predicate<DerivedTag>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<Function<StaticTag>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<Function<FluentTag>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<Function<AuxiliaryTag>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<RelationBinding<Predicate<StaticTag>>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<RelationBinding<Predicate<FluentTag>>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<RelationBinding<Predicate<DerivedTag>>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<RelationBinding<Function<StaticTag>>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<RelationBinding<Function<FluentTag>>>& value) { return to_string_impl(value); }

std::string to_string(const ygg::Data<RelationBinding<Function<AuxiliaryTag>>>& value) { return to_string_impl(value); }

}  // namespace tyr::formalism
