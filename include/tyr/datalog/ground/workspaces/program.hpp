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

#ifndef TYR_DATALOG_GROUND_WORKSPACES_PROGRAM_HPP_
#define TYR_DATALOG_GROUND_WORKSPACES_PROGRAM_HPP_

#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/workspaces/facts.hpp"
#include "tyr/datalog/ground/workspaces/rule.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/datalog/workspaces/program.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
struct GroundRuleDependencies
{
    ygg::UnorderedMap<::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag>, std::vector<::tyr::formalism::datalog::GroundRuleView<R>>>
        fluent_precondition_to_rules;
    ygg::UnorderedMap<::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>, std::vector<::tyr::formalism::datalog::GroundRuleView<R>>>
        fluent_function_term_to_rules;
};

template<>
struct ConstProgramWorkspace<GroundTag>
{
    ::tyr::formalism::datalog::ProgramView<GroundTag> program;
    ConstFactsWorkspace<GroundTag> facts;
    GroundRuleDependencies<::tyr::formalism::PredicateTag> predicate_rules;
    GroundRuleDependencies<::tyr::formalism::FunctionTag> function_rules;

    template<::tyr::formalism::RelationKind R>
    const auto& get_dependencies() const noexcept;

    explicit ConstProgramWorkspace(::tyr::formalism::datalog::ProgramView<GroundTag> program);
};

template<OrAnnotationPolicyConcept<GroundTag> OrAP,
         AndAnnotationPolicyConcept<GroundTag> AndAP,
         TerminationPolicyConcept<GroundTag> TP,
         RuleCostPolicyConcept<GroundTag> CP>
struct ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP>
{
    const ConstProgramWorkspace<GroundTag>& const_workspace;
    FactsWorkspace<GroundTag> facts;
    OrAP or_ap;
    AndAP and_ap;
    SelectedPredicateAnnotations<GroundTag> and_annot;
    SelectedFunctionAnnotations<GroundTag> numeric_and_annot;
    TP tp;
    CP cost_policy;
    RuleWorkspace<GroundTag, ::tyr::formalism::PredicateTag> predicate_rules;
    RuleWorkspace<GroundTag, ::tyr::formalism::FunctionTag> function_rules;

    explicit ProgramWorkspace(const ConstProgramWorkspace<GroundTag>& cws,
                              OrAP or_ap_ = OrAP(),
                              AndAP and_ap_ = AndAP(),
                              TP tp_ = TP(),
                              CP cost_policy_ = CP()) :
        const_workspace(cws),
        facts(cws.program.template get_predicates<::tyr::formalism::FluentTag>(),
              cws.program.template get_functions<::tyr::formalism::FluentTag>(),
              cws.program.template get_atoms<::tyr::formalism::FluentTag>(),
              cws.program.template get_fterm_values<::tyr::formalism::FluentTag>(),
              cws.program.get_context()),
        or_ap(std::move(or_ap_)),
        and_ap(std::move(and_ap_)),
        and_annot(),
        numeric_and_annot(),
        tp(std::move(tp_)),
        cost_policy(std::move(cost_policy_)),
        predicate_rules(cws.program),
        function_rules(cws.program)
    {
    }

    explicit ProgramWorkspace(Program<GroundTag>& program, OrAP or_ap_ = OrAP(), AndAP and_ap_ = AndAP(), TP tp_ = TP(), CP cost_policy_ = CP());

    void clear_costs() { cost_policy.clear(); }
};

template<>
inline const auto& ConstProgramWorkspace<GroundTag>::get_dependencies<::tyr::formalism::PredicateTag>() const noexcept
{
    return predicate_rules;
}

template<>
inline const auto& ConstProgramWorkspace<GroundTag>::get_dependencies<::tyr::formalism::FunctionTag>() const noexcept
{
    return function_rules;
}

}

#endif
