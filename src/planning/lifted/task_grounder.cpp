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

#include "tyr/formalism/grounding_context.hpp"
#include "tyr/planning/lifted/task_grounder.hpp"

#include "effect_atom_index.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/invariants/mutexes.hpp"
#include "tyr/formalism/planning/invariants/synthesis.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/static_state.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <urd/config.hpp>
#include <urd/elements/copy.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{

enum class RemapStatus : std::uint8_t
{
    mapped,
    tautology,
    contradiction,
};

template<formalism::FactKind T>
RemapStatus classify_literal(formalism::AtomView<GroundTag, T> atom, bool polarity, const urd::Store<urd::set<formalism::AtomView<GroundTag, T>>>& atoms)
{
    const bool present = atoms.contains(atom);

    if (polarity)
        return present ? RemapStatus::mapped : RemapStatus::contradiction;

    return present ? RemapStatus::mapped : RemapStatus::tautology;
}

template<formalism::FactKind T>
RemapStatus classify_literal(formalism::LiteralView<GroundTag, T> literal, const urd::Store<urd::set<formalism::AtomView<GroundTag, T>>>& atoms)
{ return classify_literal(literal.get_atom(), literal.get_polarity(), atoms); }

struct RemappedFDRFact
{
    RemapStatus status = RemapStatus::contradiction;
    std::optional<formalism::FDRFactView<formalism::FluentTag>> fact;
};

RemappedFDRFact remap_fdr_fact(formalism::FDRFactView<formalism::FluentTag> fact,
                               bool polarity,
                               const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
                               const formalism::FDRContext& fdr_context,
                               formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    assert(fact.has_value());

    const auto atom = fact.get_atom();
    URD_ASSUME(atom.has_value());  // Asserted above: the fact has a value, hence an atom.
    const auto new_atom = context.destination().view(urd::copy(*atom, context.destination(), context.builder()));
    const auto status = classify_literal(new_atom, polarity, fluent_atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    const auto new_fact = fdr_context.get_fact(new_atom);
    if (!new_fact)
        return { RemapStatus::contradiction, std::nullopt };

    return { RemapStatus::mapped, new_fact };
}

template<formalism::FactKind T>
struct RemappedLiteral
{
    RemapStatus status = RemapStatus::contradiction;
    std::optional<formalism::LiteralView<GroundTag, T>> literal;
};

template<formalism::FactKind T>
RemappedLiteral<T> remap_literal(formalism::LiteralView<GroundTag, T> literal,
                                 formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
                                 const urd::Store<urd::set<formalism::AtomView<GroundTag, T>>>& atoms)
{
    const auto new_literal = context.destination().view(urd::copy(literal, context.destination(), context.builder()));
    const auto status = classify_literal(new_literal, atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    return { RemapStatus::mapped, new_literal };
}

template<formalism::FactKind T>
RemappedLiteral<T> remap_literal(formalism::LiteralView<GroundTag, T> literal, const urd::Store<urd::set<formalism::AtomView<GroundTag, T>>>& atoms)
{
    const auto status = classify_literal(literal, atoms);

    if (status != RemapStatus::mapped)
        return { status, std::nullopt };

    return { RemapStatus::mapped, literal };
}

std::optional<formalism::ConjunctiveConditionView<GroundTag>>
create_ground_fdr_conjunctive_condition(formalism::ConjunctiveConditionView<GroundTag> element,
                                        const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
                                        const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::DerivedTag>>>& derived_atoms,
                                        const StaticState& static_state,
                                        const formalism::FDRContext& fdr_context,
                                        formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto fdr_conj_cond = formalism::checkout<formalism::ConjunctiveCondition<GroundTag>>(context.builder());

    for (const auto literal : element.get_literals<formalism::StaticTag>())
    {
        const auto new_literal = context.destination().view(urd::copy(literal, context.destination(), context.builder()));
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

        URD_ASSUME(remapped.literal.has_value());  // Mapped literals are present.

        urd::append(fdr_conj_cond->derived_literals, *remapped.literal);
    }

    for (const auto fact : element.get_facts<formalism::PositiveTag>())
    {
        const auto remapped_fdr_fact = remap_fdr_fact(fact, true, fluent_atoms, fdr_context, context);

        if (remapped_fdr_fact.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped_fdr_fact.status == RemapStatus::tautology)
            continue;

        URD_ASSUME(remapped_fdr_fact.fact.has_value());  // Mapped facts are present.

        urd::build_member(fdr_conj_cond->positive_facts.emplace_back(),
                          urd::make<formalism::FDRFact<formalism::FluentTag>>(remapped_fdr_fact.fact->get_variable(), remapped_fdr_fact.fact->get_value()),
                          context.destination());
    }

    for (const auto fact : element.get_facts<formalism::NegativeTag>())
    {
        const auto remapped_fdr_fact = remap_fdr_fact(fact, false, fluent_atoms, fdr_context, context);

        if (remapped_fdr_fact.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped_fdr_fact.status == RemapStatus::tautology)
            continue;

        URD_ASSUME(remapped_fdr_fact.fact.has_value());  // Mapped facts are present.

        urd::build_member(fdr_conj_cond->negative_facts.emplace_back(),
                          urd::make<formalism::FDRFact<formalism::FluentTag>>(remapped_fdr_fact.fact->get_variable(), remapped_fdr_fact.fact->get_value()),
                          context.destination());
    }

    for (const auto numeric_constraint : element.get_numeric_constraints())
        urd::append(fdr_conj_cond->numeric_constraints, urd::copy(numeric_constraint, context.destination(), context.builder()));

    return context.destination().insert(*fdr_conj_cond, context.builder()).first;
}

std::optional<formalism::ConjunctiveConditionView<GroundTag>>
ground_pruned(formalism::ConjunctiveConditionView<LiftedTag> element,
              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::DerivedTag>>>& derived_atoms,
              const StaticState& static_state,
              formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
              const formalism::FDRContext& fdr_context)
{
    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<GroundTag>>(context.builder());

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

        URD_ASSUME(remapped.literal.has_value());  // Mapped literals are present.

        const auto new_fact = fdr_context.get_fact(remapped.literal->get_atom());
        if (!new_fact)
            return std::nullopt;

        if (literal.get_polarity())
            urd::build_member(conj_cond->positive_facts.emplace_back(),
                              urd::make<formalism::FDRFact<formalism::FluentTag>>(new_fact->get_variable(), new_fact->get_value()),
                              context.destination());
        else
            urd::build_member(conj_cond->negative_facts.emplace_back(),
                              urd::make<formalism::FDRFact<formalism::FluentTag>>(new_fact->get_variable(), new_fact->get_value()),
                              context.destination());
    }

    for (const auto literal : element.template get_literals<formalism::DerivedTag>())
    {
        const auto new_literal = ground(literal, context).first;
        const auto remapped = remap_literal(new_literal, derived_atoms);

        if (remapped.status == RemapStatus::contradiction)
            return std::nullopt;

        if (remapped.status == RemapStatus::tautology)
            continue;

        URD_ASSUME(remapped.literal.has_value());  // Mapped literals are present.

        urd::append(conj_cond->derived_literals, *remapped.literal);
    }

    for (const auto numeric_constraint : element.get_numeric_constraints())
        urd::append(conj_cond->numeric_constraints, ground(numeric_constraint, context).first);

    return context.destination().insert(*conj_cond, context.builder()).first;
}

std::optional<formalism::ConjunctiveEffectView<GroundTag>>
ground_pruned(formalism::ConjunctiveEffectView<LiftedTag> element,
              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
              formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
              const formalism::FDRContext& fdr)
{
    // Fetch and clear
    auto conj_eff = formalism::checkout<formalism::ConjunctiveEffect<GroundTag>>(context.builder());

    for (const auto literal : element.get_literals())
    {
        const auto new_atom = ground(literal.get_atom(), context).first;
        if (!fluent_atoms.contains(new_atom))
            return std::nullopt;

        const auto new_fact = fdr.get_fact(new_atom);
        if (!new_fact)
            return std::nullopt;

        if (literal.get_polarity())
            urd::build_member(conj_eff->add_facts.emplace_back(),
                              urd::make<formalism::FDRFact<formalism::FluentTag>>(new_fact->get_variable(), new_fact->get_value()),
                              context.destination());
        else
            urd::build_member(conj_eff->del_facts.emplace_back(),
                              urd::make<formalism::FDRFact<formalism::FluentTag>>(new_fact->get_variable(), new_fact->get_value()),
                              context.destination());
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        urd::append(conj_eff->numeric_effects, ground(numeric_effect, context).first);
    if (element.get_auxiliary_numeric_effect().has_value())
        urd::store(conj_eff->auxiliary_numeric_effect, ground(element.get_auxiliary_numeric_effect().value(), context).first);

    // Prune no-op effects
    if (conj_eff->add_facts.empty() && conj_eff->del_facts.empty() && conj_eff->numeric_effects.empty() && !conj_eff->auxiliary_numeric_effect)
        return std::nullopt;  // no-op

    // Canonicalize and Serialize
    return context.destination().insert(*conj_eff, context.builder()).first;
}

std::optional<formalism::ConditionalEffectView<GroundTag>>
ground_pruned(formalism::ConditionalEffectView<LiftedTag> element,
              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::DerivedTag>>>& derived_atoms,
              const StaticState& static_state,
              formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
              const formalism::FDRContext& fdr_context)
{
    // Fetch and clear
    auto cond_effect = formalism::checkout<formalism::ConditionalEffect<GroundTag>>(context.builder());

    // Fill data
    const auto new_condition_or_nullopt = ground_pruned(element.get_condition(), fluent_atoms, derived_atoms, static_state, context, fdr_context);
    if (!new_condition_or_nullopt)
        return std::nullopt;

    urd::store(cond_effect->condition, *new_condition_or_nullopt);

    const auto new_effect_or_nullopt = ground_pruned(element.get_effect(), fluent_atoms, context, fdr_context);
    if (!new_effect_or_nullopt)
        return std::nullopt;

    urd::store(cond_effect->effect, *new_effect_or_nullopt);

    // Canonicalize and Serialize
    return context.destination().insert(*cond_effect, context.builder()).first;
}

/// Grounds the action binding of the context with its effects grounded for each completed binding that `for_each_effect_binding`
/// puts into the context's binding, dropping the parts that no reachable atom can satisfy.
template<typename ForEachEffectBinding>
std::optional<formalism::ActionView<GroundTag>> ground_pruned(formalism::ActionView<LiftedTag> element,
                                                              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
                                                              const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::DerivedTag>>>& derived_atoms,
                                                              ForEachEffectBinding for_each_effect_binding,
                                                              const StaticState& static_state,
                                                              formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
                                                              const formalism::FDRContext& fdr_context)
{
    // Fetch and clear
    auto action = formalism::checkout<formalism::Action<GroundTag>>(context.builder());

    // Fill data
    urd::store(action->row, ground(element, context).first);

    const auto new_condition_or_nullopt = ground_pruned(element.get_condition(), fluent_atoms, derived_atoms, static_state, context, fdr_context);
    if (!new_condition_or_nullopt)
        return std::nullopt;

    urd::store(action->condition, *new_condition_or_nullopt);

    for (const auto cond_effect : element.get_effects())
        for_each_effect_binding(cond_effect,
                                [&]
                                {
                                    const auto ground_cond_effect_or_nullopt =
                                        ground_pruned(cond_effect, fluent_atoms, derived_atoms, static_state, context, fdr_context);
                                    if (ground_cond_effect_or_nullopt.has_value())
                                        urd::append(action->effects, *ground_cond_effect_or_nullopt);
                                });

    if (action->effects.empty())
        return std::nullopt;

    // Canonicalize and Serialize
    return context.destination().insert(*action, context.builder()).first;
}

std::optional<formalism::AxiomView<GroundTag>> ground_pruned(formalism::AxiomView<LiftedTag> element,
                                                             const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>& fluent_atoms,
                                                             const urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::DerivedTag>>>& derived_atoms,
                                                             const StaticState& static_state,
                                                             formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
                                                             const formalism::FDRContext& fdr_context)
{
    // Fetch and clear
    auto axiom = formalism::checkout<formalism::Axiom<GroundTag>>(context.builder());

    // Fill data
    urd::store(axiom->row, ground(element, context).first);

    const auto new_body_or_nullopt = ground_pruned(element.get_body(), fluent_atoms, derived_atoms, static_state, context, fdr_context);
    if (!new_body_or_nullopt.has_value())
        return std::nullopt;  // body is false in all reachable states -> axiom is irrelevant

    urd::store(axiom->body, *new_body_or_nullopt);

    const auto new_head = ground(element.get_head(), context).first;
    if (!derived_atoms.contains(new_head))
        return std::nullopt;  // head is false in all reachable states -> axiom is irrelevant

    urd::store(axiom->head, new_head);

    // Canonicalize and Serialize
    return context.destination().insert(*axiom, context.builder()).first;
}

/// Rejects orders that are not a sequence of distinct invariants synthesized for this domain and interned into its
/// repository. Views of different repository trees can compare equal, so the repository is compared by address;
/// `synthesized` was interned below it and shares the indices of invariants interned there.
void validate_invariant_order(const formalism::InvariantViewList& invariant_order,
                              const formalism::InvariantViewList& synthesized,
                              const formalism::Repository& domain_repository)
{
    using formalism::Invariant;

    auto known = urd::Store<urd::set<urd::index<Invariant>>> {};
    for (const auto invariant : synthesized)
        known.insert(invariant.index());
    auto seen = urd::Store<urd::set<urd::index<Invariant>>> {};
    for (const auto invariant : invariant_order)
    {
        if (&invariant.context() != &domain_repository || !known.contains(invariant.index()))
            throw std::invalid_argument(fmt::format("Invariant order contains an invariant not synthesized for this domain:\n{}", invariant));
        if (!seen.insert(invariant.index()).second)
            throw std::invalid_argument(fmt::format("Invariant order contains an invariant twice:\n{}", invariant));
    }
}

/// The FDR variables' atoms. Synthesis interns into `repository`, the ground task's new layer below the domain's
/// repository: invariants already interned in the domain's repository are found there, others are added to the layer.
urd::Store<urd::sequence<formalism::AtomViewList<GroundTag, formalism::FluentTag>>>
choose_mutex_groups(const formalism::PlanningDomain& planning_domain,
                    formalism::Repository& repository,
                    const formalism::AtomViewList<GroundTag, formalism::FluentTag>& initial_atoms,
                    const formalism::AtomViewList<GroundTag, formalism::FluentTag>& fluent_atoms,
                    const std::optional<formalism::InvariantViewList>& invariant_order)
{
    namespace invariant = formalism::invariant;

    auto groups = urd::Store<urd::sequence<invariant::MutexGroup>> {};
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

    auto result = urd::Store<urd::sequence<formalism::AtomViewList<GroundTag, formalism::FluentTag>>> {};
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

    auto ground_program = GroundTaskProgram(lifted_task.get_formalism_task());
    auto workspace = datalog::ProgramWorkspace<LiftedTag>(ground_program.get_datalog_program());
    auto ctx = datalog::ProgramExecutionContext(workspace);
    datalog::execute_model(ctx, execution_context);

    // The reachable fluent atoms, and the derived ones: the task's derived atoms and the program's applicability and
    // effect atoms.
    auto fluent_bindings = urd::Store<urd::sequence<formalism::PredicateRowView<formalism::FluentTag>>> {};
    const auto& fluent_fact_sets = workspace.facts.fact_sets.get<formalism::FluentTag>();
    for (ygg::uint_t i = 0; i < fluent_fact_sets.predicate.size(); ++i)
        for (const auto binding : datalog::rows(fluent_fact_sets, i))
            fluent_bindings.push_back(binding);
    auto derived_bindings = urd::Store<urd::sequence<formalism::PredicateRowView<formalism::DerivedTag>>> {};
    const auto& derived_fact_sets = workspace.facts.fact_sets.get<formalism::DerivedTag>();
    for (ygg::uint_t i = 0; i < derived_fact_sets.predicate.size(); ++i)
        for (const auto binding : datalog::rows(derived_fact_sets, i))
            derived_bindings.push_back(binding);
    /**
     * Create basic structures of task
     */

    const auto& planning_task = lifted_task.get_formalism_task();
    const auto& planning_domain = planning_task.get_domain();

    auto task = planning_task.get_task();
    auto repository = std::make_shared<formalism::Repository>(planning_domain.get_repository().get());
    auto builder = formalism::Builder();

    auto ground_task = formalism::checkout<formalism::Task<GroundTag>>(builder);

    auto grounding_context = formalism::GroundingContext { builder, *repository };

    ground_task->name = task.get_name();
    urd::store(ground_task->domain, task.get_domain());
    for (const auto predicate : task.get_derived_predicates())
        urd::append(ground_task->derived_predicates, urd::copy(predicate, *repository, builder));
    for (const auto object : task.get_objects())
        urd::append(ground_task->objects, urd::copy(object, *repository, builder));

    auto initial_atoms = formalism::AtomViewList<GroundTag, formalism::FluentTag> {};
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        initial_atoms.push_back(repository->view(urd::copy(atom, *repository, builder)));

    /**
     * Create ground atoms
     */

    /// The derived bindings: those of the task's derived predicates and of the program's own (applicability, effects).
    const auto for_each_predicate_binding = [&](auto&& f)
    {
        for (const auto binding : derived_bindings)
            f(binding);
    };

    auto fluent_atoms = formalism::AtomViewList<GroundTag, formalism::FluentTag> {};
    auto derived_atoms = formalism::AtomViewList<GroundTag, formalism::DerivedTag> {};
    {
        // The program adds no fluent predicates; its derived ones follow the task's (dense indices).
        const auto num_derived_predicates = num_task_derived_predicates(task);
        for (const auto binding : fluent_bindings)
        {
            auto atom = formalism::checkout<formalism::Atom<GroundTag, formalism::FluentTag>>(builder);
            urd::store(atom->row, urd::copy(binding, *repository, builder));
            fluent_atoms.push_back(repository->insert(*atom, builder).first);
        }
        for (const auto binding : derived_bindings)
        {
            if (binding.get_relation().index().value < num_derived_predicates)
            {
                auto atom = formalism::checkout<formalism::Atom<GroundTag, formalism::DerivedTag>>(builder);
                urd::store(atom->row, urd::copy(binding, *repository, builder));
                derived_atoms.push_back(repository->insert(*atom, builder).first);
            }
        }
    }
    std::sort(fluent_atoms.begin(), fluent_atoms.end());
    std::sort(derived_atoms.begin(), derived_atoms.end());

    auto fluent_atoms_set = urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>>(fluent_atoms.begin(), fluent_atoms.end());
    auto derived_atoms_set = urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::DerivedTag>>>(derived_atoms.begin(), derived_atoms.end());

    // std::cout << "Fluent atoms:" << std::endl;
    // std::cout << fluent_atoms << std::endl;
    // std::cout << "Derived atoms:" << std::endl;
    // std::cout << derived_atoms << std::endl;

    auto fdr_context = std::shared_ptr<formalism::FDRContext> { nullptr };

    if (options.disable_invariant_synthesis)
    {
        fdr_context = std::make_shared<formalism::FDRContext>(fluent_atoms, repository);
    }
    else
    {
        auto mutex_groups = choose_mutex_groups(planning_domain, *repository, initial_atoms, fluent_atoms, options.invariant_order);

        fdr_context = std::make_shared<formalism::FDRContext>(mutex_groups, repository);
    }

    auto static_atoms = urd::flat_vector<urd::index<formalism::Atom<GroundTag, formalism::StaticTag>>>();  // the static state's
    for (const auto atom : task.get_atoms<formalism::StaticTag>())
    {
        const auto copied = repository->view(urd::copy(atom, *repository, builder));
        urd::append(ground_task->static_atoms, copied);
        static_atoms.push_back(copied.index());
    }
    for (const auto atom : fluent_atoms)
        urd::append(ground_task->fluent_atoms, urd::copy(atom, *repository, builder));
    for (const auto atom : derived_atoms)
        urd::append(ground_task->derived_atoms, urd::copy(atom, *repository, builder));
    auto static_fterm_values = urd::flat_vector<urd::index<formalism::FunctionTermValue<GroundTag, formalism::StaticTag>>>();
    for (const auto fterm_value : task.get_fterm_values<formalism::StaticTag>())
    {
        const auto copied = repository->view(urd::copy(fterm_value, *repository, builder));
        urd::append(ground_task->static_fterm_values, copied);
        static_fterm_values.push_back(copied.index());
    }
    for (const auto fterm_value : task.get_fterm_values<formalism::FluentTag>())
        urd::append(ground_task->fluent_fterm_values, urd::copy(fterm_value, *repository, builder));
    if (task.get_auxiliary_fterm_value().has_value())
        urd::store(ground_task->auxiliary_fterm_value, urd::copy(task.get_auxiliary_fterm_value().value(), *repository, builder));
    if (task.get_metric())
        urd::store(ground_task->metric, urd::copy(task.get_metric().value(), *repository, builder));
    for (const auto axiom : task.get_axioms())
        urd::append(ground_task->axioms, urd::copy(axiom, *repository, builder));

    /// --- Create FDR variables
    for (const auto variable : fdr_context->get_variables())
        urd::append(ground_task->fluent_variables, variable);

    /// --- Create FDR fluent facts
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        if (const auto fact = std::as_const(*fdr_context).get_fact(repository->view(urd::copy(atom, *repository, builder))))
            urd::build_member(ground_task->fluent_facts.emplace_back(),
                              urd::make<formalism::FDRFact<formalism::FluentTag>>(fact->get_variable(), fact->get_value()),
                              *repository);

    const auto static_state = StaticState(urd::resolve(static_atoms, *repository), urd::resolve(static_fterm_values, *repository));

    /// --- Create FDR goal
    const auto goal_or_nullopt =
        create_ground_fdr_conjunctive_condition(task.get_goal(), fluent_atoms_set, derived_atoms_set, static_state, *fdr_context, grounding_context);
    if (goal_or_nullopt.has_value())
        urd::store(ground_task->goal, *goal_or_nullopt);
    else
        return GroundTaskInstantiationResult { nullptr, GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE };

    /// --- Create FDR actions and axioms

    auto fluent_assign = urd::Store<urd::map<urd::index<formalism::FDRVariable<formalism::FluentTag>>, formalism::FDRValue>> {};
    auto derived_assign = urd::Store<urd::map<urd::index<formalism::Atom<GroundTag, formalism::DerivedTag>>, bool>> {};

    // The atoms of the effect predicates are the relaxed reachable groundings of the conditional effects.
    auto effect_atom_index = EffectAtomIndex(ground_program.get_predicate_to_action_mapping(),
                                             ground_program.get_conditional_effect_to_predicate_mapping(),
                                             workspace.facts.fact_sets.get<formalism::DerivedTag>(),
                                             *workspace.workspace_repository);

    for_each_predicate_binding(
        [&](const auto binding)
        {
            const auto& mapping = ground_program.get_predicate_to_action_mapping();

            if (const auto it = mapping.find(binding.get_relation()); it != mapping.end())
            {
                const auto action = it->second;

                workspace.binding.clear();
                for (const auto object : binding.template get<urd::index<formalism::Object>>())
                    workspace.binding.push_back(object);

                auto grounder_context = formalism::GroundingContext { workspace.planning_builder, *repository, workspace.binding };

                const auto ground_action_or_nullopt = ground_pruned(
                    action,
                    fluent_atoms_set,
                    derived_atoms_set,
                    [&](auto effect, auto&& apply) { effect_atom_index.for_each_effect_binding(action, effect, grounder_context.binding(), apply); },
                    static_state,
                    grounder_context,
                    *fdr_context);

                if (!ground_action_or_nullopt.has_value())
                    return;

                const auto& ground_action = ground_action_or_nullopt.value();

                assert(is_statically_applicable(ground_action, static_state));

                if (is_consistent(ground_action, fluent_assign, derived_assign))
                {
                    urd::append(ground_task->ground_actions, ground_action);
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
                for (const auto object : binding.template get<urd::index<formalism::Object>>())
                    workspace.binding.push_back(object);

                auto grounder_context = formalism::GroundingContext { workspace.planning_builder, *repository, workspace.binding };

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
                        urd::append(ground_task->ground_axioms, ground_axiom);
                    }
                }
            }
        });

    return GroundTaskInstantiationResult { std::make_shared<Task<GroundTag>>(formalism::PlanningTask<GroundTag>(repository->insert(*ground_task, builder).first,
                                                                                                                std::move(fdr_context),
                                                                                                                repository,
                                                                                                                planning_task.get_domain(),
                                                                                                                planning_task.get_path())),
                                           GroundTaskInstantiationStatus::SUCCESS };
}
}
