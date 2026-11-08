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
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
class StaticConsistencyGraph;

namespace details
{
/**
 * For mapping rule bindings to literal bindings
 *
 * Fluent assignment sets are rebuilt once per state and shared across rules. These rule-specific mappings translate vertices and edges into those shared sets
 * without materializing a new compatibility table for every rule and state.
 */

struct RuleToLiteralInfoMappings
{
    // For building vertex assignments (p/o)
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> parameter_to_infos;

    // For building edge assignments (p/o,q/c)
    urd::Store<urd::sequence<urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>>> parameter_pairs_to_infos;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> parameter_to_infos_with_constants;

    // For global vertex assignments (c) for constant c
    urd::Store<urd::sequence<ygg::uint_t>> infos_with_constants;
    // For global edge assignments (c,c') for constants c,c'
    urd::Store<urd::sequence<ygg::uint_t>> infos_with_constant_pairs;
};

struct RuleToLiteralPositionMappings
{
    urd::Store<urd::sequence<std::pair<ygg::uint_t, urd::index<formalism::Object>>>> constant_positions;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> parameter_to_positions;
};

template<formalism::FactKind T>
struct RuleToLiteralInfo
{
    urd::index<formalism::Predicate<T>> predicate;
    bool polarity = false;
    size_t kckp_arity = 0;
    size_t num_parameters = 0;
    size_t num_constants = 0;

    RuleToLiteralPositionMappings position_mappings;
};

template<formalism::FactKind T>
struct TaggedRuleToLiteralInfos
{
    urd::Store<urd::sequence<RuleToLiteralInfo<T>>> infos;

    RuleToLiteralInfoMappings info_mappings;
};

template<formalism::FactKind T>
struct RuleToFunctionTermInfo
{
    urd::index<formalism::Function<T>> function;
    size_t kckp_arity = 0;
    size_t num_parameters = 0;
    size_t num_constants = 0;

    RuleToLiteralPositionMappings position_mappings;
};

template<formalism::FactKind T>
struct TaggedRuleToFunctionTermInfos
{
    urd::Store<urd::map<urd::index<formalism::FunctionTerm<LiftedTag, T>>, RuleToFunctionTermInfo<T>>> infos;

    RuleToLiteralInfoMappings info_mappings;
};

struct RuleToConstraintInfo
{
    TaggedRuleToFunctionTermInfos<formalism::StaticTag> static_infos;
    TaggedRuleToFunctionTermInfos<formalism::FluentTag> fluent_infos;

    size_t kckp_arity = 0;
};

struct RuleToRuleToConstraintInfos
{
    urd::Store<urd::sequence<RuleToConstraintInfo>> infos;
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

    void initialize_dynamic_consistency_graphs(const AssignmentSets& assignment_sets,
                                               const kckp::GraphLayout& layout,
                                               kckp::DeltaGraph& delta_graph,
                                               kckp::FullGraph& full_graph) const;

    [[nodiscard]] const formalism::datalog::VariableDependencyGraph& get_variable_dependeny_graph() const noexcept;
    [[nodiscard]] const kckp::Graph& get_graph() const noexcept;
    [[nodiscard]] const kckp::GraphLayout& get_graph_layout() const noexcept;
    [[nodiscard]] const kckp::PartitionedAdjacencyLayout& get_partitioned_adjacency_layout() const noexcept;
    [[nodiscard]] const kckp::DeduplicatedAdjacencyMatrix& get_adjacency_matrix() const noexcept;
    /// Term positions of the hyperedge condition's literals, in its literal order.
    [[nodiscard]] const details::TaggedRuleToLiteralInfos<formalism::FluentTag>& get_hyperedge_indexed_literals() const noexcept;

private:
    formalism::datalog::ConjunctiveConditionView<LiftedTag> m_unary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> m_binary_overapproximation_condition;

    formalism::datalog::VariableDependencyGraph m_unary_overapproximation_vdg;
    formalism::datalog::VariableDependencyGraph m_binary_overapproximation_vdg;

    kckp::Graph m_compatibility_graph;
    kckp::PartitionedAdjacencyLayout m_partitioned_adjacency_layout;

    details::TaggedRuleToLiteralInfos<formalism::FluentTag> m_unary_overapproximation_indexed_literals;
    details::TaggedRuleToLiteralInfos<formalism::FluentTag> m_binary_overapproximation_indexed_literals;
    details::TaggedRuleToLiteralInfos<formalism::FluentTag> m_hyperedge_indexed_literals;

    details::RuleToRuleToConstraintInfos m_unary_overapproximation_indexed_constraints;
    details::RuleToRuleToConstraintInfos m_binary_overapproximation_indexed_constraints;
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
