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
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <vector>
#include <yggdrasil/containers/vector.hpp>

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
    std::vector<std::vector<ygg::uint_t>> parameter_to_infos;

    // For building edge assignments (p/o,q/c)
    std::vector<std::vector<std::vector<ygg::uint_t>>> parameter_pairs_to_infos;
    std::vector<std::vector<ygg::uint_t>> parameter_to_infos_with_constants;

    // For global vertex assignments (c) for constant c
    std::vector<ygg::uint_t> infos_with_constants;
    // For global edge assignments (c,c') for constants c,c'
    std::vector<ygg::uint_t> infos_with_constant_pairs;
};

struct RuleToLiteralPositionMappings
{
    std::vector<std::pair<ygg::uint_t, ygg::Index<::tyr::formalism::Object>>> constant_positions;
    std::vector<std::vector<ygg::uint_t>> parameter_to_positions;
};

template<::tyr::formalism::FactKind T>
struct RuleToLiteralInfo
{
    ygg::Index<::tyr::formalism::Predicate<T>> predicate;
    bool polarity;
    size_t kckp_arity;
    size_t num_parameters;
    size_t num_constants;

    RuleToLiteralPositionMappings position_mappings;
};

template<::tyr::formalism::FactKind T>
struct TaggedRuleToLiteralInfos
{
    std::vector<RuleToLiteralInfo<T>> infos;

    RuleToLiteralInfoMappings info_mappings;
};

template<::tyr::formalism::FactKind T>
struct RuleToFunctionTermInfo
{
    ygg::Index<::tyr::formalism::Function<T>> function;
    size_t kckp_arity;
    size_t num_parameters;
    size_t num_constants;

    RuleToLiteralPositionMappings position_mappings;
};

template<::tyr::formalism::FactKind T>
struct TaggedRuleToFunctionTermInfos
{
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::datalog::FunctionTerm<::tyr::LiftedTag, T>>, RuleToFunctionTermInfo<T>> infos;

    RuleToLiteralInfoMappings info_mappings;
};

struct RuleToConstraintInfo
{
    TaggedRuleToFunctionTermInfos<::tyr::formalism::StaticTag> static_infos;
    TaggedRuleToFunctionTermInfos<::tyr::formalism::FluentTag> fluent_infos;

    size_t kckp_arity;

    template<::tyr::formalism::FactKind T>
    const auto& get() const noexcept
    {
        if constexpr (std::is_same_v<T, ::tyr::formalism::StaticTag>)
            return static_infos;
        else if constexpr (std::is_same_v<T, ::tyr::formalism::FluentTag>)
            return fluent_infos;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }
};

struct RuleToRuleToConstraintInfos
{
    std::vector<RuleToConstraintInfo> infos;
};

/**
 * Vertex
 */

/// @brief A vertex [parameter_index/object_index] in the consistency graph.
class Vertex
{
private:
    ::tyr::formalism::ParameterIndex m_parameter_index;
    ygg::Index<::tyr::formalism::Object> m_object_index;

public:
    Vertex(::tyr::formalism::ParameterIndex parameter_index, ygg::Index<::tyr::formalism::Object> object_index) noexcept :
        m_parameter_index(parameter_index),
        m_object_index(object_index)
    {
    }

    auto get_parameter_index() const noexcept { return m_parameter_index; }
    auto get_object_index() const noexcept { return m_object_index; }
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
    Edge(Vertex vi, Vertex vj) noexcept : m_vi(std::move(vi)), m_vj(std::move(vj)) {}

    const auto& vi() const noexcept { return m_vi; }
    const auto& vj() const noexcept { return m_vj; }
};

}

class StaticConsistencyGraph
{
public:
    StaticConsistencyGraph(::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> unary_overapproximation_condition,
                           ::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> binary_overapproximation_condition,
                           kckp::Graph compatibility_graph);

    void initialize_dynamic_consistency_graphs(const AssignmentSets& assignment_sets,
                                               const kckp::GraphLayout& layout,
                                               kckp::DeltaGraph& delta_graph,
                                               kckp::FullGraph& full_graph) const;

    const ::tyr::formalism::datalog::VariableDependencyGraph& get_variable_dependeny_graph() const noexcept;
    const kckp::Graph& get_graph() const noexcept;
    const kckp::GraphLayout& get_graph_layout() const noexcept;
    const kckp::PartitionedAdjacencyLayout& get_partitioned_adjacency_layout() const noexcept;
    const kckp::DeduplicatedAdjacencyMatrix& get_adjacency_matrix() const noexcept;

private:
    ::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> m_unary_overapproximation_condition;
    ::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> m_binary_overapproximation_condition;

    ::tyr::formalism::datalog::VariableDependencyGraph m_unary_overapproximation_vdg;
    ::tyr::formalism::datalog::VariableDependencyGraph m_binary_overapproximation_vdg;

    kckp::Graph m_compatibility_graph;
    kckp::PartitionedAdjacencyLayout m_partitioned_adjacency_layout;

    details::TaggedRuleToLiteralInfos<::tyr::formalism::FluentTag> m_unary_overapproximation_indexed_literals;
    details::TaggedRuleToLiteralInfos<::tyr::formalism::FluentTag> m_binary_overapproximation_indexed_literals;

    details::RuleToRuleToConstraintInfos m_unary_overapproximation_indexed_constraints;
    details::RuleToRuleToConstraintInfos m_binary_overapproximation_indexed_constraints;
};

extern std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
create_ground_nullary_conjunctive_condition(::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> condition, ::tyr::formalism::datalog::Repository& context);

extern std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag>, bool>
create_overapproximation_conjunctive_condition(size_t k,
                                               ::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> condition,
                                               ::tyr::formalism::datalog::Repository& context);

extern std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag>, bool>
create_overapproximation_conflicting_conjunctive_condition(size_t k,
                                                           ::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> condition,
                                                           ::tyr::formalism::datalog::Repository& context);

}

#endif
