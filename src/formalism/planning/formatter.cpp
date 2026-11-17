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

#include "tyr/formalism/planning/formatter.hpp"

#include "../node_formatter.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <urd/format.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/io/iostream.hpp>

/// What only planning writes in PDDL, next to node_formatter.hpp's shared records; declared before the to_string
/// templates below, which call node_format::format qualified.
namespace tyr::formalism::node_format
{
template<FactKind T>
static std::string format(const urd::data<planning::FDRFact<T>>& value)
{ return fmt::format("<{},{}>", value.variable, value.value); }

/// A fact's view, borrowed (a stored fact) or owned (a state's, computed on the fly).
template<typename V>
    requires std::same_as<typename V::value_type, planning::FDRFact<FluentTag>>
static std::string format(V value)
{
    const auto atom = value.get_atom();
    if (!atom.has_value())
    {
        return fmt::format("<{},{}>: (none-of {})",
                           value.get_variable().index(),
                           value.get_value(),
                           fmt::join(ygg::to_strings(value.get_variable().get_atoms()), " "));
    }
    return fmt::format("<{},{}>: {}", value.get_variable().index(), value.get_value(), atom.value());
}

static std::string format(const urd::data<planning::Metric>& value) { return fmt::format("({} {})", value.optimization_direction, value.fexpr); }

static std::string format(planning::MetricView value) { return fmt::format("({} {})", value.get_optimization_direction(), value.get_fexpr()); }

/// An axiom's binding: its values (an axiom has no name).
static std::string format(planning::AxiomRowView value) { return ygg::values_text<planning::Axiom<LiftedTag>>(value); }

static std::string format(const planning::PlanningDomain& value) { return fmt::format("{}", value.get_domain()); }

template<TaskKind T>
static std::string format(const planning::PlanningTask<T>& value)
{ return fmt::format("{}", value.get_task()); }

/// A plan's step: the action's original name and its original arguments' names.
static std::string format(const std::pair<planning::ActionRowView, planning::PlanFormatting>& value)
{
    auto result = fmt::format("({}", value.first.get_relation().get_original_name());
    const auto objects = value.first.get_objects();
    for (std::size_t i = 0; i < value.first.get_relation().get_original_arity(); ++i)
        fmt::format_to(std::back_inserter(result), " {}", objects[static_cast<std::ptrdiff_t>(i)].get_name());
    result.push_back(')');
    return result;
}

static std::string format(const std::pair<planning::ActionView<GroundTag>, planning::PlanFormatting>& value)
{ return format(std::make_pair(value.first.get_row(), value.second)); }
}  // namespace tyr::formalism::node_format

namespace tyr::formalism::planning
{

std::string to_string(const urd::data<FDRFact<FluentTag>>& value) { return node_format::format(value); }

std::string to_string(const urd::data<Metric>& value) { return node_format::format(value); }

std::string to_string(const urd::data<ygg::Row<Action<LiftedTag>>>& value) { return node_format::format(value); }

std::string to_string(const urd::data<ygg::Row<Axiom<LiftedTag>>>& value) { return node_format::format(value); }

std::string to_string(ActionRowDataView value) { return node_format::format(value); }

std::string to_string(InvariantView value)
{
    auto os = std::stringstream {};
    os << "Invariant(\n";
    {
        const auto scope = ygg::IndentScope(os);
        os << ygg::print_indent;
        fmt::print(os, "index = {}\n", value.index());
        os << ygg::print_indent;
        fmt::print(os, "num rigid variables = {}\n", value.get_num_rigid_variables());
        os << ygg::print_indent;
        fmt::print(os, "num counted variables = {}\n", value.get_num_counted_variables());
        os << ygg::print_indent;
        fmt::print(os, "atoms = {}\n", value.get_atoms());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(FDRFactView<FluentTag> value) { return node_format::format(value); }

std::string to_string(urd::owning_view<FDRFact<FluentTag>, Repository> value) { return node_format::format(value); }

std::string to_string(const PlanningDomain& value) { return node_format::format(value); }

std::string to_string(const PlanningTask<LiftedTag>& value) { return node_format::format(value); }

std::string to_string(const PlanningTask<GroundTag>& value) { return node_format::format(value); }

std::string to_string(const std::pair<ActionRowView, PlanFormatting>& value) { return node_format::format(value); }

std::string to_string(const std::pair<ActionView<GroundTag>, PlanFormatting>& value) { return node_format::format(value); }

/// A term as an atom writes it: an object by its name, a parameter as its index.
std::string to_string(TermView value)
{
    return std::visit([](auto term) { return fmt::format("{}", term); }, value);
}

template<typename T>
    requires boost::mp11::mp_contains<SymbolRepositoryTypes, T>::value
std::string to_string(urd::index_view<T, Repository> value)
{
    // In PDDL where tyr writes the record so (node_format), else as urd writes a record: its fields.
    if constexpr (requires { node_format::format(value); })
        return node_format::format(value);
    else
        return urd::record_written<T, urd::data<T>>([&]<typename F>() -> decltype(auto) { return value.template member<F::member>(); });
}

template<typename R>
    requires boost::mp11::mp_contains<RelationRepositoryTypes, R>::value
std::string to_string(ygg::RowView<R, Repository> value)
{ return node_format::format(value); }

TYR_VIEW_TO_STRINGS(57, 8)

}  // namespace tyr::formalism::planning
