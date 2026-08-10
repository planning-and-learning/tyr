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

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
struct ProgramExecutionContext<GroundTag, AP, TP, CP>
{
    class In
    {
    public:
        explicit In(const ConstProgramWorkspace<GroundTag>& cws) : m_cws(cws) {}

        auto program() const noexcept { return m_cws.program; }
        const auto& facts() const noexcept { return m_cws.facts; }
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
        explicit Out(ProgramWorkspace<GroundTag, AP, TP, CP>& ws) : m_ws(ws) {}

        auto& facts() noexcept { return m_ws.facts; }
        const auto& facts() const noexcept { return m_ws.facts; }
        auto& annotation_policy() noexcept { return m_ws.annotation_policy; }
        const auto& annotation_policy() const noexcept { return m_ws.annotation_policy; }
        auto& annotations() noexcept { return m_ws.annotations; }
        const auto& annotations() const noexcept { return m_ws.annotations; }
        auto& numeric_annotations() noexcept { return m_ws.numeric_annotations; }
        const auto& numeric_annotations() const noexcept { return m_ws.numeric_annotations; }
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
            return m_ws.queue.template get_storage<R>();
        }
        template<::tyr::formalism::RelationKind R>
        const auto& queue_storage() const noexcept
        {
            return m_ws.queue.template get_storage<R>();
        }
        auto& fact_sets() noexcept { return m_ws.facts.fact_sets; }
        const auto& fact_sets() const noexcept { return m_ws.facts.fact_sets; }
        auto& statistics() noexcept { return m_ws.queue.statistics; }
        const auto& statistics() const noexcept { return m_ws.queue.statistics; }
        auto& queue() noexcept { return m_ws.queue; }
        const auto& queue() const noexcept { return m_ws.queue; }

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

        ProgramWorkspace<GroundTag, AP, TP, CP>& m_ws;
    };

    explicit ProgramExecutionContext(ProgramWorkspace<GroundTag, AP, TP, CP>& ws) : m_in(ws.const_workspace), m_out(ws) { clear(); }

    void clear() { reset_from_current_facts(); }

    void initialize() { reset_from_current_facts(); }

    template<typename Range>
    void initialize(const Range& fluent_atoms)
    {
        m_out.facts().reset();
        for (const auto atom : fluent_atoms)
            m_out.fact_sets().predicate.insert(atom);
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
        return m_out.fact_sets().predicate.contains(fact.get_row());
    }

    void initialize_annotations()
    {
        m_out.annotations().clear();
        m_out.numeric_annotations().clear();
        m_out.annotation_policy().clear_achievers();
        m_out.tp().reset();

        for (const auto& set : m_out.fact_sets().predicate.get_sets())
            for (const auto binding : set.get_bindings())
                m_out.annotation_policy().initialize_annotation(binding, m_out.annotations());

        for (const auto& set : m_out.fact_sets().function.get_sets())
            for (const auto [binding, interval] : set.get_binding_values())
                m_out.annotation_policy().initialize_annotation(binding, interval, m_out.numeric_annotations());
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
        for (const auto rule : m_in.program().template get_rules<R>())
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

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
ProgramExecutionContext(ProgramWorkspace<GroundTag, AP, TP, CP>&) -> ProgramExecutionContext<GroundTag, AP, TP, CP>;

}

#endif
