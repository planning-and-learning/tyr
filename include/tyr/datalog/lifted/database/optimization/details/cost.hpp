/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_COST_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_COST_HPP_

#include "tyr/datalog/lifted/database/optimization/details/dpccp.hpp"
#include "tyr/datalog/lifted/database/optimization/details/estimation.hpp"
#include "tyr/datalog/lifted/database/optimization/details/join_block.hpp"
#include "yggdrasil/core/config.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <optional>
#include <urd/bits.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

/// Data-driven planning of a join block whose atoms all have estimates.
namespace tyr::datalog::database::detail
{
/// The most atoms DPccp's bitsets represent.
constexpr size_t max_cost_based_atoms = max_dpccp_vertices;

class CostBasedPlanner
{
    struct Split
    {
        double cost;
        std::uint64_t lhs;
    };
    const JoinBlock* m_block;
    OperatorBuilder* m_build;
    urd::Store<urd::sequence<urd::Store<urd::sequence<urd::index<ygg::Column>>>>> m_variables;
    urd::Store<urd::sequence<urd::index<ygg::Column>>> m_required;
    urd::Store<urd::map<std::uint64_t, Estimate>> m_estimates;
    /// C_out of each subset's best plan and its left part; leaves cost nothing.
    urd::Store<urd::map<std::uint64_t, Split>> m_best;

    [[nodiscard]] urd::Store<urd::sequence<urd::index<ygg::Column>>> labels(std::uint64_t subset) const
    {
        urd::Store<urd::sequence<urd::index<ygg::Column>>> result;
        urd::bits::for_each_set_bit(subset, [&](size_t atom) { result.insert(result.end(), m_variables[atom].begin(), m_variables[atom].end()); });
        return ygg::canonicalized(std::move(result));
    }
    /// The subset's estimate; returned by value since recursion grows the memo.
    Estimate estimate(std::uint64_t subset)
    {
        if (const auto it = m_estimates.find(subset); it != m_estimates.end())
            return it->second;
        const auto lowest = subset & -subset;
        auto result = conjunction(combine(estimate(lowest), estimate(subset ^ lowest)), labels(subset));
        m_estimates.emplace(subset, result);
        return result;
    }
    double rows(std::uint64_t subset)
    {
        const auto it = m_estimates.find(subset);
        return it != m_estimates.end() ? it->second.rows : estimate(subset).rows;
    }
    /// Variables of the subset still needed outside it: output and join variables.
    [[nodiscard]] urd::Store<urd::sequence<urd::index<ygg::Column>>> needed(std::uint64_t subset) const
    {
        urd::Store<urd::sequence<urd::index<ygg::Column>>> result = m_required;
        for (size_t atom = 0; atom < m_variables.size(); ++atom)
            if (!(subset >> atom & 1))
                result.insert(result.end(), m_variables[atom].begin(), m_variables[atom].end());
        return ygg::canonicalized(std::move(result));
    }
    /// A node of the chosen operator tree: a join of its two children, or a leaf (a single operand).
    struct Node
    {
        std::uint64_t subset;
        std::optional<std::pair<size_t, size_t>> children;
    };
    /// A refined node: a multi-way join over the given tree nodes, or unchanged.
    struct Refined
    {
        bool multiway = false;
        urd::Store<urd::sequence<size_t>> inputs;
    };
    urd::Store<urd::sequence<Node>> m_tree;
    urd::Store<urd::sequence<std::optional<Refined>>> m_refined;

    size_t add(Node node)
    {
        m_tree.push_back(node);
        return m_tree.size() - 1;
    }
    /// The DPccp plan of a connected subset as tree nodes.
    size_t tree(std::uint64_t subset)
    {
        if (std::has_single_bit(subset))
            return add({ subset, std::nullopt });
        const auto lhs = m_best.at(subset).lhs;
        const auto left = tree(lhs), right = tree(subset ^ lhs);
        return add({ subset, std::pair(left, right) });
    }
    /// Algorithm 4 of M. Freitag et al., "Adopting Worst-Case Optimal Joins in Relational
    /// Database Systems", PVLDB 13(11), 2020: bottom-up, a join whose estimate exceeds both
    /// inputs, or one with a multi-way input, is collapsed with its inputs into a single
    /// multi-way join, so a growing join and all its ancestors become one multi-way join.
    const Refined& refine(size_t node);
    /// Builds a tree node, projecting it to the variables still needed.
    QueryView<> build(size_t node);

public:
    template<std::invocable<QueryView<>> GetEstimate>
    CostBasedPlanner(const JoinBlock& block, OperatorBuilder& build, GetEstimate estimate_of) : m_block(&block), m_build(&build), m_required(block.required)
    {
        for (size_t atom = 0; atom < block.atoms.size(); ++atom)
        {
            m_variables.push_back(variables(block.atoms[atom]));
            m_estimates.emplace(std::uint64_t { 1 } << atom, estimate_of(block.atoms[atom]));
            m_best.emplace(std::uint64_t { 1 } << atom, Split { 0, 0 });
        }
    }

    /// DPccp (G. Moerkotte, T. Neumann, VLDB 2006) under C_out, the sum of intermediate
    /// result sizes (S. Cluet, G. Moerkotte, ICDT 1995; V. Leis et al., PVLDB 2015), with
    /// System R estimates; connected components are joined last by cross products in
    /// ascending estimated size; then the tree is refined by Algorithm 4 of Freitag et al.
    /// Plans the block; the planner is consumed.
    QueryView<> plan() &&;
};
}  // namespace tyr::datalog::database::detail

#endif
