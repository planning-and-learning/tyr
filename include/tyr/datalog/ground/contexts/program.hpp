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
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/variant.hpp>
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
        auto numeric_support_selector() const noexcept { return m_ws.get_numeric_support_selector(); }
        auto& tp() noexcept { return m_ws.tp; }
        const auto& tp() const noexcept { return m_ws.tp; }
        auto& cost_policy() noexcept { return m_ws.cost_policy; }
        const auto& cost_policy() const noexcept { return m_ws.cost_policy; }
        auto& scheduler() noexcept { return m_ws.scheduler; }
        const auto& scheduler() const noexcept { return m_ws.scheduler; }

    private:
        ProgramWorkspace<GroundTag, AP, TP, CP>& m_ws;
    };

    explicit ProgramExecutionContext(ProgramWorkspace<GroundTag, AP, TP, CP>& ws) : m_in(ws.const_workspace), m_out(ws) { clear(); }

    void clear() { reset_from_current_facts(); }

    void initialize() { reset_from_current_facts(); }

    template<typename Range>
    void initialize(const Range& fluent_atoms)
    {
        m_out.facts().reset();
        m_out.facts().fact_sets.function.insert(m_in.program().template get_fterm_values<::tyr::formalism::FluentTag>());
        for (const auto atom : fluent_atoms)
            m_out.facts().fact_sets.predicate.insert(atom);
        initialize();
    }

    const auto& in() const noexcept { return m_in; }
    auto& out() noexcept { return m_out; }
    const auto& out() const noexcept { return m_out; }

private:
    void initialize_annotations()
    {
        m_out.annotations().clear();
        m_out.numeric_annotations().clear();
        m_out.annotation_policy().clear_achievers();
        m_out.tp().reset();

        for (const auto& set : m_out.facts().fact_sets.predicate.get_sets())
            for (const auto binding : set.get_bindings())
                m_out.annotation_policy().initialize_annotation(binding, m_out.annotations());

        for (const auto& set : m_out.facts().fact_sets.function.get_sets())
        {
            const auto bindings = set.get_bindings();
            const auto& values = set.get_values();
            assert(bindings.size() == values.size());
            for (size_t i = 0; i < bindings.size(); ++i)
                m_out.annotation_policy().initialize_annotation(bindings[i], values[i], m_out.numeric_annotations());
        }
    }

    void reset_from_current_facts()
    {
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
