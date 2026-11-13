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

#ifndef TYR_DATALOG_CONSISTENCY_GRAPH_HPP_
#define TYR_DATALOG_CONSISTENCY_GRAPH_HPP_

#include "tyr/algorithms/kckp/delta_kckp.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/kckp/assignment_sets.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
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

#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::datalog
{
class StaticConsistencyGraph;

namespace details
{
/**
 * What a rule's condition checks against the assignment sets
 *
 * Fluent assignment sets are rebuilt once per state and shared across rules. A rule keeps, per element of its condition
 * (a literal's atom or a function term), the element's terms, and its literals filed by the parameters they have, so
 * that a vertex or an edge of the consistency graph reads the assignments it makes from the shared sets.
 */

/// A term of an element: its position, and what is there, a rule parameter or a constant.
struct Term
{
    ygg::uint_t position = 0;
    ygg::uint_t value = 0;  ///< the parameter's index, or the constant's object index
    bool constant = false;

    friend bool operator==(const Term&, const Term&) = default;
};

/// Per element its terms, in position order; or per filing key its element numbers: rows stored compressed.
template<typename T>
using Rows = urd::Store<urd::sequence<urd::Store<urd::sequence<T>, urd::frozen>>>;

template<formalism::FactKind T>
struct LiteralHead
{
    urd::index<formalism::Predicate<T>> predicate;
    bool polarity = false;
};

/// A condition's literals of fact kind T, numbered in its literal order, and filed by their parameters: a vertex of
/// parameter p reads the literals at by_parameter[p]; an edge of p < q those at by_pair[p * arity + q], the literals
/// with p and q, or with p or q and a constant.
template<formalism::FactKind T>
struct Literals
{
    urd::Store<urd::sequence<LiteralHead<T>>> heads;
    Rows<Term> terms;
    Rows<ygg::uint_t> by_parameter;
    Rows<ygg::uint_t> by_pair;
    size_t arity = 0;
};

/// A numeric constraint's function terms of fact kind T: its terms and function, by the function term.
template<formalism::FactKind T>
struct FunctionTerms
{
    urd::Store<urd::map<urd::index<formalism::FunctionTerm<LiftedTag, T>>, ygg::uint_t>> number;
    urd::Store<urd::sequence<urd::index<formalism::Function<T>>>> functions;
    Rows<Term> terms;
};

struct ConstraintTerms
{
    FunctionTerms<formalism::StaticTag> static_terms;
    FunctionTerms<formalism::FluentTag> fluent_terms;
};

/**
 * Vertex
 */

/// @brief A vertex [parameter_index/object_index] in the consistency graph.
class Vertex
{
private:
    formalism::ParameterIndex m_parameter_index;
    urd::index<formalism::Object> m_object_index;

public:
    Vertex(formalism::ParameterIndex parameter_index, urd::index<formalism::Object> object_index) noexcept :
        m_parameter_index(parameter_index),
        m_object_index(object_index)
    {
    }

    [[nodiscard]] auto get_parameter_index() const noexcept { return m_parameter_index; }
    [[nodiscard]] auto get_object_index() const noexcept { return m_object_index; }
};

/**
 * Edge
 */

class Edge
{
private:
    Vertex m_vi;
    Vertex m_vj;

public:
    Edge(Vertex vi, Vertex vj) noexcept : m_vi(vi), m_vj(vj) {}

    [[nodiscard]] const auto& vi() const noexcept { return m_vi; }
    [[nodiscard]] const auto& vj() const noexcept { return m_vj; }
};

}

class StaticConsistencyGraph
{
public:
    StaticConsistencyGraph(formalism::datalog::ConjunctiveConditionView<LiftedTag> unary_overapproximation_condition,
                           formalism::datalog::ConjunctiveConditionView<LiftedTag> binary_overapproximation_condition,
                           formalism::datalog::ConjunctiveConditionView<LiftedTag> hyperedge_condition,
                           kckp::Graph compatibility_graph);

    void initialize_dynamic_consistency_graphs(const AssignmentSets& assignment_sets, kckp::DeltaGraph& delta_graph, kckp::FullGraph& full_graph) const;

    [[nodiscard]] const formalism::datalog::VariableDependencyGraph& get_variable_dependeny_graph() const noexcept;
    [[nodiscard]] const kckp::Graph& get_graph() const noexcept;
    [[nodiscard]] const kckp::PartitionedAdjacencyLayout& get_partitioned_adjacency_layout() const noexcept;
    /// The hyperedge condition's literals, in its literal order.
    [[nodiscard]] const details::Literals<formalism::FluentTag>& get_hyperedge_literals() const noexcept;

private:
    formalism::datalog::ConjunctiveConditionView<LiftedTag> m_unary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> m_binary_overapproximation_condition;

    formalism::datalog::VariableDependencyGraph m_unary_overapproximation_vdg;
    formalism::datalog::VariableDependencyGraph m_binary_overapproximation_vdg;

    kckp::Graph m_compatibility_graph;
    kckp::PartitionedAdjacencyLayout m_partitioned_adjacency_layout;

    details::Literals<formalism::FluentTag> m_unary_overapproximation_literals;
    details::Literals<formalism::FluentTag> m_binary_overapproximation_literals;
    details::Literals<formalism::FluentTag> m_hyperedge_literals;

    urd::Store<urd::sequence<details::ConstraintTerms>> m_unary_overapproximation_constraints;
    urd::Store<urd::sequence<details::ConstraintTerms>> m_binary_overapproximation_constraints;
};

extern std::pair<formalism::datalog::ConjunctiveConditionView<GroundTag>, bool>
create_ground_nullary_conjunctive_condition(formalism::datalog::ConjunctiveConditionView<LiftedTag> condition, formalism::datalog::Repository& context);

extern std::pair<formalism::datalog::ConjunctiveConditionView<LiftedTag>, bool>
create_overapproximation_conjunctive_condition(size_t k,
                                               formalism::datalog::ConjunctiveConditionView<LiftedTag> condition,
                                               formalism::datalog::Repository& context);

extern std::pair<formalism::datalog::ConjunctiveConditionView<LiftedTag>, bool>
create_overapproximation_conflicting_conjunctive_condition(size_t k,
                                                           formalism::datalog::ConjunctiveConditionView<LiftedTag> condition,
                                                           formalism::datalog::Repository& context);

/// The positive fluent literals of the conflicting condition: a new atom of one anchors the cliques it makes applicable.
extern std::pair<formalism::datalog::ConjunctiveConditionView<LiftedTag>, bool>
create_hyperedge_conjunctive_condition(formalism::datalog::ConjunctiveConditionView<LiftedTag> conflicting_condition, formalism::datalog::Repository& context);

}

#endif
