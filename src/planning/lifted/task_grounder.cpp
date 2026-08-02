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

#include "tyr/planning/lifted/task_grounder.hpp"

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/lifted/bottom_up.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/invariants/mutexes.hpp"
#include "tyr/formalism/planning/invariants/synthesis.hpp"
#include "tyr/formalism/planning/merge.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <utility>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;
namespace fpi = tyr::formalism::planning::invariant;

namespace tyr::planning
{
namespace
{

enum class RemapStatus
{
    mapped,
    tautology,
    contradiction,
};

template<f::FactKind T>
RemapStatus classify_literal(fp::GroundAtomView<T> atom, bool polarity, const ygg::UnorderedSet<fp::GroundAtomView<T>>& atoms)
{
    const bool present = atoms.contains(atom);

    if (polarity)
        return present ? RemapStatus::mapped : RemapStatus::contradiction;

    return present ? RemapStatus::mapped : RemapStatus::tautology;
}

template<f::FactKind T>
RemapStatus classify_literal(fp::GroundLiteralView<T> literal, const ygg::UnorderedSet<fp::GroundAtomView<T>>& atoms)
{
    return classify_literal(literal.get_atom(), literal.get_polarity(), atoms);
}

struct RemappedFDRFact
{
    RemapStatus status;
    std::optional<fp::FDRFactView<f::FluentTag>> fact;
};

RemappedFDRFact remap_fdr_fact(fp::FDRFactView<f::FluentTag> fact,
                               bool polarity,
                               const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                               const fp::FDRContext& fdr_context,
                               fp::MergeContext& context)
{
    assert(fact.has_value());

    const auto new_atom = merge_p2p(fact.get_atom().value(), context).first;
    const auto status = classify_literal(new_atom, polarity, fluent_atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    const auto new_fact = fdr_context.get_fact(new_atom);
    if (!new_fact)
        return { RemapStatus::contradiction, std::nullopt };

    return { RemapStatus::mapped, *new_fact };
}

template<f::FactKind T>
struct RemappedLiteral
{
    RemapStatus status;
    std::optional<fp::GroundLiteralView<T>> literal;
};

template<f::FactKind T>
RemappedLiteral<T> remap_literal(fp::GroundLiteralView<T> literal, fp::MergeContext& context, const ygg::UnorderedSet<fp::GroundAtomView<T>>& atoms)
{
    const auto new_literal = merge_p2p(literal, context).first;
    const auto status = classify_literal(new_literal, atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    return { RemapStatus::mapped, new_literal };
}

template<f::FactKind T>
RemappedLiteral<T> remap_literal(fp::GroundLiteralView<T> literal, const ygg::UnorderedSet<fp::GroundAtomView<T>>& atoms)
{
    const auto status = classify_literal(literal, atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    return { RemapStatus::mapped, literal };
}

std::optional<fp::GroundConjunctiveConditionView>
create_ground_fdr_conjunctive_condition(fp::GroundConjunctiveConditionView element,
                                        const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                                        const ygg::UnorderedSet<fp::GroundAtomView<f::DerivedTag>>& derived_atoms,
                                        const boost::dynamic_bitset<>& static_atoms_bitset,
                                        const fp::FDRContext& fdr_context,
                                        fp::MergeContext& context)
{
    auto fdr_conj_cond_ptr = context.builder.get_builder<fp::GroundConjunctiveCondition>();
    auto& fdr_conj_cond = *fdr_conj_cond_ptr;
    fdr_conj_cond.clear();

    for (const auto literal : element.get_literals<f::StaticTag>())
    {
        const auto new_literal = merge_p2p(literal, context).first;
        if (!is_statically_applicable(new_literal, static_atoms_bitset))
            return std::nullopt;
    }

    for (const auto literal : element.get_literals<f::DerivedTag>())
    {
        const auto remapped = remap_literal(literal, context, derived_atoms);

        if (remapped.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped.status == RemapStatus::tautology)
            continue;

        fdr_conj_cond.derived_literals.push_back(remapped.literal->get_index());
    }

    for (const auto fact : element.get_facts<f::PositiveTag>())
    {
        const auto remapped_fdr_fact = remap_fdr_fact(fact, true, fluent_atoms, fdr_context, context);

        if (remapped_fdr_fact.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped_fdr_fact.status == RemapStatus::tautology)
            continue;

        fdr_conj_cond.positive_facts.push_back(remapped_fdr_fact.fact->get_data());
    }

    for (const auto fact : element.get_facts<f::NegativeTag>())
    {
        const auto remapped_fdr_fact = remap_fdr_fact(fact, false, fluent_atoms, fdr_context, context);

        if (remapped_fdr_fact.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped_fdr_fact.status == RemapStatus::tautology)
            continue;

        fdr_conj_cond.negative_facts.push_back(remapped_fdr_fact.fact->get_data());
    }

    for (const auto numeric_constraint : element.get_numeric_constraints())
        fdr_conj_cond.numeric_constraints.push_back(merge_p2p(numeric_constraint, context).get_data());

    canonicalize(fdr_conj_cond);
    return context.destination.get_or_create(fdr_conj_cond).first;
}

std::optional<fp::GroundConjunctiveConditionView> ground_pruned(fp::ConjunctiveConditionView element,
                                                                const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                                                                const ygg::UnorderedSet<fp::GroundAtomView<f::DerivedTag>>& derived_atoms,
                                                                const boost::dynamic_bitset<>& static_atoms_bitset,
                                                                fp::GrounderContext& context,
                                                                const fp::FDRContext& fdr_context)
{
    auto conj_cond_ptr = context.builder.template get_builder<fp::GroundConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto literal : element.template get_literals<f::StaticTag>())
    {
        const auto new_literal = ground(literal, context).first;
        if (!is_statically_applicable(new_literal, static_atoms_bitset))
            return std::nullopt;
    }

    for (const auto literal : element.template get_literals<f::FluentTag>())
    {
        const auto new_literal = ground(literal, context).first;
        const auto remapped = remap_literal(new_literal, fluent_atoms);

        if (remapped.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped.status == RemapStatus::tautology)
            continue;

        const auto new_fact = fdr_context.get_fact(remapped.literal->get_atom());
        if (!new_fact)
            return std::nullopt;

        if (literal.get_polarity())
            conj_cond.positive_facts.push_back(new_fact->get_data());
        else
            conj_cond.negative_facts.push_back(new_fact->get_data());
    }

    for (const auto literal : element.template get_literals<f::DerivedTag>())
    {
        const auto new_literal = ground(literal, context).first;
        const auto remapped = remap_literal(new_literal, derived_atoms);

        if (remapped.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped.status == RemapStatus::tautology)
            continue;

        conj_cond.derived_literals.push_back(remapped.literal->get_index());
    }

    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(ground(numeric_constraint, context).get_data());

    canonicalize(conj_cond);
    return context.destination.get_or_create(conj_cond).first;
}

std::optional<fp::GroundConjunctiveEffectView> ground_pruned(fp::ConjunctiveEffectView element,
                                                             const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                                                             fp::GrounderContext& context,
                                                             const fp::FDRContext& fdr)
{
    // Fetch and clear
    auto conj_effect_ptr = context.builder.template get_builder<fp::GroundConjunctiveEffect>();
    auto& conj_eff = *conj_effect_ptr;
    conj_eff.clear();

    for (const auto literal : element.get_literals())
    {
        const auto new_atom = ground(literal.get_atom(), context).first;
        if (!fluent_atoms.contains(new_atom))
            return std::nullopt;

        const auto new_fact = fdr.get_fact(new_atom);
        if (!new_fact)
            return std::nullopt;

        if (literal.get_polarity())
            conj_eff.add_facts.push_back(new_fact->get_data());
        else
            conj_eff.del_facts.push_back(new_fact->get_data());
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        conj_eff.numeric_effects.push_back(ground(numeric_effect, context).get_data());
    if (element.get_auxiliary_numeric_effect().has_value())
        conj_eff.auxiliary_numeric_effect = ground(element.get_auxiliary_numeric_effect().value(), context).get_data();

    // Prune no-op effects
    if (conj_eff.add_facts.empty() && conj_eff.del_facts.empty() && conj_eff.numeric_effects.empty() && !conj_eff.auxiliary_numeric_effect)
        return std::nullopt;  // no-op

    // Canonicalize and Serialize
    canonicalize(conj_eff);
    return context.destination.get_or_create(conj_eff).first;
}

std::optional<fp::GroundConditionalEffectView> ground_pruned(fp::ConditionalEffectView element,
                                                             const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                                                             const ygg::UnorderedSet<fp::GroundAtomView<f::DerivedTag>>& derived_atoms,
                                                             const boost::dynamic_bitset<>& static_atoms_bitset,
                                                             fp::GrounderContext& context,
                                                             const fp::FDRContext& fdr_context)
{
    // Fetch and clear
    auto cond_effect_ptr = context.builder.template get_builder<fp::GroundConditionalEffect>();
    auto& cond_effect = *cond_effect_ptr;
    cond_effect.clear();

    // Fill data
    const auto new_condition_or_nullopt = ground_pruned(element.get_condition(), fluent_atoms, derived_atoms, static_atoms_bitset, context, fdr_context);
    if (!new_condition_or_nullopt)
        return std::nullopt;

    cond_effect.condition = new_condition_or_nullopt->get_index();

    const auto new_effect_or_nullopt = ground_pruned(element.get_effect(), fluent_atoms, context, fdr_context);
    if (!new_effect_or_nullopt)
        return std::nullopt;

    cond_effect.effect = new_effect_or_nullopt->get_index();

    // Canonicalize and Serialize
    canonicalize(cond_effect);
    return context.destination.get_or_create(cond_effect).first;
}

std::optional<fp::GroundActionView> ground_pruned(fp::ActionView element,
                                                  const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                                                  const ygg::UnorderedSet<fp::GroundAtomView<f::DerivedTag>>& derived_atoms,
                                                  const analysis::ActionDomain& action_domains,
                                                  const boost::dynamic_bitset<>& static_atoms_bitset,
                                                  ygg::itertools::cartesian_set::Workspace<ygg::Index<f::Object>>& iter_workspace,
                                                  fp::GrounderContext& context,
                                                  const fp::FDRContext& fdr_context)
{
    // Fetch and clear
    auto action_ptr = context.builder.template get_builder<fp::GroundAction>();
    auto& action = *action_ptr;
    action.clear();

    // Fill data
    action.binding = ground(element, context).first.get_index();

    const auto new_condition_or_nullopt = ground_pruned(element.get_condition(), fluent_atoms, derived_atoms, static_atoms_bitset, context, fdr_context);
    if (!new_condition_or_nullopt)
        return std::nullopt;

    action.condition = new_condition_or_nullopt->get_index();

    auto binding_size = context.binding.size();

    for (ygg::uint_t cond_effect_index = 0; cond_effect_index < element.get_effects().size(); ++cond_effect_index)
    {
        const auto cond_effect = element.get_effects()[cond_effect_index];
        const auto& parameter_domains = action_domains.payload.effect_domains.at(cond_effect.get_index()).payload.effect_domain.payload;

        assert(std::distance(parameter_domains.begin(), parameter_domains.end()) == static_cast<int>(element.get_arity() + cond_effect.get_arity()));

        ygg::itertools::cartesian_set::for_each_element(
            parameter_domains.begin() + element.get_arity(),
            parameter_domains.end(),
            iter_workspace,
            [&](auto&& binding_cond)
            {
                // push the additional parameters to the end
                context.binding.resize(binding_size);
                context.binding.insert(context.binding.end(), binding_cond.begin(), binding_cond.end());

                const auto ground_cond_effect_or_nullopt = ground_pruned(cond_effect, fluent_atoms, derived_atoms, static_atoms_bitset, context, fdr_context);
                if (ground_cond_effect_or_nullopt.has_value())
                    action.effects.push_back(ground_cond_effect_or_nullopt->get_index());
            });
    }
    context.binding.resize(binding_size);  ///< important to restore the binding in case of grounding other actions

    if (action.effects.empty())
        return std::nullopt;

    // Canonicalize and Serialize
    canonicalize(action);
    return context.destination.get_or_create(action).first;
}

std::optional<fp::GroundAxiomView> ground_pruned(fp::AxiomView element,
                                                 const ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>& fluent_atoms,
                                                 const ygg::UnorderedSet<fp::GroundAtomView<f::DerivedTag>>& derived_atoms,
                                                 const boost::dynamic_bitset<>& static_atoms_bitset,
                                                 fp::GrounderContext& context,
                                                 const fp::FDRContext& fdr_context)
{
    // Fetch and clear
    auto axiom_ptr = context.builder.template get_builder<fp::GroundAxiom>();
    auto& axiom = *axiom_ptr;
    axiom.clear();

    // Fill data
    axiom.binding = ground(element, context).first.get_index();

    const auto new_body_or_nullopt = ground_pruned(element.get_body(), fluent_atoms, derived_atoms, static_atoms_bitset, context, fdr_context);
    if (!new_body_or_nullopt.has_value())
        return std::nullopt;  // body is false in all reachable states -> axiom is irrelevant

    axiom.body = new_body_or_nullopt->get_index();

    const auto new_head = ground(element.get_head(), context).first;
    if (!derived_atoms.contains(new_head))
        return std::nullopt;  // head is false in all reachable states -> axiom is irrelevant

    axiom.head = new_head.get_index();

    // Canonicalize and Serialize
    canonicalize(axiom);
    return context.destination.get_or_create(axiom).first;
}
}

GroundTaskInstantiationResult instantiate_ground_task(Task<LiftedTag>& lifted_task,  //
                                                      ygg::ExecutionContext& execution_context,
                                                      const GroundTaskInstantiationOptions& options)
{
    /**
     * Execute datalog program.
     */

    auto ground_program = GroundTaskProgram(lifted_task.get_task());
    auto workspace = d::ProgramWorkspace<LiftedTag>(ground_program.get_datalog_program());
    auto ctx = d::ProgramExecutionContext(workspace);

    execution_context.arena().execute([&] { d::solve_bottom_up(ctx); });

    auto predicate_bindings = std::vector<fd::PredicateBindingView<f::FluentTag>> {};
    for (const auto& set : workspace.facts.fact_sets.predicate.get_sets())
        for (const auto binding : set.get_bindings())
            predicate_bindings.push_back(binding);
    std::sort(predicate_bindings.begin(), predicate_bindings.end(), d::canonical_binding_less<fd::PredicateBindingView<f::FluentTag>>);

    /**
     * Create basic structures of task
     */

    const auto& planning_task = lifted_task.get_formalism_task();
    const auto& planning_domain = planning_task.get_domain();

    auto task = planning_task.get_task();
    auto repository = planning_domain.get_repository_factory()->create_shared(task.get_domain().get_constants().size() + task.get_objects().size(),
                                                                              planning_domain.get_repository().get());
    auto builder = fp::Builder();

    auto fdr_task_ptr = builder.get_builder<fp::FDRTask>();
    auto& fdr_task = *fdr_task_ptr;
    fdr_task.clear();

    auto merge_context = fp::MergeContext { builder, *repository };

    fdr_task.name = task.get_name();
    fdr_task.domain = task.get_domain().get_index();
    for (const auto predicate : task.get_derived_predicates())
        fdr_task.derived_predicates.push_back(merge_p2p(predicate, merge_context).first.get_index());
    for (const auto object : task.get_objects())
        fdr_task.objects.push_back(merge_p2p(object, merge_context).first.get_index());

    auto initial_atoms = fp::GroundAtomViewList<f::FluentTag> {};
    for (const auto atom : task.get_atoms<f::FluentTag>())
        initial_atoms.push_back(merge_p2p(atom, merge_context).first);

    /**
     * Create ground atoms
     */

    const auto for_each_predicate_binding = [&](auto&& f)
    {
        for (const auto binding : predicate_bindings)
            f(binding);
    };

    auto fluent_atoms = fp::GroundAtomViewList<f::FluentTag> {};
    auto derived_atoms = fp::GroundAtomViewList<f::DerivedTag> {};
    {
        auto merge_planning_context = fp::MergePlanningContext { builder, *repository };
        auto fluent_atom_ptr = builder.get_builder<fp::GroundAtom<f::FluentTag>>();
        auto& fluent_atom = *fluent_atom_ptr;
        auto derived_atom_ptr = builder.get_builder<fp::GroundAtom<f::DerivedTag>>();
        auto& derived_atom = *derived_atom_ptr;
        for_each_predicate_binding(
            [&](const auto binding)
            {
                if (ground_program.get_translation_context().d2p.fluent_to_fluent_predicate.contains(binding.get_relation()))
                {
                    fluent_atom.clear();
                    fluent_atom.binding = fp::merge_d2p<f::FluentTag, f::FluentTag>(binding,
                                                                                    ground_program.get_translation_context().d2p.fluent_to_fluent_predicate,
                                                                                    merge_planning_context)
                                              .first.get_index();
                    canonicalize(fluent_atom);
                    fluent_atoms.push_back(repository->get_or_create(fluent_atom).first);
                }
                else if (ground_program.get_translation_context().d2p.fluent_to_derived_predicate.contains(binding.get_relation()))
                {
                    derived_atom.clear();
                    derived_atom.binding = fp::merge_d2p<f::FluentTag, f::DerivedTag>(binding,
                                                                                      ground_program.get_translation_context().d2p.fluent_to_derived_predicate,
                                                                                      merge_planning_context)
                                               .first.get_index();
                    canonicalize(derived_atom);
                    derived_atoms.push_back(repository->get_or_create(derived_atom).first);
                }
            });
    }
    std::sort(fluent_atoms.begin(), fluent_atoms.end());
    std::sort(derived_atoms.begin(), derived_atoms.end());

    auto fluent_atoms_set = ygg::UnorderedSet<fp::GroundAtomView<f::FluentTag>>(fluent_atoms.begin(), fluent_atoms.end());
    auto derived_atoms_set = ygg::UnorderedSet<fp::GroundAtomView<f::DerivedTag>>(derived_atoms.begin(), derived_atoms.end());

    // std::cout << "Fluent atoms:" << std::endl;
    // std::cout << fluent_atoms << std::endl;
    // std::cout << "Derived atoms:" << std::endl;
    // std::cout << derived_atoms << std::endl;

    auto fdr_context = std::shared_ptr<fp::FDRContext> { nullptr };

    if (options.disable_invariant_synthesis)
    {
        fdr_context = std::make_shared<fp::FDRContext>(fluent_atoms, repository);
    }
    else
    {
        auto invariants = fpi::synthesize_invariants(planning_domain.get_domain());
        auto mutex_groups = fpi::compute_mutex_groups(initial_atoms, fluent_atoms, invariants);

        // std::cout << "Invariants:" << std::endl;
        // print(std::cout, invariants);
        // std::cout << std::endl;
        // std::cout << "Mutex groups:" << std::endl;
        // print(std::cout, mutex_groups);
        // std::cout << std::endl;

        fdr_context = std::make_shared<fp::FDRContext>(mutex_groups, repository);
    }

    for (const auto atom : task.get_atoms<f::StaticTag>())
        fdr_task.static_atoms.push_back(merge_p2p(atom, merge_context).first.get_index());
    for (const auto atom : fluent_atoms)
        fdr_task.fluent_atoms.push_back(merge_p2p(atom, merge_context).first.get_index());
    for (const auto atom : derived_atoms)
        fdr_task.derived_atoms.push_back(merge_p2p(atom, merge_context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<f::StaticTag>())
        fdr_task.static_fterm_values.push_back(merge_p2p(fterm_value, merge_context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<f::FluentTag>())
        fdr_task.fluent_fterm_values.push_back(merge_p2p(fterm_value, merge_context).first.get_index());
    if (task.get_auxiliary_fterm_value().has_value())
        fdr_task.auxiliary_fterm_value = merge_p2p(task.get_auxiliary_fterm_value().value(), merge_context).first.get_index();
    if (task.get_metric())
        fdr_task.metric = merge_p2p(task.get_metric().value(), merge_context).first.get_index();
    for (const auto axiom : task.get_axioms())
        fdr_task.axioms.push_back(merge_p2p(axiom, merge_context).first.get_index());

    /// --- Create FDR variables
    for (const auto variable : fdr_context->get_variables())
        fdr_task.fluent_variables.push_back(variable.get_index());

    /// --- Create FDR fluent facts
    for (const auto atom : task.get_atoms<f::FluentTag>())
        if (const auto fact = std::as_const(*fdr_context).get_fact(merge_p2p(atom, merge_context).first))
            fdr_task.fluent_facts.push_back(fact->get_data());

    auto static_atoms_bitset = boost::dynamic_bitset<>();
    for (const auto atom : task.get_atoms<f::StaticTag>())
        ygg::set(ygg::uint_t(atom.get_index()), true, static_atoms_bitset);

    /// --- Create FDR goal
    const auto goal_or_nullopt =
        create_ground_fdr_conjunctive_condition(task.get_goal(), fluent_atoms_set, derived_atoms_set, static_atoms_bitset, *fdr_context, merge_context);
    if (goal_or_nullopt.has_value())
        fdr_task.goal = goal_or_nullopt->get_index();
    else
        return GroundTaskInstantiationResult { nullptr, GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE };

    /// --- Create FDR actions and axioms

    auto fluent_assign = ygg::UnorderedMap<ygg::Index<fp::FDRVariable<f::FluentTag>>, fp::FDRValue> {};
    auto derived_assign = ygg::UnorderedMap<ygg::Index<fp::GroundAtom<f::DerivedTag>>, bool> {};
    auto iter_workspace = ygg::itertools::cartesian_set::Workspace<ygg::Index<f::Object>> {};

    for_each_predicate_binding(
        [&](const auto binding)
        {
            const auto& mapping = ground_program.get_predicate_to_action_mapping();

            if (const auto it = mapping.find(binding.get_relation()); it != mapping.end())
            {
                const auto action = it->second;

                workspace.binding.clear();
                for (const auto object : binding.get_objects())
                    workspace.binding.push_back(object.get_index());

                auto grounder_context = fp::GrounderContext { workspace.planning_builder, *repository, workspace.binding };

                const auto ground_action_or_nullopt =
                    ground_pruned(action,
                                  fluent_atoms_set,
                                  derived_atoms_set,
                                  lifted_task.get_formalism_task().get_variable_domains().action_domains.at(action.get_index()),
                                  static_atoms_bitset,
                                  iter_workspace,
                                  grounder_context,
                                  *fdr_context);

                if (!ground_action_or_nullopt.has_value())
                    return;

                const auto& ground_action = ground_action_or_nullopt.value();

                assert(is_statically_applicable(ground_action, static_atoms_bitset));

                if (is_consistent(ground_action, fluent_assign, derived_assign))
                {
                    fdr_task.ground_actions.push_back(ground_action.get_index());
                }
            }
        });

    /// --- Ground Axioms

    for_each_predicate_binding(
        [&](const auto binding)
        {
            const auto& mapping = ground_program.get_predicate_to_axiom_mapping();

            if (const auto it = mapping.find(binding.get_relation()); it != mapping.end())
            {
                workspace.binding.clear();
                for (const auto object : binding.get_objects())
                    workspace.binding.push_back(object.get_index());

                auto grounder_context = fp::GrounderContext { workspace.planning_builder, *repository, workspace.binding };

                for (const auto axiom : it->second)
                {
                    const auto ground_axiom_or_nullopt =
                        ground_pruned(axiom, fluent_atoms_set, derived_atoms_set, static_atoms_bitset, grounder_context, *fdr_context);

                    if (!ground_axiom_or_nullopt.has_value())
                        continue;

                    const auto& ground_axiom = ground_axiom_or_nullopt.value();

                    assert(is_statically_applicable(ground_axiom, static_atoms_bitset));

                    if (is_consistent(ground_axiom, fluent_assign, derived_assign))
                    {
                        fdr_task.ground_axioms.push_back(ground_axiom.get_index());
                    }
                }
            }
        });

    canonicalize(fdr_task);

    return GroundTaskInstantiationResult {
        std::make_shared<Task<GroundTag>>(
            fp::PlanningFDRTask(repository->get_or_create(fdr_task).first, std::move(fdr_context), repository, planning_task.get_domain())),
        GroundTaskInstantiationStatus::SUCCESS
    };
}

}
