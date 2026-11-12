/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_EVALUATOR_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_EVALUATOR_HPP_

#include "tyr/datalog/lifted/database/datalog/repository.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog
{
/// Evaluates a program rule by rule over indexed relations, without intermediate relations: each rule is an index
/// nested-loop join compiled once per literal that reads a change (semi-naive evaluation, counting, DRed). Atoms that
/// a step's new variables complete are intersected, the one with the fewest matches enumerating (Generic Join
/// [Ngo13]), so cyclic bodies such as triangles stay worst-case optimal. Relations are strata of strongly connected
/// components (stratification.hpp), evaluated in dependency order:
///
/// - evaluate(): a stratum without recursion in one pass, a recursive one semi-naively, a rule once per atom of the
///   stratum reading the rows found last;
/// - update(), after initialize(): a stratum without recursion keeps derivation counts, adjusted by the rule with one
///   changed literal reading its added or removed rows, earlier literals the new and later ones the old relations
///   [Gupta93, counting]; a recursive stratum uses Delete and Rederive [Gupta93]: delete every row with a derivation
///   through a removed row (with the old relations), rederive the deleted rows that the remaining rows still derive
///   (the rule with its head bound), then insert semi-naively from the additions and the rederived rows.
///
/// Only the strata the requested outputs depend on are evaluated, and only the input slots they read are read.
///
/// An aggregate reads a relation of the satisfying assignments of its body, kept by counting, whose groups it keeps
/// incrementally; COUNT and SUM are 0 for a group without rows. Columns are uint_t; relation i of the outputs has
/// columns labeled 0, ..., arity - 1. Input slot s is binding s; static slots are read by the first evaluate() or
/// initialize() only.
///
/// [Gupta93] A. Gupta, I. S. Mumick, V. S. Subrahmanian, Maintaining Views Incrementally, SIGMOD 1993.
/// [Ngo13] H. Q. Ngo, C. Ré, A. Rudra, Skew Strikes Back: New Developments in the Theory of Join Algorithms, SIGMOD
///   Record 2013.
class Evaluator
{
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    urd::Store<urd::sequence<ygg::uint_t>> m_row;  ///< The input row being decoded.

    [[nodiscard]] size_t input_arity(size_t slot) const;
    /// Starts reading inputs anew, or (`updating`) their changes.
    void begin(bool updating);
    void input_row(size_t slot, std::span<const ygg::uint_t> row);
    void change_row(size_t slot, bool added, std::span<const ygg::uint_t> row);
    void finish_evaluate();
    void finish_initialize();
    void finish_update();

    template<RelationViewConcept V, typename F>
    void decode(size_t slot, const V& relation, const F& f)
    {
        const auto arity = input_arity(slot);
        if (relation.columns().size() != arity)
            throw std::invalid_argument("Datalog evaluator: an input does not have its relation's arity.");
        for (const auto& column : relation.columns())
            if (column.type != ygg::type_id<ygg::uint_t>)
                throw std::invalid_argument("Datalog evaluator: input columns must be uint_t.");
        auto& row = m_row;
        row.resize(arity);
        for (size_t i = 0; i < relation.size(); ++i)
        {
            const auto values = relation.row(i).get_values();
            for (size_t c = 0; c < arity; ++c)
                row[c] = ygg::decode_value<ygg::uint_t>(values.get(c));
            f(std::span<const ygg::uint_t>(row));
        }
    }
    template<RelationViewRange R>
    void read_inputs(const R& inputs)
    {
        begin(false);
        for (size_t slot = 0; slot < std::ranges::size(inputs); ++slot)
            if (reads(slot))
                decode(slot, inputs[slot], [&](std::span<const ygg::uint_t> row) { input_row(slot, row); });
    }

public:
    /// Whether a requested relation depends on the input slot; the others are not read.
    [[nodiscard]] bool reads(size_t slot) const;
    /// Evaluates the requested relations of a program; static slots hold the same rows in every call. Its results are
    /// read through `layer`, which must outlive it.
    Evaluator(const ygg::FreeRelationRepository& layer, ProgramView program, std::span<const RelationView> outputs, std::span<const size_t> static_slots = {});
    ~Evaluator();
    Evaluator(const Evaluator&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
    Evaluator(Evaluator&&) noexcept;
    Evaluator& operator=(Evaluator&&) noexcept;

    /// Full evaluation of the inputs.
    template<RelationViewRange R>
    void evaluate(const R& inputs)
    {
        read_inputs(inputs);
        finish_evaluate();
    }
    /// Full evaluation that keeps what update() maintains. Once initialized, the inputs are compared with the rows read
    /// last and the difference is applied as by update(), so that what the strata maintain (search trees included)
    /// carries over.
    template<RelationViewRange R>
    void initialize(const R& inputs)
    {
        read_inputs(inputs);
        finish_initialize();
    }
    /// One exact (added, removed) change per input slot; static slots must not change.
    template<incremental::RelationDeltaRange R>
    void update(const R& changes)
    {
        begin(true);
        for (size_t slot = 0; slot < std::ranges::size(changes); ++slot)
            if (reads(slot))
            {
                const auto& change = changes[slot];
                const auto& added = std::get<0>(change);
                const auto& removed = std::get<1>(change);
                decode(slot, removed, [&](std::span<const ygg::uint_t> row) { change_row(slot, false, row); });
                decode(slot, added, [&](std::span<const ygg::uint_t> row) { change_row(slot, true, row); });
            }
        finish_update();
    }

    [[nodiscard]] ygg::FreeBuilderRelationView get_result(size_t output = 0) const;
    /// The last update's exact change of an output.
    [[nodiscard]] const incremental::Delta& get_delta(size_t output = 0) const;
    [[nodiscard]] size_t memory_usage() const noexcept;
};
}  // namespace tyr::datalog::database::datalog

#endif
