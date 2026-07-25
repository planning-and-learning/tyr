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

#include "tyr/datalog/contexts/program.hpp"
#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/datalog/ground/workspaces/queue.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<OrAnnotationPolicyConcept<GroundTag> OrAP,
         AndAnnotationPolicyConcept<GroundTag> AndAP,
         TerminationPolicyConcept<GroundTag> TP,
         RuleCostPolicyConcept<GroundTag> CP>
struct ProgramExecutionContext<GroundTag, OrAP, AndAP, TP, CP>
{
    class In
    {
    public:
        explicit In(const ConstProgramWorkspace<GroundTag>& cws) : m_cws(cws) {}

        auto program() const noexcept { return m_cws.program; }
        template<::tyr::formalism::RelationKind R>
        const auto& dependencies() const noexcept
        {
            return m_cws.template get_dependencies<R>();
        }

    private:
        const ConstProgramWorkspace<GroundTag>& m_cws;
    };

    class Out
    {
    public:
        explicit Out(ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP>& ws, QueueWorkspace<GroundTag>& queue_ws) : m_ws(ws), m_queue_ws(queue_ws) {}

        auto& facts() noexcept { return m_ws.facts; }
        const auto& facts() const noexcept { return m_ws.facts; }
        auto& or_ap() noexcept { return m_ws.or_ap; }
        const auto& or_ap() const noexcept { return m_ws.or_ap; }
        auto& and_ap() noexcept { return m_ws.and_ap; }
        const auto& and_ap() const noexcept { return m_ws.and_ap; }
        auto& and_annot() noexcept { return m_ws.and_annot; }
        const auto& and_annot() const noexcept { return m_ws.and_annot; }
        auto& numeric_and_annot() noexcept { return m_ws.numeric_and_annot; }
        const auto& numeric_and_annot() const noexcept { return m_ws.numeric_and_annot; }
        auto& tp() noexcept { return m_ws.tp; }
        const auto& tp() const noexcept { return m_ws.tp; }
        auto& cost_policy() noexcept { return m_ws.cost_policy; }
        const auto& cost_policy() const noexcept { return m_ws.cost_policy; }
        template<::tyr::formalism::RelationKind R>
        auto& rule_states() noexcept
        {
            return rules<R>().states;
        }
        template<::tyr::formalism::RelationKind R>
        const auto& rule_states() const noexcept
        {
            return rules<R>().states;
        }
        template<::tyr::formalism::RelationKind R>
        auto& queue_storage() noexcept
        {
            return m_queue_ws.template get_storage<R>();
        }
        template<::tyr::formalism::RelationKind R>
        const auto& queue_storage() const noexcept
        {
            return m_queue_ws.template get_storage<R>();
        }
        auto& fluent_fact_sets() noexcept { return m_ws.facts.fluent_fact_sets; }
        const auto& fluent_fact_sets() const noexcept { return m_ws.facts.fluent_fact_sets; }
        auto& fluent_atoms() noexcept { return m_ws.facts.fluent_atoms; }
        const auto& fluent_atoms() const noexcept { return m_ws.facts.fluent_atoms; }
        auto& static_fterm_intervals() noexcept { return m_ws.facts.static_fterm_intervals; }
        const auto& static_fterm_intervals() const noexcept { return m_ws.facts.static_fterm_intervals; }
        auto& fluent_fterm_intervals() noexcept { return m_ws.facts.fluent_fterm_intervals; }
        const auto& fluent_fterm_intervals() const noexcept { return m_ws.facts.fluent_fterm_intervals; }
        auto& statistics() noexcept { return m_queue_ws.statistics; }
        const auto& statistics() const noexcept { return m_queue_ws.statistics; }
        auto& queue() noexcept { return m_queue_ws; }
        const auto& queue() const noexcept { return m_queue_ws; }

    private:
        template<::tyr::formalism::RelationKind R>
        auto& rules() noexcept
        {
            if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
                return m_ws.predicate_rules;
            else
                return m_ws.function_rules;
        }
        template<::tyr::formalism::RelationKind R>
        const auto& rules() const noexcept
        {
            if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
                return m_ws.predicate_rules;
            else
                return m_ws.function_rules;
        }

        ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP>& m_ws;
        QueueWorkspace<GroundTag>& m_queue_ws;
    };

    ProgramExecutionContext(ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP>& ws, QueueWorkspace<GroundTag>& queue_ws) :
        m_in(ws.const_workspace),
        m_out(ws, queue_ws)
    {
    }

    void clear() { reset_from_current_facts(); }

    void initialize() { reset_from_current_facts(); }

    template<typename Range>
    void initialize(const Range& fluent_atoms)
    {
        m_out.fluent_atoms().clear();
        for (const auto atom : fluent_atoms)
            m_out.fluent_atoms().insert(atom);
        initialize();
    }

    const auto& in() const noexcept { return m_in; }
    auto& out() noexcept { return m_out; }
    const auto& out() const noexcept { return m_out; }

private:
    template<typename Index>
    static size_t position(Index index) noexcept
    {
        return static_cast<size_t>(index.get_value());
    }

    template<typename T, typename Index>
    static decltype(auto) at(std::vector<T>& vector, Index index) noexcept
    {
        return vector[position(index)];
    }

    template<typename T, typename Index>
    static decltype(auto) at(const std::vector<T>& vector, Index index) noexcept
    {
        return vector[position(index)];
    }

    bool is_fluent_fact_true(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> fact) const noexcept
    {
        return m_out.fluent_atoms().contains(fact);
    }

    void initialize_annotations()
    {
        m_out.and_annot().clear();
        m_out.numeric_and_annot().clear();
        m_out.and_ap().clear_achievers();
        m_out.tp().reset();

        m_out.static_fterm_intervals().clear();
        for (const auto fterm_value : m_in.program().template get_fterm_values<::tyr::formalism::StaticTag>())
            m_out.static_fterm_intervals().insert_or_assign(fterm_value.get_fterm(),
                                                            ygg::ClosedInterval<ygg::float_t>(fterm_value.get_value(), fterm_value.get_value()));

        m_out.fluent_fact_sets().reset();

        for (const auto fact : m_out.fluent_atoms())
        {
            m_out.fluent_fact_sets().predicate.insert(fact);
            m_out.or_ap().initialize_annotation(fact, m_out.and_annot());
        }

        for (const auto& [fterm, interval] : m_out.fluent_fterm_intervals())
        {
            m_out.fluent_fact_sets().function.insert(fterm, interval);
            m_out.or_ap().initialize_annotation(fterm, interval, m_out.numeric_and_annot());
        }
    }

    void reset_from_current_facts()
    {
        initialize_annotations();
        m_out.template rule_states<::tyr::formalism::PredicateTag>().clear();
        m_out.template rule_states<::tyr::formalism::FunctionTag>().clear();
        m_out.queue().clear();
        initialize_rule_states<::tyr::formalism::PredicateTag>();
        initialize_rule_states<::tyr::formalism::FunctionTag>();
    }

    template<::tyr::formalism::RelationKind R>
    void initialize_rule_states()
    {
        auto& states = m_out.template rule_states<R>();
        for (const auto rule : m_in.program().template get_ground_rules<R>())
        {
            const auto rule_index = rule.get_index();
            if (position(rule_index) >= states.size())
                states.resize(position(rule_index) + 1);

            auto& state = at(states, rule_index);

            auto unsatisfied_count = ygg::uint_t(0);
            for (const auto literal : rule.get_body().template get_literals<::tyr::formalism::FluentTag>())
            {
                if (!literal.get_polarity())
                    ++unsatisfied_count;
                else if (!is_fluent_fact_true(literal.get_atom()))
                    ++unsatisfied_count;
            }
            const auto numeric_constraints = rule.get_body().get_numeric_constraints();
            state.numeric_constraint_satisfied.assign(numeric_constraints.size(), false);
            for (ygg::uint_t i = 0; i < numeric_constraints.size(); ++i)
                ++unsatisfied_count;

            state.unsatisfied_count = unsatisfied_count;
            state.fired = false;
            state.queued_cost = std::nullopt;
        }
    }

    In m_in;
    Out m_out;
};

template<OrAnnotationPolicyConcept<GroundTag> OrAP,
         AndAnnotationPolicyConcept<GroundTag> AndAP,
         TerminationPolicyConcept<GroundTag> TP,
         RuleCostPolicyConcept<GroundTag> CP>
ProgramExecutionContext(ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP>&,
                        QueueWorkspace<GroundTag>&) -> ProgramExecutionContext<GroundTag, OrAP, AndAP, TP, CP>;

}

#endif
