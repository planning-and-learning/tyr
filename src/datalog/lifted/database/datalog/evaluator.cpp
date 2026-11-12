/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/evaluator.hpp"

#include "tyr/datalog/lifted/database/datalog/detail/executor.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/program.hpp"

#include <algorithm>
#include <stdexcept>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog
{
using namespace detail;

struct Evaluator::Impl
{
    const ygg::FreeRelationRepository* layer;
    CompiledProgram program;
    Executor executor;
    urd::Store<urd::sequence<urd::builder<ygg::Relation<ygg::Schema>>>> results;
    urd::Store<urd::sequence<incremental::Delta>> deltas;
    bool ready = false;
    bool initialized = false;
    /// initialize() of an initialized evaluator: input rows are compared with the current ones (in `frontier`).
    bool rebinding = false;
    urd::Store<urd::sequence<ygg::Value>> values;  ///< The output row being encoded.

    Impl(const ygg::FreeRelationRepository& layer_, ProgramView view, std::span<const RelationView> outputs, std::span<const size_t> static_slots) :
        layer(&layer_),
        program(translate(view, outputs, static_slots)),
        executor(program)
    {
        for (const auto table : program.outputs)
        {
            auto columns = urd::Store<urd::sequence<ygg::ColumnLayout>>();
            for (size_t i = 0; i < program.tables[table]->arity; ++i)
                columns.push_back(ygg::ColumnLayout { urd::index<ygg::Column>(ygg::to_uint_t(i)), ygg::type_id<ygg::uint_t> });
            results.emplace_back(std::span<const ygg::ColumnLayout>(columns));
            deltas.emplace_back(*layer, std::span<const ygg::ColumnLayout>(columns));
        }
    }

    /// Full evaluation; `maintained` keeps what update() needs.
    void evaluate(bool maintained)
    {
        for (auto& table : program.tables)
            if (table->slot)
                table->loaded = true;
        for (const auto& stratum : program.strata)
        {
            if (maintained)
                stratum->initialize(program, executor);
            else
                stratum->evaluate(program, executor);
        }
        publish(false);
    }

    /// After an initialized evaluator read its inputs anew: the rows read last but not now are removed, the new ones
    /// added, as an update's changes.
    void apply_rebinding()
    {
        for (const auto table_index : program.slot_tables)
        {
            if (table_index == npos)
                continue;
            auto& table = *program.tables[table_index];
            if (table.is_static && table.loaded)
                continue;
            table.current.for_each(
                [&](std::span<const ygg::uint_t> row)
                {
                    if (!table.frontier.contains(row))
                        table.removed.insert(row);
                });
            table.removed.for_each([&](std::span<const ygg::uint_t> row) { table.current.erase(row); });
            table.added.for_each([&](std::span<const ygg::uint_t> row) { table.current.insert(row); });
            table.frontier.clear();
        }
        rebinding = false;
    }

    void update()
    {
        for (const auto& stratum : program.strata)
            if (std::ranges::any_of(stratum->info().reads, [&](size_t t) { return program.tables[t]->changed(); }))
                stratum->update(program, executor);
        publish(true);
    }

    std::span<const ygg::Value> encode(std::span<const ygg::uint_t> row)
    {
        values.resize(row.size());
        for (size_t c = 0; c < row.size(); ++c)
            values[c] = ygg::encode_value(row[c]);
        return values;
    }
    void publish(bool exact_changes)
    {
        for (size_t i = 0; i < program.outputs.size(); ++i)
        {
            const auto& table = *program.tables[program.outputs[i]];
            auto& result = results[i];
            auto& delta = deltas[i];
            delta.clear();
            if (!exact_changes)
            {
                result.clear();
                table.current.for_each([&](std::span<const ygg::uint_t> row) { result.insert(encode(row)); });
                continue;
            }
            table.removed.for_each(
                [&](std::span<const ygg::uint_t> row)
                {
                    const auto encoded = encode(row);
                    delta.removed.insert(encoded);
                    result.erase(*result.find(encoded));
                });
            table.added.for_each(
                [&](std::span<const ygg::uint_t> row)
                {
                    const auto encoded = encode(row);
                    delta.added.insert(encoded);
                    result.insert(encoded);
                });
        }
    }
};

Evaluator::Evaluator(const ygg::FreeRelationRepository& layer,
                     ProgramView program,
                     std::span<const RelationView> outputs,
                     std::span<const size_t> static_slots) :
    m_impl(std::make_unique<Impl>(layer, program, outputs, static_slots))
{
}

Evaluator::~Evaluator() = default;
Evaluator::Evaluator(Evaluator&&) noexcept = default;
Evaluator& Evaluator::operator=(Evaluator&&) noexcept = default;

bool Evaluator::reads(size_t slot) const
{
    const auto& slots = m_impl->program.slot_tables;
    if (slot >= slots.size() || slots[slot] == npos)
        return false;
    const auto& table = *m_impl->program.tables[slots[slot]];
    return !(table.is_static && table.loaded);
}

size_t Evaluator::input_arity(size_t slot) const { return m_impl->program.tables[m_impl->program.slot_tables[slot]]->arity; }

void Evaluator::begin(bool updating)
{
    auto& impl = *m_impl;
    if (updating && !impl.initialized)
        throw std::logic_error("Datalog evaluator: initialize before update.");
    impl.ready = false;
    impl.rebinding = false;
    for (auto& table : impl.program.tables)
    {
        table->clear_changes();
        if (!updating && table->slot && !(table->is_static && table->loaded))
        {
            if (impl.initialized)
                table->frontier.clear();
            else
                table->current.clear();
        }
    }
    impl.rebinding = !updating && impl.initialized;
}

void Evaluator::input_row(size_t slot, std::span<const ygg::uint_t> row)
{
    auto& table = *m_impl->program.tables[m_impl->program.slot_tables[slot]];
    if (!m_impl->rebinding)
        table.current.insert(row);
    else if (table.frontier.insert(row) && !table.current.contains(row))
        table.added.insert(row);
}

void Evaluator::change_row(size_t slot, bool added, std::span<const ygg::uint_t> row)
{
    auto& table = *m_impl->program.tables[m_impl->program.slot_tables[slot]];
    if (added)
    {
        if (!table.current.insert(row))
            throw std::invalid_argument("Datalog evaluator: an added row is already present.");
        table.added.insert(row);
    }
    else
    {
        if (!table.current.erase(row))
            throw std::invalid_argument("Datalog evaluator: a removed row is absent.");
        table.removed.insert(row);
    }
}

void Evaluator::finish_evaluate()
{
    if (m_impl->rebinding)
        m_impl->apply_rebinding();
    m_impl->initialized = false;
    m_impl->evaluate(false);
    m_impl->ready = true;
}

void Evaluator::finish_initialize()
{
    auto& impl = *m_impl;
    if (impl.rebinding)
    {
        impl.apply_rebinding();
        impl.update();
    }
    else
        impl.evaluate(true);
    impl.initialized = true;
    impl.ready = true;
}

void Evaluator::finish_update()
{
    if (!m_impl->initialized)
        throw std::logic_error("Datalog evaluator: initialize before update.");
    m_impl->update();
    m_impl->ready = true;
}

ygg::FreeBuilderRelationView Evaluator::get_result(size_t output) const
{
    if (!m_impl->ready)
        throw std::logic_error("Datalog evaluator: evaluate before reading results.");
    return m_impl->layer->view(m_impl->results.at(output));
}

const incremental::Delta& Evaluator::get_delta(size_t output) const
{
    if (!m_impl->ready)
        throw std::logic_error("Datalog evaluator: evaluate before reading results.");
    return m_impl->deltas.at(output);
}

size_t Evaluator::memory_usage() const noexcept
{
    size_t result = 0;
    for (const auto& table : m_impl->program.tables)
        result += table->memory_usage();
    for (const auto& stratum : m_impl->program.strata)
        result += stratum->memory_usage();
    for (const auto& output : m_impl->results)
        result += output.memory_usage();
    return result;
}
}  // namespace tyr::datalog::database::datalog
