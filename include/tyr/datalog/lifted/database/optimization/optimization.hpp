/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_OPTIMIZATION_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_OPTIMIZATION_HPP_

#include "tyr/datalog/lifted/database/optimization/details/optimization.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/optimization/statistics.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include "yggdrasil/diagnostics/warnings.hpp"

#include <algorithm>
#include <concepts>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace tyr::datalog::database
{
/// Measures row and per-column distinct counts of the relations bound to each input slot.
template<RelationViewRange R>
Statistics collect_statistics(const R& inputs)
{
    Statistics result;
    size_t slot = 0;
    for (const auto& input : inputs)
        result.inputs[slot++] = measure_relation(input);
    return result;
}

/// Averages relation statistics over sample states (e.g. states of training tasks) into one estimate per key.
template<typename Key>
class StatisticsAccumulator
{
public:
    /// Adds one sample of key's relation.
    template<RelationViewConcept Relation>
    void add(const Key& key, const Relation& relation)
    {
        auto& total = m_totals[key];
        const auto sample = measure_relation(relation);
        ++total.samples;
        total.statistics.rows += sample.rows;
        for (const auto& [column, distinct] : sample.distinct)
            total.statistics.distinct[column] += distinct;
    }

    /// Means of rows and per-column distinct counts. A nonempty mean is at least one row, so distinct counts lie in
    /// [1, rows]. A key empty in every sample keeps 0 rows and adds a warning naming it with describe(key).
    template<typename Describe>
        requires std::invocable<Describe&, const Key&> && std::convertible_to<std::invoke_result_t<Describe&, const Key&>, std::string>
    urd::Store<urd::map<Key, RelationStatistics>> finish(::ygg::diagnostics::Warnings& warnings, Describe describe) const
    {
        urd::Store<urd::map<Key, RelationStatistics>> result;
        for (const auto& [key, total] : m_totals)
        {
            const auto samples = static_cast<double>(total.samples);
            auto mean = total.statistics;
            if (mean.rows == 0)
                warnings.add(::ygg::diagnostics::Diagnostic { "Statistics: " + std::string(describe(key)) + " was empty in all " + std::to_string(total.samples)
                                                       + " samples; its size is estimated as 0." });
            mean.rows = mean.rows > 0 ? std::max(mean.rows / samples, 1.0) : 0.0;
            for (const auto& [column, distinct] : mean.distinct)
                mean.distinct.at(column) = mean.rows > 0 ? std::clamp(distinct / samples, 1.0, mean.rows) : 0.0;
            result.emplace(key, std::move(mean));
        }
        return result;
    }

private:
    struct Total
    {
        size_t samples = 0;
        RelationStatistics statistics;
    };
    urd::Store<urd::map<Key, Total>> m_totals;
};

/// Plans the roots by applying established query optimization methods:
///
/// 1. The query is normalized by the textbook algebraic rewrites [GMUW08]: selections
///    are pushed down through joins, projections, renames, unions, and differences,
///    projections through renames and unions, and empty relations are propagated.
/// 2. Each maximal tree of joins forms a join block, planned as a whole. Unions,
///    differences, distances, and selections spanning several join operands bound
///    the blocks.
/// 3. A block whose operands all have known cardinalities is planned cost-based:
///    DPccp [MN06] finds the bushy join order of least C_out [CM95, Leis15] under
///    System R cardinality estimates [Selinger79] (the textbook estimates of [GMUW08]
///    for projection, union, and difference), with observed cardinalities replacing
///    estimates [LEO01]; disconnected components are joined last by cross products.
///    The join tree is then refined by Algorithm 4 of [Freitag20]: growing joins and
///    their ancestors become one worst-case-optimal join.
/// 4. Any other block is planned from its structure: if the GYO reduction
///    [Graham79, YO79] shows it acyclic, by Yannakakis' algorithm [Yannakakis81];
///    otherwise by Generic Join [NRR13], whose running time is bounded by the AGM
///    bound [AGM08].
///
/// Generic Join's variable order is fixed by column label: any order attains the
/// worst-case bound, but [Freitag20] additionally optimizes it with the Tributary Join
/// cost model, which is not implemented. Distances have no estimate in the literature
/// and are only known when observed. Not modeled: distributing joins over unions,
/// sharing-aware choices across roots, and costs of incremental maintenance.
///
/// Steps 2 to 4 assume full evaluation. A root evaluated bound only gets step 1: the
/// bound evaluator flattens its query and orders the joins per binding from the bound
/// columns, so a plan made for full materialization (for instance Yannakakis'
/// semijoins) would only add atoms to every binding's plan.
///
/// The roots belong to one repository; the plan preserves every root's ordered, typed
/// schema and set semantics.
///
/// [Selinger79] P. G. Selinger et al., Access Path Selection in a Relational Database
///     Management System, SIGMOD 1979.
/// [GMUW08] H. Garcia-Molina, J. D. Ullman, J. Widom, Database Systems: The Complete
///     Book, 2nd ed., 2008, ch. 16.
/// [Graham79] M. H. Graham, On the Universal Relation, University of Toronto, 1979.
/// [YO79] C. T. Yu, M. Z. Özsoyoğlu, An Algorithm for Tree-Query Membership of a
///     Distributed Query, COMPSAC 1979.
/// [Yannakakis81] M. Yannakakis, Algorithms for Acyclic Database Schemes, VLDB 1981.
/// [AGM08] A. Atserias, M. Grohe, D. Marx, Size Bounds and Query Plans for Relational
///     Joins, FOCS 2008.
/// [NRR13] H. Q. Ngo, C. Ré, A. Rudra, Skew Strikes Back: New Developments in the
///     Theory of Join Algorithms, SIGMOD Record 2013.
/// [MN06] G. Moerkotte, T. Neumann, Analysis of Two Existing and One New Dynamic
///     Programming Algorithm for the Generation of Optimal Bushy Join Trees without
///     Cross Products, VLDB 2006.
/// [CM95] S. Cluet, G. Moerkotte, On the Complexity of Generating Optimal Left-Deep
///     Processing Trees with Cross Products, ICDT 1995.
/// [Leis15] V. Leis et al., How Good Are Query Optimizers, Really?, PVLDB 9(3), 2015.
/// [Freitag20] M. Freitag, M. Bandle, T. Schmidt, A. Kemper, T. Neumann, Adopting
///     Worst-Case Optimal Joins in Relational Database Systems, PVLDB 13(11), 2020.
/// [LEO01] M. Stillger, G. Lohman, V. Markl, M. Kandil, LEO – DB2's LEarning Optimizer,
///     VLDB 2001.
inline QueryPlan optimize(std::span<const QueryView<>> roots, std::span<const Evaluation> evaluations, const std::type_identity_t<Statistics>& statistics = {})
{ return detail::optimize(roots, evaluations, statistics); }

/// Plans roots that are all evaluated fully.
inline QueryPlan optimize(std::span<const QueryView<>> roots, const std::type_identity_t<Statistics>& statistics = {})
{
    const auto evaluations = urd::Store<urd::sequence<Evaluation>>(roots.size(), Evaluation::full);
    return optimize(roots, std::span<const Evaluation>(evaluations), statistics);
}

inline QueryPlan optimize(QueryView<> root, const std::type_identity_t<Statistics>& statistics = {})
{ return optimize(std::span<const QueryView<>>(&root, 1), statistics); }

template<std::ranges::contiguous_range R>
    requires std::same_as<std::ranges::range_value_t<R>, QueryView<>>
QueryPlan optimize(const R& roots, const std::type_identity_t<Statistics>& statistics = {})
{ return optimize(std::span<const QueryView<>>(roots), statistics); }
}  // namespace tyr::datalog::database
#endif
