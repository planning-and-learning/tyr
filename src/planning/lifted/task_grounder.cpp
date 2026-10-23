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

#include "tyr/formalism/planning/interning.hpp"
#include "tyr/planning/lifted/task_grounder.hpp"

#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/solver.hpp"
#include "effect_atom_index.hpp"
#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/planning/copy.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/invariants/mutexes.hpp"
#include "tyr/formalism/planning/invariants/synthesis.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/formalism/planning/mutable/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/static_state.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

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

template<formalism::FactKind T>
RemapStatus
classify_literal(formalism::planning::AtomView<GroundTag, T> atom, bool polarity, const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, T>>& atoms)
{
    const bool present = atoms.contains(atom);

    if (polarity)
        return present ? RemapStatus::mapped : RemapStatus::contradiction;

    return present ? RemapStatus::mapped : RemapStatus::tautology;
}

template<formalism::FactKind T>
RemapStatus classify_literal(formalism::planning::LiteralView<GroundTag, T> literal,
                             const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, T>>& atoms)
{
    return classify_literal(literal.get_atom(), literal.get_polarity(), atoms);
}

struct RemappedFDRFact
{
    RemapStatus status;
    std::optional<formalism::planning::FDRFactView<formalism::FluentTag>> fact;
};

RemappedFDRFact remap_fdr_fact(formalism::planning::FDRFactView<formalism::FluentTag> fact,
                               bool polarity,
                               const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
                               const formalism::planning::FDRContext& fdr_context,
                               formalism::planning::CopyContext& context)
{
    assert(fact.has_value());

    const auto new_atom = copy(fact.get_atom().value(), context).first;
    const auto status = classify_literal(new_atom, polarity, fluent_atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    const auto new_fact = fdr_context.get_fact(new_atom);
    if (!new_fact)
        return { RemapStatus::contradiction, std::nullopt };

    return { RemapStatus::mapped, *new_fact };
}

template<formalism::FactKind T>
struct RemappedLiteral
{
    RemapStatus status;
    std::optional<formalism::planning::LiteralView<GroundTag, T>> literal;
};

template<formalism::FactKind T>
RemappedLiteral<T> remap_literal(formalism::planning::LiteralView<GroundTag, T> literal,
                                 formalism::planning::CopyContext& context,
                                 const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, T>>& atoms)
{
    const auto new_literal = copy(literal, context).first;
    const auto status = classify_literal(new_literal, atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    return { RemapStatus::mapped, new_literal };
}

template<formalism::FactKind T>
RemappedLiteral<T> remap_literal(formalism::planning::LiteralView<GroundTag, T> literal,
                                 const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, T>>& atoms)
{
    const auto status = classify_literal(literal, atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    return { RemapStatus::mapped, literal };
}

std::optional<formalism::planning::ConjunctiveConditionView<GroundTag>>
create_ground_fdr_conjunctive_condition(formalism::planning::ConjunctiveConditionView<GroundTag> element,
                                        const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
                                        const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>& derived_atoms,
                                        const StaticState& static_state,
                                        const formalism::planning::FDRContext& fdr_context,
                                        formalism::planning::CopyContext& context)
{
    auto fdr_conj_cond = formalism::planning::checkout<formalism::planning::ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto literal : element.get_literals<formalism::StaticTag>())
    {
        const auto new_literal = copy(literal, context).first;
        if (!is_statically_applicable(new_literal, static_state))
            return std::nullopt;
    }

    for (const auto literal : element.get_literals<formalism::DerivedTag>())
    {
        const auto remapped = remap_literal(literal, context, derived_atoms);

        if (remapped.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped.status == RemapStatus::tautology)
            continue;

        fdr_conj_cond->derived_literals.push_back(remapped.literal->get_index());
    }

    for (const auto fact : element.get_facts<formalism::PositiveTag>())
    {
        const auto remapped_fdr_fact = remap_fdr_fact(fact, true, fluent_atoms, fdr_context, context);

        if (remapped_fdr_fact.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped_fdr_fact.status == RemapStatus::tautology)
            continue;

        fdr_conj_cond->positive_facts.push_back(remapped_fdr_fact.fact->get_data());
    }

    for (const auto fact : element.get_facts<formalism::NegativeTag>())
    {
        const auto remapped_fdr_fact = remap_fdr_fact(fact, false, fluent_atoms, fdr_context, context);

        if (remapped_fdr_fact.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped_fdr_fact.status == RemapStatus::tautology)
            continue;

        fdr_conj_cond->negative_facts.push_back(remapped_fdr_fact.fact->get_data());
    }

    for (const auto numeric_constraint : element.get_numeric_constraints())
        fdr_conj_cond->numeric_constraints.push_back(copy(numeric_constraint, context).get_data());

    return formalism::planning::insert(context.destination, *fdr_conj_cond).first;
}

std::optional<formalism::planning::ConjunctiveConditionView<GroundTag>>
ground_pruned(formalism::planning::ConjunctiveConditionView<LiftedTag> element,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>& derived_atoms,
              const StaticState& static_state,
              formalism::planning::GrounderContext& context,
              const formalism::planning::FDRContext& fdr_context)
{
    auto conj_cond = formalism::planning::checkout<formalism::planning::ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto literal : element.template get_literals<formalism::StaticTag>())
    {
        const auto new_literal = ground(literal, context).first;
        if (!is_statically_applicable(new_literal, static_state))
            return std::nullopt;
    }

    for (const auto literal : element.template get_literals<formalism::FluentTag>())
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
            conj_cond->positive_facts.push_back(new_fact->get_data());
        else
            conj_cond->negative_facts.push_back(new_fact->get_data());
    }

    for (const auto literal : element.template get_literals<formalism::DerivedTag>())
    {
        const auto new_literal = ground(literal, context).first;
        const auto remapped = remap_literal(new_literal, derived_atoms);

        if (remapped.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped.status == RemapStatus::tautology)
            continue;

        conj_cond->derived_literals.push_back(remapped.literal->get_index());
    }

    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(ground(numeric_constraint, context).get_data());

    return formalism::planning::insert(context.destination, *conj_cond).first;
}

std::optional<formalism::planning::ConjunctiveEffectView<GroundTag>>
ground_pruned(formalism::planning::ConjunctiveEffectView<LiftedTag> element,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
              formalism::planning::GrounderContext& context,
              const formalism::planning::FDRContext& fdr)
{
    // Fetch and clear
    auto conj_eff = formalism::planning::checkout<formalism::planning::ConjunctiveEffect<GroundTag>>(context.builder);

    for (const auto literal : element.get_literals())
    {
        const auto new_atom = ground(literal.get_atom(), context).first;
        if (!fluent_atoms.contains(new_atom))
            return std::nullopt;

        const auto new_fact = fdr.get_fact(new_atom);
        if (!new_fact)
            return std::nullopt;

        if (literal.get_polarity())
            conj_eff->add_facts.push_back(new_fact->get_data());
        else
            conj_eff->del_facts.push_back(new_fact->get_data());
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        conj_eff->numeric_effects.push_back(ground(numeric_effect, context).get_data());
    if (element.get_auxiliary_numeric_effect().has_value())
        conj_eff->auxiliary_numeric_effect = ground(element.get_auxiliary_numeric_effect().value(), context).get_data();

    // Prune no-op effects
    if (conj_eff->add_facts.empty() && conj_eff->del_facts.empty() && conj_eff->numeric_effects.empty() && !conj_eff->auxiliary_numeric_effect)
        return std::nullopt;  // no-op

    // Canonicalize and Serialize
    return formalism::planning::insert(context.destination, *conj_eff).first;
}

std::optional<formalism::planning::ConditionalEffectView<GroundTag>>
ground_pruned(formalism::planning::ConditionalEffectView<LiftedTag> element,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>& derived_atoms,
              const StaticState& static_state,
              formalism::planning::GrounderContext& context,
              const formalism::planning::FDRContext& fdr_context)
{
    // Fetch and clear
    auto cond_effect = formalism::planning::checkout<formalism::planning::ConditionalEffect<GroundTag>>(context.builder);

    // Fill data
    const auto new_condition_or_nullopt = ground_pruned(element.get_condition(), fluent_atoms, derived_atoms, static_state, context, fdr_context);
    if (!new_condition_or_nullopt)
        return std::nullopt;

    cond_effect->condition = new_condition_or_nullopt->get_index();

    const auto new_effect_or_nullopt = ground_pruned(element.get_effect(), fluent_atoms, context, fdr_context);
    if (!new_effect_or_nullopt)
        return std::nullopt;

    cond_effect->effect = new_effect_or_nullopt->get_index();

    // Canonicalize and Serialize
    return formalism::planning::insert(context.destination, *cond_effect).first;
}

/// Grounds the action binding of the context with its effects grounded for each completed binding that `for_each_effect_binding`
/// puts into the context's binding, dropping the parts that no reachable atom can satisfy.
template<typename ForEachEffectBinding>
std::optional<formalism::planning::ActionView<GroundTag>>
ground_pruned(formalism::planning::ActionView<LiftedTag> element,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>& derived_atoms,
              ForEachEffectBinding&& for_each_effect_binding,
              const StaticState& static_state,
              formalism::planning::GrounderContext& context,
              const formalism::planning::FDRContext& fdr_context)
{
    // Fetch and clear
    auto action = formalism::planning::checkout<formalism::planning::Action<GroundTag>>(context.builder);

    // Fill data
    action->binding = ground(element, context).first.get_index();

    const auto new_condition_or_nullopt = ground_pruned(element.get_condition(), fluent_atoms, derived_atoms, static_state, context, fdr_context);
    if (!new_condition_or_nullopt)
        return std::nullopt;

    action->condition = new_condition_or_nullopt->get_index();

    for (const auto cond_effect : element.get_effects())
        for_each_effect_binding(cond_effect,
                                [&]
                                {
                                    const auto ground_cond_effect_or_nullopt =
                                        ground_pruned(cond_effect, fluent_atoms, derived_atoms, static_state, context, fdr_context);
                                    if (ground_cond_effect_or_nullopt.has_value())
                                        action->effects.push_back(ground_cond_effect_or_nullopt->get_index());
                                });

    if (action->effects.empty())
        return std::nullopt;

    // Canonicalize and Serialize
    return formalism::planning::insert(context.destination, *action).first;
}

std::optional<formalism::planning::AxiomView<GroundTag>>
ground_pruned(formalism::planning::AxiomView<LiftedTag> element,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>& fluent_atoms,
              const ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>& derived_atoms,
              const StaticState& static_state,
              formalism::planning::GrounderContext& context,
              const formalism::planning::FDRContext& fdr_context)
{
    // Fetch and clear
    auto axiom = formalism::planning::checkout<formalism::planning::Axiom<GroundTag>>(context.builder);

    // Fill data
    axiom->binding = ground(element, context).first.get_index();

    const auto new_body_or_nullopt = ground_pruned(element.get_body(), fluent_atoms, derived_atoms, static_state, context, fdr_context);
    if (!new_body_or_nullopt.has_value())
        return std::nullopt;  // body is false in all reachable states -> axiom is irrelevant

    axiom->body = new_body_or_nullopt->get_index();

    const auto new_head = ground(element.get_head(), context).first;
    if (!derived_atoms.contains(new_head))
        return std::nullopt;  // head is false in all reachable states -> axiom is irrelevant

    axiom->head = new_head.get_index();

    // Canonicalize and Serialize
    return formalism::planning::insert(context.destination, *axiom).first;
}

/// Rejects orders that are not a sequence of distinct invariants synthesized for this domain and interned into its
/// repository. Views of different repository trees can compare equal, so the repository is compared by address;
/// `synthesized` was interned below it and shares the indices of invariants interned there.
void validate_invariant_order(const formalism::planning::InvariantViewList& invariant_order,
                              const formalism::planning::InvariantViewList& synthesized,
                              const formalism::planning::Repository& domain_repository)
{
    using formalism::planning::Invariant;

    auto known = ygg::UnorderedSet<ygg::Index<Invariant>> {};
    for (const auto invariant : synthesized)
        known.insert(invariant.get_index());
    auto seen = ygg::UnorderedSet<ygg::Index<Invariant>> {};
    for (const auto invariant : invariant_order)
    {
        if (&invariant.get_context() != &domain_repository || !known.contains(invariant.get_index()))
            throw std::invalid_argument(fmt::format("Invariant order contains an invariant not synthesized for this domain:\n{}", invariant));
        if (!seen.insert(invariant.get_index()).second)
            throw std::invalid_argument(fmt::format("Invariant order contains an invariant twice:\n{}", invariant));
    }
}

/// The FDR variables' atoms. Synthesis interns into `repository`, the ground task's new layer below the domain's
/// repository: invariants already interned in the domain's repository are found there, others are added to the layer.
std::vector<formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>>
choose_mutex_groups(const formalism::planning::PlanningDomain& planning_domain,
                    formalism::planning::Repository& repository,
                    const formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>& initial_atoms,
                    const formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>& fluent_atoms,
                    const std::optional<formalism::planning::InvariantViewList>& invariant_order)
{
    namespace invariant = formalism::planning::invariant;

    auto groups = std::vector<invariant::MutexGroup> {};
    if (!invariant_order.has_value())
    {
        groups = invariant::compute_mutex_groups(initial_atoms, fluent_atoms, invariant::synthesize_invariants(planning_domain.get_domain(), repository));
    }
    else
    {
        if (!invariant_order->empty())
            validate_invariant_order(*invariant_order,
                                     invariant::synthesize_invariants(planning_domain.get_domain(), repository),
                                     *planning_domain.get_repository());
        groups = invariant::compute_mutex_groups_in_order(initial_atoms, fluent_atoms, *invariant_order);
    }

    auto result = std::vector<formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>> {};
    result.reserve(groups.size());
    for (auto& group : groups)
        result.push_back(std::move(group.atoms));
    return result;
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
    auto workspace = datalog::ProgramWorkspace<LiftedTag>(ground_program.get_datalog_program());
    auto ctx = datalog::ProgramExecutionContext(workspace);
    datalog::execute_model(ctx, execution_context);

    auto predicate_bindings = std::vector<formalism::datalog::PredicateBindingView<formalism::FluentTag>> {};
    for (const auto& set : workspace.facts.fact_sets.predicate.get_sets())
        for (const auto binding : set.get_bindings())
            predicate_bindings.push_back(binding);
    /**
     * Create basic structures of task
     */

    const auto& planning_task = lifted_task.get_formalism_task();
    const auto& planning_domain = planning_task.get_domain();

    auto task = planning_task.get_task();
    auto repository = planning_domain.get_repository_factory()->create_shared(task.get_num_objects(), planning_domain.get_repository().get());
    auto builder = formalism::planning::Builder();

    auto ground_task = formalism::planning::checkout<formalism::planning::Task<GroundTag>>(builder);

    auto merge_context = formalism::planning::CopyContext { builder, *repository };

    ground_task->name = task.get_name();
    ground_task->domain = task.get_domain().get_index();
    for (const auto predicate : task.get_derived_predicates())
        ground_task->derived_predicates.push_back(copy(predicate, merge_context).first.get_index());
    for (const auto object : task.get_objects())
        ground_task->objects.push_back(copy(object, merge_context).first.get_index());

    auto initial_atoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> {};
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        initial_atoms.push_back(copy(atom, merge_context).first);

    /**
     * Create ground atoms
     */

    const auto for_each_predicate_binding = [&](auto&& f)
    {
        for (const auto binding : predicate_bindings)
            f(binding);
    };

    auto fluent_atoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> {};
    auto derived_atoms = formalism::planning::AtomViewList<GroundTag, formalism::DerivedTag> {};
    {
        auto merge_planning_context = formalism::planning::MergePlanningContext { builder, *repository };
        for_each_predicate_binding(
            [&](const auto binding)
            {
                if (ground_program.get_translation_context().d2p.fluent_to_fluent_predicate.contains(binding.get_relation()))
                {
                    auto fluent_atom = formalism::planning::checkout<formalism::planning::Atom<GroundTag, formalism::FluentTag>>(builder);
                    fluent_atom->binding = formalism::planning::merge_d2p<formalism::FluentTag, formalism::FluentTag>(
                                               binding,
                                               ground_program.get_translation_context().d2p.fluent_to_fluent_predicate,
                                               merge_planning_context)
                                               .first.get_index();
                    fluent_atoms.push_back(formalism::planning::insert(*repository, *fluent_atom).first);
                }
                else if (ground_program.get_translation_context().d2p.fluent_to_derived_predicate.contains(binding.get_relation()))
                {
                    auto derived_atom = formalism::planning::checkout<formalism::planning::Atom<GroundTag, formalism::DerivedTag>>(builder);
                    derived_atom->binding = formalism::planning::merge_d2p<formalism::FluentTag, formalism::DerivedTag>(
                                                binding,
                                                ground_program.get_translation_context().d2p.fluent_to_derived_predicate,
                                                merge_planning_context)
                                                .first.get_index();
                    derived_atoms.push_back(formalism::planning::insert(*repository, *derived_atom).first);
                }
            });
    }
    std::sort(fluent_atoms.begin(), fluent_atoms.end());
    std::sort(derived_atoms.begin(), derived_atoms.end());

    auto fluent_atoms_set = ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>(fluent_atoms.begin(), fluent_atoms.end());
    auto derived_atoms_set = ygg::UnorderedSet<formalism::planning::AtomView<GroundTag, formalism::DerivedTag>>(derived_atoms.begin(), derived_atoms.end());

    // std::cout << "Fluent atoms:" << std::endl;
    // std::cout << fluent_atoms << std::endl;
    // std::cout << "Derived atoms:" << std::endl;
    // std::cout << derived_atoms << std::endl;

    auto fdr_context = std::shared_ptr<formalism::planning::FDRContext> { nullptr };

    if (options.disable_invariant_synthesis)
    {
        fdr_context = std::make_shared<formalism::planning::FDRContext>(fluent_atoms, repository);
    }
    else
    {
        auto mutex_groups = choose_mutex_groups(planning_domain, *repository, initial_atoms, fluent_atoms, options.invariant_order);

        fdr_context = std::make_shared<formalism::planning::FDRContext>(mutex_groups, repository);
    }

    for (const auto atom : task.get_atoms<formalism::StaticTag>())
        ground_task->static_atoms.push_back(copy(atom, merge_context).first.get_index());
    for (const auto atom : fluent_atoms)
        ground_task->fluent_atoms.push_back(copy(atom, merge_context).first.get_index());
    for (const auto atom : derived_atoms)
        ground_task->derived_atoms.push_back(copy(atom, merge_context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<formalism::StaticTag>())
        ground_task->static_fterm_values.push_back(copy(fterm_value, merge_context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<formalism::FluentTag>())
        ground_task->fluent_fterm_values.push_back(copy(fterm_value, merge_context).first.get_index());
    if (task.get_auxiliary_fterm_value().has_value())
        ground_task->auxiliary_fterm_value = copy(task.get_auxiliary_fterm_value().value(), merge_context).first.get_index();
    if (task.get_metric())
        ground_task->metric = copy(task.get_metric().value(), merge_context).first.get_index();
    for (const auto axiom : task.get_axioms())
        ground_task->axioms.push_back(copy(axiom, merge_context).first.get_index());

    /// --- Create FDR variables
    for (const auto variable : fdr_context->get_variables())
        ground_task->fluent_variables.push_back(variable.get_index());

    /// --- Create FDR fluent facts
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        if (const auto fact = std::as_const(*fdr_context).get_fact(copy(atom, merge_context).first))
            ground_task->fluent_facts.push_back(fact->get_data());

    const auto static_state =
        StaticState(ygg::make_view(ground_task->static_atoms, *repository), ygg::make_view(ground_task->static_fterm_values, *repository));

    /// --- Create FDR goal
    const auto goal_or_nullopt =
        create_ground_fdr_conjunctive_condition(task.get_goal(), fluent_atoms_set, derived_atoms_set, static_state, *fdr_context, merge_context);
    if (goal_or_nullopt.has_value())
        ground_task->goal = goal_or_nullopt->get_index();
    else
        return GroundTaskInstantiationResult { nullptr, GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE };

    /// --- Create FDR actions and axioms

    auto fluent_assign = ygg::UnorderedMap<ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>>, formalism::planning::FDRValue> {};
    auto derived_assign = ygg::UnorderedMap<ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>>, bool> {};

    // The atoms of the effect predicates are the relaxed reachable groundings of the conditional effects.
    auto effect_atom_index = EffectAtomIndex(ground_program.get_predicate_to_action_mapping(),
                                             ground_program.get_conditional_effect_to_predicate_mapping(),
                                             workspace.facts.fact_sets.predicate,
                                             workspace.workspace_repository);

    for_each_predicate_binding(
        [&](const auto binding)
        {
            const auto& mapping = ground_program.get_predicate_to_action_mapping();

            if (const auto it = mapping.find(binding.get_relation()); it != mapping.end())
            {
                const auto action = it->second;

                workspace.binding.clear();
                ygg::extend(binding.get_objects(), workspace.binding);

                auto grounder_context = formalism::planning::GrounderContext { workspace.planning_builder, *repository, workspace.binding };

                const auto ground_action_or_nullopt = ground_pruned(
                    action,
                    fluent_atoms_set,
                    derived_atoms_set,
                    [&](auto effect, auto&& apply) { effect_atom_index.for_each_effect_binding(action, effect, grounder_context.binding, apply); },
                    static_state,
                    grounder_context,
                    *fdr_context);

                if (!ground_action_or_nullopt.has_value())
                    return;

                const auto& ground_action = ground_action_or_nullopt.value();

                assert(is_statically_applicable(ground_action, static_state));

                if (is_consistent(ground_action, fluent_assign, derived_assign))
                {
                    ground_task->ground_actions.push_back(ground_action.get_index());
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
                ygg::extend(binding.get_objects(), workspace.binding);

                auto grounder_context = formalism::planning::GrounderContext { workspace.planning_builder, *repository, workspace.binding };

                for (const auto axiom : it->second)
                {
                    const auto ground_axiom_or_nullopt =
                        ground_pruned(axiom, fluent_atoms_set, derived_atoms_set, static_state, grounder_context, *fdr_context);

                    if (!ground_axiom_or_nullopt.has_value())
                        continue;

                    const auto& ground_axiom = ground_axiom_or_nullopt.value();

                    assert(is_statically_applicable(ground_axiom, static_state));

                    if (is_consistent(ground_axiom, fluent_assign, derived_assign))
                    {
                        ground_task->ground_axioms.push_back(ground_axiom.get_index());
                    }
                }
            }
        });

    return GroundTaskInstantiationResult { std::make_shared<Task<GroundTag>>(
                                               formalism::planning::PlanningTask<GroundTag>(formalism::planning::insert(*repository, *ground_task).first,
                                                                                            std::move(fdr_context),
                                                                                            repository,
                                                                                            planning_task.get_domain(),
                                                                                            planning_task.get_path())),
                                           GroundTaskInstantiationStatus::SUCCESS };
}
}
