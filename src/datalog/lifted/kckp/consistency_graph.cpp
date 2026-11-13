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

#include "tyr/datalog/lifted/kckp/consistency_graph.hpp"

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/kckp/assignment_sets.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <urd/graphs.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::datalog
{
namespace details
{
namespace
{

/// Where a term's object comes from: the vertex's (or the edge's first vertex's) parameter p, the edge's second vertex's
/// parameter q, or a constant; or nowhere, a term of another parameter.
enum class Source : std::uint8_t
{
    p,
    q,
    constant,
    unbound,
};

/// The assignments a check reads, as bits: of one term from source s, and of two terms from sources s and t.
constexpr std::uint16_t single(Source s) noexcept { return static_cast<std::uint16_t>(1U << static_cast<unsigned>(s)); }
constexpr std::uint16_t pair(Source s, Source t) noexcept
{
    const auto first = static_cast<unsigned>(s);
    const auto second = static_cast<unsigned>(t);
    return static_cast<std::uint16_t>(1U << (4U + (3U * std::min(first, second)) + std::max(first, second)));
}

/// A vertex reads its parameter's terms, alone and with each constant, and the constants alone.
constexpr auto at_vertex = static_cast<std::uint16_t>(single(Source::p) | single(Source::constant) | pair(Source::p, Source::constant));
/// An edge reads its literals' terms of p with those of q, and of p or q with each constant: what its vertices do not.
constexpr auto at_edge = static_cast<std::uint16_t>(pair(Source::p, Source::q) | pair(Source::p, Source::constant) | pair(Source::q, Source::constant));
/// A function term's interval at an edge: everything an edge and both its vertices read, and the constants in pairs.
constexpr auto at_edge_interval = static_cast<std::uint16_t>(at_vertex | at_edge | single(Source::q) | pair(Source::constant, Source::constant));

inline std::uint16_t interval_assignments(const Vertex&) noexcept { return at_vertex; }
inline std::uint16_t interval_assignments(const Edge&) noexcept { return at_edge_interval; }

/// Where a term's object comes from at a vertex: its parameter (p) or a constant.
inline Source source_of(const Term& term, const Vertex& vertex) noexcept
{
    if (term.constant)
        return Source::constant;
    return term.value == vertex.get_parameter_index().value ? Source::p : Source::unbound;
}
/// At an edge: the parameter of its first vertex (p), of its second (q), or a constant.
inline Source source_of(const Term& term, const Edge& edge) noexcept
{
    if (term.constant)
        return Source::constant;
    if (term.value == edge.vi().get_parameter_index().value)
        return Source::p;
    return term.value == edge.vj().get_parameter_index().value ? Source::q : Source::unbound;
}

inline urd::index<formalism::Object> object_of(const Term& term, Source from, const Vertex& vertex) noexcept
{ return from == Source::constant ? urd::index<formalism::Object>(term.value) : vertex.get_object_index(); }
inline urd::index<formalism::Object> object_of(const Term& term, Source from, const Edge& edge) noexcept
{
    if (from == Source::constant)
        return urd::index<formalism::Object>(term.value);
    return from == Source::p ? edge.vi().get_object_index() : edge.vj().get_object_index();
}

/// f(assignment) for each assignment `wanted` names of the terms a vertex or an edge binds, the constants included:
/// positions in increasing order, so a pair is sorted as an EdgeAssignment is. f returns false to stop; whether none did.
template<typename Terms, typename GraphStructure, typename F>
inline bool for_each_assignment(const Terms& terms, const GraphStructure& structure, std::uint16_t wanted, F f) noexcept
{
    const auto source = [&](const Term& term) { return source_of(term, structure); };
    const auto object = [&](const Term& term, Source from) { return object_of(term, from, structure); };

    for (size_t i = 0; i < terms.size(); ++i)
    {
        const auto& first = terms[i];
        const auto first_source = source(first);
        if (first_source == Source::unbound)
            continue;
        const auto first_object = object(first, first_source);
        if ((wanted & single(first_source)) != 0 && !f(VertexAssignment(formalism::ParameterIndex(first.position), first_object)))
            return false;
        for (size_t j = i + 1; j < terms.size(); ++j)
        {
            const auto& second = terms[j];
            const auto second_source = source(second);
            if (second_source == Source::unbound || (wanted & pair(first_source, second_source)) == 0)
                continue;
            if (!f(EdgeAssignment(formalism::ParameterIndex(first.position),
                                  first_object,
                                  formalism::ParameterIndex(second.position),
                                  object(second, second_source))))
                return false;
        }
    }
    return true;
}

/// Whether every assignment `wanted` names of the literals filed at `filed` agrees with its literal's polarity.
template<formalism::FactKind T, typename Filed, typename GraphStructure>
inline bool consistent_literals(const Literals<T>& literals,
                                const Filed& filed,
                                const GraphStructure& structure,
                                std::uint16_t wanted,
                                const PredicateAssignmentSets<T>& predicate_assignment_sets) noexcept
{
    for (const auto literal : filed)
    {
        const auto& [predicate, polarity] = literals.heads[literal];
        const auto& set = predicate_assignment_sets.get_set(predicate);
        if (!for_each_assignment(literals.terms[literal], structure, wanted, [&](const auto& assignment) { return set.at(assignment) == polarity; }))
            return false;
    }
    return true;
}

template<formalism::FactKind T>
inline bool consistent_literals(const Vertex& vertex, const Literals<T>& literals, const PredicateAssignmentSets<T>& predicate_assignment_sets) noexcept
{ return consistent_literals(literals, literals.by_parameter[vertex.get_parameter_index().value], vertex, at_vertex, predicate_assignment_sets); }

template<formalism::FactKind T>
inline bool consistent_literals(const Edge& edge, const Literals<T>& literals, const PredicateAssignmentSets<T>& predicate_assignment_sets) noexcept
{
    const auto i = edge.vi().get_parameter_index().value;
    const auto j = edge.vj().get_parameter_index().value;
    return consistent_literals(literals, literals.by_pair[(std::min(i, j) * literals.arity) + std::max(i, j)], edge, at_edge, predicate_assignment_sets);
}

/// The interval of function term `term` the assignments a vertex or an edge reads allow.
template<formalism::FactKind T, typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(const FunctionTerms<T>& terms,
                                                             urd::index<formalism::FunctionTerm<LiftedTag, T>> term,
                                                             const GraphStructure& structure,
                                                             const FunctionAssignmentSets<T>& function_assignment_sets)
{
    const auto number = terms.number.at(term);
    const auto& set = function_assignment_sets.get_set(terms.functions[number]);
    auto bounds = set.at(EmptyAssignment());
    if (empty(bounds))
        return bounds;
    for_each_assignment(terms.terms[number],
                        structure,
                        interval_assignments(structure),
                        [&](const auto& assignment)
                        {
                            bounds = intersect(bounds, set.at(assignment));
                            return !empty(bounds);
                        });
    return bounds;
}

template<typename GraphStructure>
ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::UnaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element,
                                                      const GraphStructure& structure,
                                                      const ConstraintTerms& constraint_info,
                                                      const AssignmentSets& assignment_sets);

template<typename GraphStructure>
ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::BinaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element,
                                                      const GraphStructure& structure,
                                                      const ConstraintTerms& constraint_info,
                                                      const AssignmentSets& assignment_sets);

template<typename GraphStructure>
ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::MultiOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element,
                                                      const GraphStructure& structure,
                                                      const ConstraintTerms& constraint_info,
                                                      const AssignmentSets& assignment_sets);

template<typename GraphStructure>
ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::FunctionExpressionView<LiftedTag> element,
                                                      const GraphStructure& structure,
                                                      const ConstraintTerms& constraint_info,
                                                      const AssignmentSets& assignment_sets);

template<typename GraphStructure>
bool consistent_numeric_constraint(formalism::datalog::BooleanOperatorView<LiftedTag> element,
                                   const GraphStructure& structure,
                                   const ConstraintTerms& constraint_info,
                                   const AssignmentSets& assignment_sets);

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::UnaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element,
                                                             const GraphStructure& structure,
                                                             const ConstraintTerms& constraint_info,
                                                             const AssignmentSets& assignment_sets)
{ return apply(element.get_operator(), consistent_interval(element.get_arg(), structure, constraint_info, assignment_sets)); }

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::BinaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element,
                                                             const GraphStructure& structure,
                                                             const ConstraintTerms& constraint_info,
                                                             const AssignmentSets& assignment_sets)
{
    return apply(element.get_operator(),
                 consistent_interval(element.get_lhs(), structure, constraint_info, assignment_sets),
                 consistent_interval(element.get_rhs(), structure, constraint_info, assignment_sets));
}

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::MultiOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element,
                                                             const GraphStructure& structure,
                                                             const ConstraintTerms& constraint_info,
                                                             const AssignmentSets& assignment_sets)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           consistent_interval(child_fexprs.front(), structure, constraint_info, assignment_sets),
                           [&](const auto& value, const auto& child_expr)
                           { return apply(element.get_operator(), value, consistent_interval(child_expr, structure, constraint_info, assignment_sets)); });
}

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(ygg::float_t value, const GraphStructure&, const ConstraintTerms&, const AssignmentSets&)
{ return ygg::ClosedInterval<ygg::float_t>(value, value); }

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::FunctionTermView<LiftedTag, formalism::StaticTag> element,
                                                             const GraphStructure& structure,
                                                             const ConstraintTerms& constraint_info,
                                                             const AssignmentSets& assignment_sets)
{ return consistent_interval(constraint_info.static_terms, element.index(), structure, assignment_sets.static_sets().function); }

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::FunctionTermView<LiftedTag, formalism::FluentTag> element,
                                                             const GraphStructure& structure,
                                                             const ConstraintTerms& constraint_info,
                                                             const AssignmentSets& assignment_sets)
{ return consistent_interval(constraint_info.fluent_terms, element.index(), structure, assignment_sets.fluent_sets().function); }

template<typename GraphStructure>
inline ygg::ClosedInterval<ygg::float_t> consistent_interval(formalism::datalog::FunctionExpressionView<LiftedTag> element,
                                                             const GraphStructure& structure,
                                                             const ConstraintTerms& constraint_info,
                                                             const AssignmentSets& assignment_sets)
{
    return visit([&](const auto& arg) { return consistent_interval(arg, structure, constraint_info, assignment_sets); }, element);
}

template<typename GraphStructure>
inline bool consistent_numeric_constraint(formalism::datalog::BooleanOperatorView<LiftedTag> element,
                                          const GraphStructure& structure,
                                          const ConstraintTerms& constraint_info,
                                          const AssignmentSets& assignment_sets)
{
    return apply_existential(element.get_operator(),
                             consistent_interval(element.get_lhs(), structure, constraint_info, assignment_sets),
                             consistent_interval(element.get_rhs(), structure, constraint_info, assignment_sets));
}

/// Whether a vertex's or an edge's assignments satisfy every numeric constraint (nullary ones are tested separately).
template<typename GraphStructure>
inline bool consistent_numeric_constraints(const GraphStructure& structure,
                                           formalism::datalog::BooleanOperatorListView<LiftedTag> numeric_constraints,
                                           const urd::Store<urd::sequence<ConstraintTerms>>& constraint_terms,
                                           const AssignmentSets& assignment_sets)
{
    assert(numeric_constraints.size() == constraint_terms.size());

    for (ygg::uint_t i = 0; i < numeric_constraints.size(); ++i)
        if (!consistent_numeric_constraint(numeric_constraints[i], structure, constraint_terms[i], assignment_sets))
            return false;

    return true;
}

}
}

/// An element's terms (a literal's atom's or a function term's), in position order.
template<typename Terms>
static std::vector<details::Term> terms_of(const Terms& terms)
{
    auto result = std::vector<details::Term>();
    for (ygg::uint_t position = 0; position < terms.size(); ++position)
        visit(
            [&]<typename Alternative>(const Alternative& arg)
            {
                if constexpr (std::is_same_v<Alternative, formalism::ParameterIndex>)
                    result.push_back({ position, arg.value, false });
                else  // formalism::datalog::ObjectView
                    result.push_back({ position, static_cast<ygg::uint_t>(arg.index().value), true });
            },
            terms[position]);
    return result;
}

template<typename T>
static details::Rows<T> rows_of(const std::vector<std::vector<T>>& rows)
{ return details::Rows<T>(rows.begin(), rows.end()); }

/// A condition's literals of fact kind T and where a vertex or an edge reads them.
template<formalism::FactKind T>
static details::Literals<T> literals_of(formalism::datalog::LiteralListView<LiftedTag, T> literals, size_t arity)
{
    auto result = details::Literals<T> {};
    result.arity = arity;
    auto terms = std::vector<std::vector<details::Term>>();
    auto by_parameter = std::vector<std::vector<ygg::uint_t>>(arity);
    auto by_pair = std::vector<std::vector<ygg::uint_t>>(arity * arity);
    for (const auto literal : literals)
    {
        const auto number = static_cast<ygg::uint_t>(terms.size());
        result.heads.push_back({ literal.get_atom().get_predicate().index(), literal.get_polarity() });
        const auto& element = terms.emplace_back(terms_of(literal.get_atom().get_terms()));

        auto has = std::vector<bool>(arity);
        auto constants = false;
        for (const auto& term : element)
        {
            if (term.constant)
                constants = true;
            else
                has[term.value] = true;
        }
        for (size_t p = 0; p < arity; ++p)
        {
            if (has[p])
                by_parameter[p].push_back(number);
            for (size_t q = p + 1; q < arity; ++q)
                if ((has[p] && has[q]) || (constants && (has[p] || has[q])))
                    by_pair[(p * arity) + q].push_back(number);
        }
    }
    result.terms = rows_of(terms);
    result.by_parameter = rows_of(by_parameter);
    result.by_pair = rows_of(by_pair);
    return result;
}

template<formalism::FactKind T>
static details::FunctionTerms<T> function_terms_of(const urd::Store<urd::sequence<formalism::datalog::FunctionTermView<LiftedTag, T>>>& fterms)
{
    auto result = details::FunctionTerms<T> {};
    auto terms = std::vector<std::vector<details::Term>>();
    for (const auto fterm : fterms)
    {
        result.number.emplace(fterm.index(), static_cast<ygg::uint_t>(terms.size()));
        result.functions.push_back(fterm.get_function().index());
        terms.push_back(terms_of(fterm.get_terms()));
    }
    result.terms = rows_of(terms);
    return result;
}

static auto constraint_terms_of(formalism::datalog::ConjunctiveConditionView<LiftedTag> element)
{
    auto result = urd::Store<urd::sequence<details::ConstraintTerms>> {};
    for (const auto constraint : element.get_numeric_constraints())
        result.push_back({ function_terms_of(formalism::datalog::collect_fterms<LiftedTag, formalism::StaticTag>(constraint)),
                           function_terms_of(formalism::datalog::collect_fterms<LiftedTag, formalism::FluentTag>(constraint)) });
    return result;
}

static auto literals_of(formalism::datalog::ConjunctiveConditionView<LiftedTag> element)
{ return literals_of(element.get_literals<formalism::FluentTag>(), element.get_arity()); }

static auto classify_adjacency(const kckp::Graph& graph, const formalism::datalog::VariableDependencyGraph& dependency_graph)
{
    const auto num_partitions = graph.num_parts();
    assert(num_partitions == dependency_graph.k());
    auto result = urd::Store<urd::sequence<kckp::AdjacencyKind>> {};
    if (num_partitions != 0 && num_partitions > result.max_size() / num_partitions)
        throw std::overflow_error("StaticConsistencyGraph: partition-pair table size overflow");
    result.assign(num_partitions * num_partitions, kckp::AdjacencyKind::IMPLICIT);
    const auto& dependencies = dependency_graph.binary();

    for (ygg::uint_t pi = 0; pi < num_partitions; ++pi)
        for (ygg::uint_t pj = 0; pj < num_partitions; ++pj)
        {
            if (!dependencies.has_dependency(pi, pj))
                continue;

            const auto has_runtime_dependency = dependencies.has_literal_dependency<formalism::FluentTag, formalism::PositiveTag>(pi, pj)
                                                || dependencies.has_literal_dependency<formalism::FluentTag, formalism::NegativeTag>(pi, pj)
                                                || dependencies.has_numeric_dependency(pi, pj);
            result[pi * num_partitions + pj] = has_runtime_dependency ? kckp::AdjacencyKind::RUNTIME : kckp::AdjacencyKind::STATIC_ONLY;
        }

    return result;
}

StaticConsistencyGraph::StaticConsistencyGraph(formalism::datalog::ConjunctiveConditionView<LiftedTag> unary_overapproximation_condition,
                                               formalism::datalog::ConjunctiveConditionView<LiftedTag> binary_overapproximation_condition,
                                               formalism::datalog::ConjunctiveConditionView<LiftedTag> hyperedge_condition,
                                               kckp::Graph compatibility_graph) :
    m_unary_overapproximation_condition(unary_overapproximation_condition),
    m_binary_overapproximation_condition(binary_overapproximation_condition),
    m_unary_overapproximation_vdg(unary_overapproximation_condition),
    m_binary_overapproximation_vdg(binary_overapproximation_condition),
    m_compatibility_graph(std::move(compatibility_graph)),
    m_partitioned_adjacency_layout(m_compatibility_graph, classify_adjacency(m_compatibility_graph, m_binary_overapproximation_vdg)),
    m_unary_overapproximation_literals(literals_of(m_unary_overapproximation_condition)),
    m_binary_overapproximation_literals(literals_of(m_binary_overapproximation_condition)),
    m_hyperedge_literals(literals_of(hyperedge_condition)),
    m_unary_overapproximation_constraints(constraint_terms_of(m_unary_overapproximation_condition)),
    m_binary_overapproximation_constraints(constraint_terms_of(m_binary_overapproximation_condition))
{
}

void StaticConsistencyGraph::initialize_dynamic_consistency_graphs(const AssignmentSets& assignment_sets,
                                                                   kckp::DeltaGraph& delta_graph,
                                                                   kckp::FullGraph& full_graph) const
{
    const auto& graph = m_compatibility_graph;
    const auto num_partitions = graph.num_parts();
    const auto vertex_of = [&](ygg::uint_t p, kckp::Vertex v)
    { return details::Vertex(formalism::ParameterIndex(p), urd::index<formalism::Object>(graph.label(v))); };

    /// 1. Copy old full into delta, then add new vertices and edges into delta, before finally subtracting full from delta.

    /// 2. Monotonically update full consistent vertices partition

    {
        const auto unary_overapproximation_constraints = m_unary_overapproximation_condition.get_numeric_constraints();

        for (ygg::uint_t p = 0; p < num_partitions; ++p)
        {
            auto& full_affected_partition = full_graph.affected_partitions[p];
            auto& delta_affected_partition = delta_graph.affected_partitions[p];
            auto& delta_delta_partition = delta_graph.delta_partitions[p];

            // Implicit case doesnt exist for vertices, so we dont need to check for that here.

            const auto has_runtime_dependency = m_unary_overapproximation_vdg.unary().has_literal_dependency<formalism::FluentTag, formalism::PositiveTag>(p)
                                                || m_unary_overapproximation_vdg.unary().has_literal_dependency<formalism::FluentTag, formalism::NegativeTag>(p)
                                                || m_unary_overapproximation_vdg.unary().has_numeric_dependency(p);

            // Static-only case.
            if (!has_runtime_dependency)
            {
                delta_affected_partition = graph.all(p);
                delta_affected_partition -= full_affected_partition;
                delta_delta_partition = delta_affected_partition;
                full_affected_partition = graph.all(p);
                continue;
            }

            // Runtime-filtered case.
            for (const auto v : graph.part(p))
            {
                if (full_affected_partition.contains(v))
                    continue;

                const auto vertex = vertex_of(p, v);
                if (details::consistent_literals(vertex, m_unary_overapproximation_literals, assignment_sets.fluent_sets().predicate)
                    && details::consistent_numeric_constraints(vertex,
                                                               unary_overapproximation_constraints,
                                                               m_unary_overapproximation_constraints,
                                                               assignment_sets))
                {
                    /// Process delta consistent vertex.
                    full_affected_partition.insert(v);
                    delta_affected_partition.insert(v);
                    delta_delta_partition.insert(v);
                }
            }
        }
    }

    /// 3. Monotonically update full explicitly consistent edges

    {
        const auto binary_overapproximation_constraints = m_binary_overapproximation_condition.get_numeric_constraints();

        for (ygg::uint_t pi = 0; pi < num_partitions; ++pi)
        {
            const auto& full_affected_partition_i = full_graph.affected_partitions[pi];
            auto& delta_affected_partition_i = delta_graph.affected_partitions[pi];
            const auto& delta_delta_partition_i = delta_graph.delta_partitions[pi];

            for (ygg::uint_t pj = pi + 1; pj < num_partitions; ++pj)
            {
                const auto& full_affected_partition_j = full_graph.affected_partitions[pj];
                auto& delta_affected_partition_j = delta_graph.affected_partitions[pj];
                const auto& delta_delta_partition_j = delta_graph.delta_partitions[pj];
                auto& new_edges = delta_graph.scratch[pj];

                if (m_partitioned_adjacency_layout.is_implicit(pi, pj))
                {
                    if (!delta_delta_partition_i.empty())
                        delta_affected_partition_j |= full_affected_partition_j;

                    if (!delta_delta_partition_j.empty())
                        delta_affected_partition_i |= full_affected_partition_i;

                    continue;
                }

                if (m_partitioned_adjacency_layout.is_static_only(pi, pj))
                {
                    const auto target_changed = !delta_delta_partition_j.empty();
                    if (!target_changed && delta_delta_partition_i.empty())
                        continue;
                    for (const auto vi : target_changed ? full_affected_partition_i : delta_delta_partition_i)
                    {
                        urd::intersect_neighbours(graph.rows(),
                                                  vi,
                                                  pj,
                                                  delta_delta_partition_i.contains(vi) ? full_affected_partition_j : delta_delta_partition_j,
                                                  new_edges);
                        if (new_edges.empty())
                            continue;
                        delta_affected_partition_j |= new_edges;
                        delta_affected_partition_i.insert(vi);
                    }
                    continue;
                }

                assert(m_partitioned_adjacency_layout.is_runtime(pi, pj));

                for (const auto vi : full_affected_partition_i)  ///< vi is consistent + delta
                {
                    // The static neighbours of vi among the consistent vertices, without the edges found before.
                    urd::intersect_neighbours(graph.rows(), vi, pj, full_affected_partition_j, new_edges);
                    new_edges -= full_graph.matrix.runtime_neighbours(vi, pj);

                    const auto vertex_i = vertex_of(pi, vi);
                    for (const auto vj : new_edges)
                    {
                        const auto edge = details::Edge(vertex_i, vertex_of(pj, vj));
                        if (details::consistent_literals(edge, m_binary_overapproximation_literals, assignment_sets.fluent_sets().predicate)
                            && details::consistent_numeric_constraints(edge,
                                                                       binary_overapproximation_constraints,
                                                                       m_binary_overapproximation_constraints,
                                                                       assignment_sets))
                        {
                            /// Process delta consistent edge.
                            full_graph.matrix.add_edge(vi, vj);
                            delta_graph.matrix.add_edge(vi, vj);
                            delta_affected_partition_i.insert(vi);
                            delta_affected_partition_j.insert(vj);
                        }
                    }
                }
            }
        }
    }
}

const formalism::datalog::VariableDependencyGraph& StaticConsistencyGraph::get_variable_dependeny_graph() const noexcept
{ return m_binary_overapproximation_vdg; }

const kckp::Graph& StaticConsistencyGraph::get_graph() const noexcept { return m_compatibility_graph; }

const kckp::PartitionedAdjacencyLayout& StaticConsistencyGraph::get_partitioned_adjacency_layout() const noexcept { return m_partitioned_adjacency_layout; }

const details::Literals<formalism::FluentTag>& StaticConsistencyGraph::get_hyperedge_literals() const noexcept { return m_hyperedge_literals; }

std::pair<formalism::datalog::ConjunctiveConditionView<LiftedTag>, bool>
create_overapproximation_conjunctive_condition(size_t k,
                                               formalism::datalog::ConjunctiveConditionView<LiftedTag> condition,
                                               formalism::datalog::Repository& context)
{
    auto builder = formalism::datalog::Builder {};
    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(builder);

    for (const auto variable : condition.get_variables())
        conj_cond->variables.push_back(variable.index());

    const auto literal_belongs_to_k_overapproximation = [k](const auto literal)
    { return literal.get_polarity() ? kckp_arity(literal) >= k : kckp_arity(literal) == k; };
    const auto numeric_constraint_belongs_to_k_overapproximation = [k](const auto& numeric_constraint) { return kckp_arity(numeric_constraint) >= k; };

    for (const auto literal : condition.get_literals<formalism::StaticTag>())
        if (literal_belongs_to_k_overapproximation(literal))
            conj_cond->static_literals.push_back(literal.index());

    for (const auto literal : condition.get_literals<formalism::FluentTag>())
        if (literal_belongs_to_k_overapproximation(literal))
            conj_cond->fluent_literals.push_back(literal.index());

    for (const auto numeric_constraint : condition.get_numeric_constraints())
        if (numeric_constraint_belongs_to_k_overapproximation(numeric_constraint))
            conj_cond->numeric_constraints.push_back(numeric_constraint.index());

    return formalism::datalog::insert(context, *conj_cond);
}

std::pair<formalism::datalog::ConjunctiveConditionView<LiftedTag>, bool>
create_overapproximation_conflicting_conjunctive_condition(size_t k,
                                                           formalism::datalog::ConjunctiveConditionView<LiftedTag> condition,
                                                           formalism::datalog::Repository& context)
{
    auto builder = formalism::datalog::Builder {};
    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(builder);

    for (const auto variable : condition.get_variables())
        conj_cond->variables.push_back(variable.index());

    // Binary KCKP edges join distinct rule parameters, so a one-parameter literal such as p(x,x) still needs an exact check.
    const auto requires_exact_literal_check = [k](const auto literal)
    { return kckp_arity(literal) > k || (k == 2 && kckp_arity(literal) == 2 && parameter_arity(literal) == 1); };
    const auto requires_exact_numeric_check = [k](const auto& numeric_constraint)
    { return kckp_arity(numeric_constraint) > k || (k == 2 && kckp_arity(numeric_constraint) == 2); };

    for (const auto literal : condition.get_literals<formalism::StaticTag>())
        if (requires_exact_literal_check(literal))
            conj_cond->static_literals.push_back(literal.index());

    for (const auto literal : condition.get_literals<formalism::FluentTag>())
        if (requires_exact_literal_check(literal))
            conj_cond->fluent_literals.push_back(literal.index());

    for (const auto numeric_constraint : condition.get_numeric_constraints())
        if (requires_exact_numeric_check(numeric_constraint))
            conj_cond->numeric_constraints.push_back(numeric_constraint.index());

    return formalism::datalog::insert(context, *conj_cond);
}

std::pair<formalism::datalog::ConjunctiveConditionView<LiftedTag>, bool>
create_hyperedge_conjunctive_condition(formalism::datalog::ConjunctiveConditionView<LiftedTag> conflicting_condition, formalism::datalog::Repository& context)
{
    auto builder = formalism::datalog::Builder {};
    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(builder);

    for (const auto variable : conflicting_condition.get_variables())
        conj_cond->variables.push_back(variable.index());

    for (const auto literal : conflicting_condition.get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond->fluent_literals.push_back(literal.index());

    return formalism::datalog::insert(context, *conj_cond);
}

std::pair<formalism::datalog::ConjunctiveConditionView<GroundTag>, bool>
create_ground_nullary_conjunctive_condition(formalism::datalog::ConjunctiveConditionView<LiftedTag> condition, formalism::datalog::Repository& context)
{
    auto builder = formalism::datalog::Builder {};
    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(builder);

    auto binding_empty = urd::flat_vector<urd::index<formalism::Object>> {};
    auto grounder_context = formalism::WriteContext { builder, context, binding_empty };

    const auto is_nullary = [](const auto& element) { return parameter_arity(element) == 0; };

    for (const auto literal : condition.get_literals<formalism::StaticTag>())
        if (is_nullary(literal))
            conj_cond->static_literals.push_back(ground(literal, grounder_context).first.index());

    for (const auto literal : condition.get_literals<formalism::FluentTag>())
        if (is_nullary(literal))
            conj_cond->fluent_literals.push_back(ground(literal, grounder_context).first.index());

    for (const auto numeric_constraint : condition.get_numeric_constraints())
        if (is_nullary(numeric_constraint))
            conj_cond->numeric_constraints.push_back(ground(numeric_constraint, grounder_context).first.index());

    return formalism::datalog::insert(context, *conj_cond);
}
}
