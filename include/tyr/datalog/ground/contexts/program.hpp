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

#ifndef TYR_DATALOG_GROUND_CONTEXTS_PROGRAM_HPP_
#define TYR_DATALOG_GROUND_CONTEXTS_PROGRAM_HPP_

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct ProgramExecutionContext<GroundTag, AP, TP, CP>
{
    class In
    {
    public:
        explicit In(const ConstProgramWorkspace<GroundTag>& cws) : m_cws(&cws) {}

        [[nodiscard]] auto program() const noexcept { return m_cws->program; }
        [[nodiscard]] const auto& facts() const noexcept { return m_cws->facts; }
        [[nodiscard]] const auto& dependencies() const noexcept { return m_cws->dependencies; }

    private:
        const ConstProgramWorkspace<GroundTag>* m_cws;
    };

    class Out
    {
    public:
        explicit Out(ProgramWorkspace<GroundTag, AP, TP, CP>& ws) : m_ws(&ws) {}

        auto& facts() noexcept { return m_ws->facts; }
        [[nodiscard]] const auto& facts() const noexcept { return m_ws->facts; }
        /// Commits a fact. Whether it was new.
        bool insert(formalism::datalog::PredicateRowIndex<formalism::FluentTag> fact) { return m_ws->facts.insert(fact); }
        auto& annotation_policy() noexcept { return m_ws->annotation_policy; }
        [[nodiscard]] const auto& annotation_policy() const noexcept { return m_ws->annotation_policy; }
        auto& annotations() noexcept { return m_ws->annotations; }
        [[nodiscard]] const auto& annotations() const noexcept { return m_ws->annotations; }
        auto& tp() noexcept { return m_ws->tp; }
        [[nodiscard]] const auto& tp() const noexcept { return m_ws->tp; }
        auto& cost_policy() noexcept { return m_ws->cost_policy; }
        [[nodiscard]] const auto& cost_policy() const noexcept { return m_ws->cost_policy; }
        auto& cost_buckets() noexcept { return m_ws->cost_buckets; }
        auto& pending_achievers() noexcept { return m_ws->pending_achievers; }
        auto& scheduler() noexcept { return m_ws->scheduler; }
        [[nodiscard]] const auto& scheduler() const noexcept { return m_ws->scheduler; }

    private:
        ProgramWorkspace<GroundTag, AP, TP, CP>* m_ws;
    };

    explicit ProgramExecutionContext(ProgramWorkspace<GroundTag, AP, TP, CP>& ws) : m_in(*ws.const_workspace), m_out(ws) { clear(); }

    void clear() { reset_from_current_facts(); }

    void initialize() { reset_from_current_facts(); }

    template<typename Range>
    void initialize(const Range& fluent_atoms)
    {
        m_out.facts().reset();
        auto& facts = m_out.facts().fact_sets;
        for (const auto fterm_value : m_in.program().template get_fterm_values<formalism::FluentTag>())
            insert(facts, fterm_value.get_fterm().get_row().data(), ygg::ClosedInterval<ygg::float_t>(fterm_value.get_value(), fterm_value.get_value()));
        for (const auto atom : fluent_atoms)
            insert(facts, atom.get_row().data());
        initialize();
    }

    [[nodiscard]] const auto& in() const noexcept { return m_in; }
    auto& out() noexcept { return m_out; }
    [[nodiscard]] const auto& out() const noexcept { return m_out; }

private:
    void initialize_annotations()
    {
        m_out.annotations().clear();
        m_out.annotation_policy().clear_achievers();
        m_out.tp().reset();

        const auto& facts = m_out.facts().fact_sets;
        for (ygg::uint_t i = 0; i < facts.predicate.size(); ++i)
            for (const auto id : facts.predicate[i])
                m_out.annotation_policy().initialize_annotation(
                    formalism::datalog::PredicateRowIndex<formalism::FluentTag> { urd::index<formalism::Predicate<formalism::FluentTag>>(i), id },
                    m_out.annotations());
    }

    void reset_from_current_facts()
    {
        m_out.cost_buckets().clear();
        m_out.pending_achievers().clear();
        initialize_annotations();
        const auto fact_sets = FactSets { m_in.facts().fact_sets, m_out.facts().fact_sets };
        m_out.scheduler().reset(fact_sets);
    }

    In m_in;
    Out m_out;
};

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramExecutionContext(ProgramWorkspace<GroundTag, AP, TP, CP>&) -> ProgramExecutionContext<GroundTag, AP, TP, CP>;

}

#endif
