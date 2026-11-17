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

#include "tyr/formalism/datalog/formatter.hpp"

#include "../node_formatter.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iterator>
#include <urd/dot.hpp>
#include <urd/format.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

/// What only datalog writes, next to node_formatter.hpp's shared records; declared before the to_string templates
/// below, which call node_format::format qualified.
namespace tyr::formalism::node_format
{
/// A rule's binding: its values (a rule has no name).
static std::string format(datalog::RuleRowView value) { return ygg::values_text(value); }
}  // namespace tyr::formalism::node_format

namespace tyr::formalism::datalog
{

/// A term as an atom writes it: an object by its name, a parameter as its index.
std::string to_string(TermView value)
{
    return std::visit([](auto term) { return fmt::format("{}", term); }, value);
}

std::string to_string(const urd::data<ygg::Row<Rule<LiftedTag>>>& value) { return node_format::format(value); }

/// One DOT graph per kind of binary dependency, named after it: the condition's parameters, adjacent where they occur
/// together in a literal (or constraint) of that kind.
std::string to_string(const VariableDependencyGraph& value)
{
    const auto& binary = value.binary();
    const auto parameter = [](const auto& /*graph*/, ParameterIndex v) { return v; };
    return urd::to_dot(binary.static_positive_literal, parameter, urd::unlabeled, "static_positive_literal")
           + urd::to_dot(binary.static_negative_literal, parameter, urd::unlabeled, "static_negative_literal")
           + urd::to_dot(binary.fluent_positive_literal, parameter, urd::unlabeled, "fluent_positive_literal")
           + urd::to_dot(binary.fluent_negative_literal, parameter, urd::unlabeled, "fluent_negative_literal")
           + urd::to_dot(binary.numeric_constraint, parameter, urd::unlabeled, "numeric_constraint");
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

TYR_VIEW_TO_STRINGS(42, 6)

}  // namespace tyr::formalism::datalog
