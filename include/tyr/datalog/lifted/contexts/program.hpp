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

#include "tyr/datalog/contexts/program.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/contexts/stratum.hpp"
#include "tyr/datalog/lifted/policies/cost.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"

#include <cassert>
#include <ranges>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::datalog
{

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
struct ProgramExecutionContext<LiftedTag, AP, TP, CP>
{
    class In
    {
    public:
        explicit In(const ConstProgramWorkspace<LiftedTag>& cws) : m_cws(cws) {}

        const auto& facts() const noexcept { return m_cws.facts; }
        template<::tyr::formalism::RelationKind R>
        const auto& get_rules() const noexcept
        {
            return m_cws.template get_rules<R>();
        }

    private:
        const ConstProgramWorkspace<LiftedTag>& m_cws;
    };

    class Out
    {
    public:
        explicit Out(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws) : m_ws(ws) {}

        auto& facts() noexcept { return m_ws.facts; }
        const auto& facts() const noexcept { return m_ws.facts; }
        auto& annotation_policy() noexcept { return m_ws.annotation_policy; }
        const auto& annotation_policy() const noexcept { return m_ws.annotation_policy; }
        auto& annotations() noexcept { return m_ws.annotations; }
        const auto& annotations() const noexcept { return m_ws.annotations; }
        auto& numeric_annotations() noexcept { return m_ws.numeric_annotations; }
        const auto& numeric_annotations() const noexcept { return m_ws.numeric_annotations; }
        auto& delta_annotations() noexcept { return m_ws.delta_annotations; }
        const auto& delta_annotations() const noexcept { return m_ws.delta_annotations; }
        auto& delta_numeric_annotations() noexcept { return m_ws.delta_numeric_annotations; }
        const auto& delta_numeric_annotations() const noexcept { return m_ws.delta_numeric_annotations; }
        const auto& numeric_support_selector() const noexcept { return m_ws.get_numeric_support_selector(); }
        void rebuild_numeric_support_selector(const TaggedFactSets<::tyr::formalism::StaticTag>& static_fact_sets)
        {
            m_ws.numeric_support_selector.emplace(FactSets { static_fact_sets, m_ws.facts.fact_sets }, m_ws.numeric_annotations);
        }
        void reset_numeric_support_selector() noexcept { m_ws.numeric_support_selector.reset(); }
        auto& tp() noexcept { return m_ws.tp; }
        const auto& tp() const noexcept { return m_ws.tp; }
        auto& cost_policy() noexcept { return m_ws.cost_policy; }
        const auto& cost_policy() const noexcept { return m_ws.cost_policy; }
        template<::tyr::formalism::RelationKind R>
        auto& get_rules() noexcept
        {
            return m_ws.template get_rules<R>();
        }
        template<::tyr::formalism::RelationKind R>
        const auto& get_rules() const noexcept
        {
            return m_ws.template get_rules<R>();
        }
        auto& datalog_builder() noexcept { return m_ws.datalog_builder; }
        const auto& datalog_builder() const noexcept { return m_ws.datalog_builder; }
        auto& workspace_repository() noexcept { return m_ws.workspace_repository; }
        const auto& workspace_repository() const noexcept { return m_ws.workspace_repository; }
        auto& schedulers() noexcept { return m_ws.schedulers; }
        const auto& schedulers() const noexcept { return m_ws.schedulers; }
        auto& cost_buckets() noexcept { return m_ws.cost_buckets; }
        const auto& cost_buckets() const noexcept { return m_ws.cost_buckets; }
        auto& statistics() noexcept { return m_ws.statistics; }
        const auto& statistics() const noexcept { return m_ws.statistics; }

    private:
        ProgramWorkspace<LiftedTag, AP, TP, CP>& m_ws;
    };

    explicit ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>& ws) : m_in(ws.const_workspace), m_out(ws) { clear(); }

    void clear() noexcept
    {
        auto& out = this->out();

        const auto clear_rules = [](auto& rules)
        {
            for (auto& rule : rules)
                if (rule)
                    rule->clear();
        };
        clear_rules(out.template get_rules<::tyr::formalism::PredicateTag>());
        clear_rules(out.template get_rules<::tyr::formalism::FunctionTag>());

        out.tp().reset();
        out.reset_numeric_support_selector();
        out.annotations().clear();
        out.numeric_annotations().clear();
        out.delta_annotations().clear();
        out.delta_numeric_annotations().clear();
        out.annotation_policy().clear_achievers();

        for (const auto& set : out.facts().fact_sets.predicate.get_sets())
        {
            for (const auto binding : set.get_bindings())
            {
                out.annotation_policy().initialize_annotation(binding, out.annotations());
                out.facts().assignment_sets.predicate.insert(binding);
            }
        }

        for (const auto& set : out.facts().fact_sets.function.get_sets())
        {
            for (const auto [binding, interval] : set.get_binding_values())
            {
                out.annotation_policy().initialize_annotation(binding, interval, out.numeric_annotations());
                out.facts().assignment_sets.function.insert(binding, interval);
            }
        }

        out.cost_buckets().clear();
        out.rebuild_numeric_support_selector(in().facts().fact_sets);
    }

    auto get_stratum_execution_contexts()
    {
        return out().schedulers().data
               | std::views::transform([this](RuleSchedulerStratum& scheduler) { return StratumExecutionContext<AP, TP, CP> { scheduler, *this }; });
    }

    const auto& in() const noexcept { return m_in; }
    auto& out() noexcept { return m_out; }
    const auto& out() const noexcept { return m_out; }

private:
    In m_in;
    Out m_out;
};

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
ProgramExecutionContext(ProgramWorkspace<LiftedTag, AP, TP, CP>&) -> ProgramExecutionContext<LiftedTag, AP, TP, CP>;

}

#endif
