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

#ifndef TYR_DATALOG_LIFTED_CONTEXTS_RULE_HPP_
#define TYR_DATALOG_LIFTED_CONTEXTS_RULE_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"

#include <cstddef>

namespace tyr::datalog
{
template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct StratumExecutionContext;

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct RuleExecutionContext;

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
class RuleWorkerExecutionContext
{
public:
    class In
    {
    public:
        explicit In(RuleExecutionContext<R, AP, TP, CP>& rctx) :
            m_rctx(rctx),
            m_cws_rule(rctx.in().cws_rule()),
            m_fact_sets(rctx.stratum_in().program().facts().fact_sets, rctx.stratum_out().program().facts().fact_sets)
        {
        }

        const auto& cws_rule() const noexcept { return m_cws_rule; }

        auto& annotation_policy() noexcept { return m_rctx.stratum_out().program().annotation_policy(); }
        const auto& annotations() const noexcept { return m_rctx.stratum_out().program().annotations(); }
        const auto& numeric_annotations() const noexcept { return m_rctx.stratum_out().program().numeric_annotations(); }
        const auto& numeric_support_selector() const noexcept { return m_rctx.stratum_out().program().numeric_support_selector(); }
        const auto& cost_policy() const noexcept { return m_rctx.stratum_out().program().cost_policy(); }
        const auto& fact_sets() const noexcept { return m_fact_sets; }

    private:
        RuleExecutionContext<R, AP, TP, CP>& m_rctx;

        const ConstRuleWorkspace<LiftedTag, R>& m_cws_rule;

        const FactSets m_fact_sets;
    };

    class Out
    {
    public:
        Out(RuleExecutionContext<R, AP, TP, CP>& rctx, typename RuleWorkspace<LiftedTag, R>::Worker& ws_worker) :
            m_rctx(rctx),
            m_ws_worker(ws_worker),
            m_ground_context(ws_worker.builder, rctx.stratum_out().program().workspace_repository(), ws_worker.binding)
        {
        }

        auto& kckp_workspace() noexcept { return m_ws_worker.iteration.kckp_workspace; }
        auto& delta_annotations() noexcept { return m_rctx.stratum_out().program().delta_annotations(); }
        auto& delta_numeric_annotations() noexcept { return m_rctx.stratum_out().program().delta_numeric_annotations(); }
        auto& head_updates() noexcept { return m_ws_worker.iteration.head_updates; }

#ifndef NDEBUG
        auto& seen_bindings() noexcept { return m_ws_worker.solve.seen_bindings; }
#endif
#ifndef TYR_ENABLE_SEMI_NAIVE
        auto& seen_pending_rule_bindings() noexcept { return m_ws_worker.solve.seen_pending_rule_bindings; }
#endif
        auto& pending_rule_bindings() noexcept { return m_ws_worker.solve.pending_rule_bindings; }
        auto& rule_evaluation_workspace() noexcept { return m_ws_worker.solve.rule_evaluation_workspace; }
        auto& applicability_cache() noexcept { return m_ws_worker.solve.applicability_cache; }
        auto& statistics() noexcept { return m_ws_worker.solve.statistics; }

        auto& ground_context() noexcept { return m_ground_context; }

    private:
        RuleExecutionContext<R, AP, TP, CP>& m_rctx;
        typename RuleWorkspace<LiftedTag, R>::Worker& m_ws_worker;

        ::tyr::formalism::datalog::GrounderContext m_ground_context;
    };

    RuleWorkerExecutionContext(RuleExecutionContext<R, AP, TP, CP>& rctx, typename RuleWorkspace<LiftedTag, R>::Worker& ws_worker) :
        m_in(rctx),
        m_out(rctx, ws_worker)
    {
    }

    auto& in() noexcept { return m_in; }
    const auto& in() const noexcept { return m_in; }

    auto& out() noexcept { return m_out; }
    const auto& out() const noexcept { return m_out; }

private:
    In m_in;
    Out m_out;
};

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct RuleExecutionContext
{
    class In
    {
    public:
        explicit In(const ConstRuleWorkspace<LiftedTag, R>& cws_rule) : m_cws_rule(cws_rule) {}

        const auto& cws_rule() const noexcept { return m_cws_rule; }

    private:
        const ConstRuleWorkspace<LiftedTag, R>& m_cws_rule;
    };

    class Out
    {
    public:
        explicit Out(RuleWorkspace<LiftedTag, R>& ws_rule) : m_ws_rule(ws_rule) {}

        auto& common() noexcept { return m_ws_rule.common; }
        const auto& common() const noexcept { return m_ws_rule.common; }
        auto& kckp() noexcept { return m_ws_rule.common.kckp; }
        const auto& kckp() const noexcept { return m_ws_rule.common.kckp; }
        auto& statistics() noexcept { return m_ws_rule.common.statistics; }
        const auto& statistics() const noexcept { return m_ws_rule.common.statistics; }
        auto& workers() noexcept { return m_ws_rule.worker; }
        const auto& workers() const noexcept { return m_ws_rule.worker; }

    private:
        RuleWorkspace<LiftedTag, R>& m_ws_rule;
    };

    RuleExecutionContext(ygg::Index<::tyr::formalism::datalog::Rule<R>> rule, StratumExecutionContext<AP, TP, CP>& ctx) :
        m_ctx(ctx),
        m_in(*ctx.in().program().template get_rules<R>()[ygg::uint_t(rule)]),
        m_out(*ctx.out().program().template get_rules<R>()[ygg::uint_t(rule)])
    {
    }

    void initialize()
    {
        out().common().initialize_iteration(AssignmentSets { stratum_in().program().facts().assignment_sets, stratum_out().program().facts().assignment_sets });
    }

    void clear_iteration() noexcept
    {
        for (auto& worker : out().workers())
            worker.iteration.clear();
    }

    auto get_rule_worker_execution_context(std::size_t worker_index = 0)
    {
        return RuleWorkerExecutionContext<R, AP, TP, CP>(*this, out().workers()[worker_index]);
    }

    const auto& in() const noexcept { return m_in; }
    auto& out() noexcept { return m_out; }
    const auto& out() const noexcept { return m_out; }

    const auto& stratum_in() const noexcept { return m_ctx.in(); }
    auto& stratum_out() noexcept { return m_ctx.out(); }
    const auto& stratum_out() const noexcept { return m_ctx.out(); }

private:
    StratumExecutionContext<AP, TP, CP>& m_ctx;

    In m_in;
    Out m_out;
};
}

#endif
