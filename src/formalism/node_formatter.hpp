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

#ifndef TYR_SRC_FORMALISM_NODE_FORMATTER_HPP_
#define TYR_SRC_FORMALISM_NODE_FORMATTER_HPP_

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"

#include <boost/mp11/algorithm.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string>
#include <yggdrasil/formatting/cista_formatters.hpp>

/// How the records datalog and planning share print: their data, and their views read through either repository C
/// (the including file brings C's fmt formatters, formalism/<namespace>/formatter.hpp).
namespace tyr::formalism::node_format
{

template<TaskKind T>
std::string format(const urd::data<UnaryOperator<T, ArithmeticOperatorKind>>& value)
{ return fmt::format("({} {})", value.operator_kind, value.arg); }

template<TaskKind T, typename C>
std::string format(urd::index_view<UnaryOperator<T, ArithmeticOperatorKind>, C> value)
{ return fmt::format("({} {})", value.get_operator(), value.get_arg()); }

template<TaskKind T, OperatorKind O>
std::string format(const urd::data<BinaryOperator<T, O>>& value)
{ return fmt::format("({} {} {})", value.operator_kind, value.lhs, value.rhs); }

template<TaskKind T, OperatorKind O, typename C>
std::string format(urd::index_view<BinaryOperator<T, O>, C> value)
{ return fmt::format("({} {} {})", value.get_operator(), value.get_lhs(), value.get_rhs()); }

template<TaskKind T>
std::string format(const urd::data<MultiOperator<T, ArithmeticOperatorKind>>& value)
{ return fmt::format("({} {})", value.operator_kind, fmt::join(ygg::to_strings(value.args), " ")); }

template<TaskKind T, typename C>
std::string format(urd::index_view<MultiOperator<T, ArithmeticOperatorKind>, C> value)
{ return fmt::format("({} {})", value.get_operator(), fmt::join(ygg::to_strings(value.get_args()), " ")); }

template<FactKind F>
std::string format(const urd::data<Atom<LiftedTag, F>>& value)
{ return fmt::format("({} {})", value.predicate, fmt::join(ygg::to_strings(value.terms), " ")); }

template<FactKind F, typename C>
std::string format(urd::index_view<Atom<LiftedTag, F>, C> value)
{ return fmt::format("({} {})", value.get_predicate().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " ")); }

template<FactKind F>
std::string format(const urd::data<Atom<GroundTag, F>>& value)
{ return fmt::format("({})", value.row); }

template<FactKind F, typename C>
std::string format(urd::index_view<Atom<GroundTag, F>, C> value)
{ return fmt::format("{}", value.get_row()); }

template<TaskKind T, FactKind F>
std::string format(const urd::data<Literal<T, F>>& value)
{ return value.polarity ? fmt::format("{}", value.atom) : fmt::format("(not {})", value.atom); }

template<TaskKind T, FactKind F, typename C>
std::string format(urd::index_view<Literal<T, F>, C> value)
{ return value.get_polarity() ? fmt::format("{}", value.get_atom()) : fmt::format("(not {})", value.get_atom()); }

template<FactKind F>
std::string format(const urd::data<FunctionTerm<LiftedTag, F>>& value)
{ return fmt::format("({} {})", value.function, fmt::join(ygg::to_strings(value.terms), " ")); }

template<FactKind F, typename C>
std::string format(urd::index_view<FunctionTerm<LiftedTag, F>, C> value)
{ return fmt::format("({} {})", value.get_function().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " ")); }

template<FactKind F>
std::string format(const urd::data<FunctionTerm<GroundTag, F>>& value)
{ return fmt::format("({})", value.row); }

template<FactKind F, typename C>
std::string format(urd::index_view<FunctionTerm<GroundTag, F>, C> value)
{ return fmt::format("{}", value.get_row()); }

template<FactKind F>
std::string format(const urd::data<FunctionTermValue<GroundTag, F>>& value)
{ return fmt::format("(= {} {})", value.fterm, value.value); }

template<FactKind F, typename C>
std::string format(urd::index_view<FunctionTermValue<GroundTag, F>, C> value)
{ return fmt::format("(= {} {})", value.get_fterm(), value.get_value()); }

template<TaskKind T, FactKind F>
std::string format(const urd::data<NumericEffect<T, F>>& value)
{ return fmt::format("({} {} {})", value.operator_kind, value.fterm, value.fexpr); }

template<TaskKind T, FactKind F, typename C>
std::string format(urd::index_view<NumericEffect<T, F>, C> value)
{ return fmt::format("({} {} {})", value.get_operator(), value.get_fterm(), value.get_fexpr()); }

}


/// Explicit instantiations of a namespace's view to_string templates (formalism/<namespace>/formatter.hpp) for the I-th
/// type of its SymbolRepositoryTypes, or of its RelationRepositoryTypes (the relation's binding view).
#define TYR_VIEW_TO_STRING_AT(z, I, List) template std::string to_string(urd::index_view<boost::mp11::mp_at_c<List, I>, Repository>);
#define TYR_BINDING_VIEW_TO_STRING_AT(z, I, List) \
    template std::string to_string(                \
        ygg::RowView<boost::mp11::mp_at_c<List, I>, Repository>);

/// All of them, in the namespace that defines the templates; the counts are checked against the lists.
#define TYR_VIEW_TO_STRINGS(SymbolCount, RelationCount)                                                              \
    static_assert(boost::mp11::mp_size<SymbolRepositoryTypes>::value == (SymbolCount),                                \
                  "TYR_VIEW_TO_STRINGS: SymbolCount must be the number of SymbolRepositoryTypes");                   \
    BOOST_PP_REPEAT(SymbolCount, TYR_VIEW_TO_STRING_AT, SymbolRepositoryTypes)                                        \
    static_assert(boost::mp11::mp_size<RelationRepositoryTypes>::value == (RelationCount),                            \
                  "TYR_VIEW_TO_STRINGS: RelationCount must be the number of RelationRepositoryTypes");               \
    BOOST_PP_REPEAT(RelationCount, TYR_BINDING_VIEW_TO_STRING_AT, RelationRepositoryTypes)

#endif
