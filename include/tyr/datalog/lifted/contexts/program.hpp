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

#ifndef TYR_DATALOG_LIFTED_CONTEXTS_PROGRAM_HPP_
#define TYR_DATALOG_LIFTED_CONTEXTS_PROGRAM_HPP_

#include "tyr/datalog/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

namespace tyr::datalog
{

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct ProgramExecutionContext<LiftedTag, AP, TP, CP>
{
    class In
    {
    public:
        explicit In(const ConstProgramWorkspace<LiftedTag>& cws) : m_cws(&cws) {}

        [[nodiscard]] const auto& facts() const noexcept { return m_cws->facts; }
        [[nodiscard]] const auto& engine() const noexcept { return m_cws->engine; }

    private:
        const ConstProgramWorkspace<LiftedTag>* m_cws;
    };

    class Out
    {
    public:
        Out(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws, urd::Store<urd::sequence<Scheduler<LiftedTag>>>& schedulers) : m_ws(&ws), m_schedulers(&schedulers) {}

        auto& facts() noexcept { return m_ws->facts; }
        [[nodiscard]] const auto& facts() const noexcept { return m_ws->facts; }
        auto& engine() noexcept { return m_ws->engine; }
        [[nodiscard]] const auto& engine() const noexcept { return m_ws->engine; }
        /// Commits a fact: into the facts, then into the engine's index over them. Whether it was new.
        bool insert(formalism::datalog::PredicateRowIndex<formalism::FluentTag> fact)
        {
            if (!m_ws->facts.insert(fact))
                return false;
            m_ws->engine.assignment_sets.predicate.insert(urd::resolve(fact, *m_ws->workspace_repository));
            return true;
        }
        bool insert(formalism::datalog::FunctionRowIndex<formalism::FluentTag> fact, const ygg::ClosedInterval<ygg::float_t>& interval)
        {
            if (!m_ws->facts.insert(fact, interval))
                return false;
            m_ws->engine.assignment_sets.function.insert(urd::resolve(fact, *m_ws->workspace_repository), interval);
            return true;
        }
        auto& annotation_policy() noexcept { return m_ws->annotation_policy; }
        [[nodiscard]] const auto& annotation_policy() const noexcept { return m_ws->annotation_policy; }
        auto& annotations() noexcept { return m_ws->annotations; }
        [[nodiscard]] const auto& annotations() const noexcept { return m_ws->annotations; }
        auto& numeric_annotations() noexcept { return m_ws->numeric_annotations; }
        [[nodiscard]] const auto& numeric_annotations() const noexcept { return m_ws->numeric_annotations; }
        auto& delta_annotations() noexcept { return m_ws->delta_annotations; }
        [[nodiscard]] const auto& delta_annotations() const noexcept { return m_ws->delta_annotations; }
        auto& delta_numeric_annotations() noexcept { return m_ws->delta_numeric_annotations; }
        [[nodiscard]] const auto& delta_numeric_annotations() const noexcept { return m_ws->delta_numeric_annotations; }
        [[nodiscard]] const auto& numeric_support_selector() const noexcept { return m_ws->get_numeric_support_selector(); }
        void rebuild_numeric_support_selector(const TaggedFactSets<formalism::StaticTag>& static_fact_sets)
        { m_ws->numeric_support_selector.emplace(FactSets { static_fact_sets, m_ws->facts.fact_sets }, m_ws->numeric_annotations, !AP::stores_annotations); }
        void reset_numeric_support_selector() noexcept { m_ws->numeric_support_selector.reset(); }
        auto& tp() noexcept { return m_ws->tp; }
        [[nodiscard]] const auto& tp() const noexcept { return m_ws->tp; }
        auto& cost_policy() noexcept { return m_ws->cost_policy; }
        [[nodiscard]] const auto& cost_policy() const noexcept { return m_ws->cost_policy; }
        auto& cost_buckets() noexcept { return m_ws->cost_buckets; }
        auto& pending_achievers() noexcept { return m_ws->pending_achievers; }
        auto& datalog_builder() noexcept { return m_ws->datalog_builder; }
        [[nodiscard]] const auto& datalog_builder() const noexcept { return m_ws->datalog_builder; }
        auto& workspace_repository() noexcept { return *m_ws->workspace_repository; }
        [[nodiscard]] const auto& workspace_repository() const noexcept { return *m_ws->workspace_repository; }
        auto& schedulers() noexcept { return *m_schedulers; }
        [[nodiscard]] const auto& schedulers() const noexcept { return *m_schedulers; }
        auto& statistics() noexcept { return m_ws->statistics; }
        [[nodiscard]] const auto& statistics() const noexcept { return m_ws->statistics; }

    private:
        ProgramWorkspace<LiftedTag, AP, TP, CP>* m_ws;
        urd::Store<urd::sequence<Scheduler<LiftedTag>>>* m_schedulers;
    };

    explicit ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws, size_t num_threads = 1) :
        ProgramExecutionContext(ws, ws.schedulers, num_threads)
    {
    }

    /// Schedulers must use the workspace's repository and a subset of its rules.
    ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws, urd::Store<urd::sequence<Scheduler<LiftedTag>>>& schedulers, size_t num_threads = 1) :
        ProgramExecutionContext(In(*ws.const_workspace), Out(ws, schedulers), num_threads)
    { clear(); }

    /// The caller has reset the workspace and inserted this evaluation's input facts.
    static ProgramExecutionContext from_reset_workspace(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws, size_t num_threads = 1)
    { return from_reset_workspace(ws, ws.schedulers, num_threads); }

    static ProgramExecutionContext
    from_reset_workspace(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws, urd::Store<urd::sequence<Scheduler<LiftedTag>>>& schedulers, size_t num_threads = 1)
    {
        auto context = ProgramExecutionContext(In(*ws.const_workspace), Out(ws, schedulers), num_threads);
        context.initialize();
        return context;
    }

    void clear()
    {
        auto& out = this->out();

        out.engine().clear();
        out.cost_buckets().clear();
        out.pending_achievers().clear();
        out.facts().delta_fact_sets.reset();
        out.reset_numeric_support_selector();
        out.annotations().clear();
        out.numeric_annotations().clear();
        out.delta_annotations().clear();
        out.delta_numeric_annotations().clear();
        out.annotation_policy().clear_achievers();

        initialize();
    }

    [[nodiscard]] const auto& in() const noexcept { return m_in; }
    auto& out() noexcept { return m_out; }
    [[nodiscard]] const auto& out() const noexcept { return m_out; }
    [[nodiscard]] bool is_single_threaded() const noexcept { return m_num_threads == 1; }
    void set_num_threads(size_t num_threads) noexcept
    {
        assert(num_threads > 0);
        m_num_threads = num_threads;
    }

private:
    ProgramExecutionContext(In in, Out out, size_t num_threads) : m_in(in), m_out(out), m_num_threads(num_threads) { assert(num_threads > 0); }

    void initialize()
    {
        auto& out = this->out();
        out.tp().reset();
        // The engine's index is rebuilt from the facts the evaluation starts with.
        out.engine().assignment_sets.reset();
        for (const auto& set : out.facts().fact_sets.predicate.get_sets())
        {
            for (const auto binding : set.get_bindings())
            {
                out.annotation_policy().initialize_annotation(binding.data(), out.annotations());
                out.engine().assignment_sets.predicate.insert(binding);
            }
        }

        for (const auto& set : out.facts().fact_sets.function.get_sets())
        {
            const auto bindings = set.get_bindings();
            const auto& values = set.get_values();
            assert(bindings.size() == values.size());
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                out.annotation_policy().initialize_annotation(bindings[i].data(), values[i], out.numeric_annotations());
                out.engine().assignment_sets.function.insert(bindings[i], values[i]);
            }
        }

        out.rebuild_numeric_support_selector(in().facts().fact_sets);
    }

    In m_in;
    Out m_out;
    size_t m_num_threads;
};

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>&) -> ProgramExecutionContext<LiftedTag, AP, TP, CP>;

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>&, size_t) -> ProgramExecutionContext<LiftedTag, AP, TP, CP>;

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>&, urd::Store<urd::sequence<Scheduler<LiftedTag>>>&, size_t = 1)
    -> ProgramExecutionContext<LiftedTag, AP, TP, CP>;
}

#endif
