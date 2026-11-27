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

#include "tyr/planning/successors/ground/successor_generator.hpp"

#include "../../metric.hpp"
#include "../outcome_distributions.hpp"
#include "../temporal_transitions.hpp"
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
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/successors/action_executor.hpp"
#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/ground/match_tree/match_tree.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/tasks/task_utils.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
using Source = NodeBuilderView<GroundTag>;
using Builder = urd::builder<State<GroundTag>, StateRepository<GroundTag>>;
using Bindings = urd::Store<urd::sequence<formalism::ActionRowView>>;

void validate_task(const TaskPtr<GroundTag>& task, const StateRepository<GroundTag>& state_repository)
{
    if (state_repository.get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: state repository belongs to a different task.");
}

void validate_task(const TaskPtr<GroundTag>& task, const AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    if (axiom_evaluator.get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: axiom evaluator belongs to a different task.");
}

void validate_task(const TaskPtr<GroundTag>& task, const Source& node)
{
    if (node.context().get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
}

}

struct SuccessorGenerator<GroundTag>::Impl
{
    using ActionBindingMap = urd::Store<urd::map<formalism::ActionRowView, formalism::ActionView<GroundTag>>>;
    using ActionMatchTrees = urd::Store<urd::map<formalism::ActionView<LiftedTag>, match_tree::MatchTreePtr<formalism::Action<GroundTag>>>>;
    using MatchTree = match_tree::MatchTree<formalism::Action<GroundTag>>;

    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task);

        TaskPtr<GroundTag> task;
        match_tree::MatchTreePtr<formalism::Action<GroundTag>> action_match_tree_prototype;
        ActionMatchTrees schema_match_tree_prototypes;
        ActionBindingMap action_binding_to_ground_action;
        /// Whether transitions keep a temporal network (is_temporal).
        bool temporal;
    };

    struct Evaluator
    {
        explicit Evaluator(const Definition& definition);

        match_tree::MatchTreePtr<formalism::Action<GroundTag>> action_match_tree;
        ActionMatchTrees schema_match_trees;
        formalism::ActionViewList<GroundTag> applicable_actions;
        ActionExecutor executor;
        StateDelta successor_delta;
        urd::data<ygg::Row<formalism::Action<LiftedTag>>> checked_binding;
        GroundOutcomeDistributions outcome_distributions;
        Happening happening;
        TemporalStepper stepper;
        bool scheduled = true;
    };

    explicit Impl(TaskPtr<GroundTag> task) : definition(std::make_shared<Definition>(std::move(task))), evaluator(*definition) {}

    explicit Impl(std::shared_ptr<const Definition> definition_) : definition(std::move(definition_)), evaluator(*this->definition) {}

    MatchTree& get_schema_match_tree(formalism::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_match_trees.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_match_trees.end() || &action.context() != &it->first.context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return *it->second;
    }

    /// The match tree of the action schema, or of all actions.
    MatchTree& get_match_tree(std::optional<formalism::ActionView<LiftedTag>> action)
    { return action ? get_schema_match_tree(*action) : *evaluator.action_match_tree; }

    template<typename Objects>
    ActionBindingResult try_get_applicable_action_binding(const Source& node, formalism::ActionView<LiftedTag> action, Objects objects);

    template<typename Callback>
    bool for_each_applicable_action(const Source& node, MatchTree& tree, Callback&& callback)
    {
        return for_each_applicable_action(node, tree, [](auto) { return true; }, std::forward<Callback>(callback), [] { return false; });
    }

    template<typename Accept, typename Callback, typename Stop>
    bool for_each_applicable_action(const Source& node, MatchTree& tree, Accept accept, Callback callback, Stop stop)
    {
        if (stop())
            return false;
        const auto state_context = state_context_of(node);
        tree.generate(state_context, evaluator.applicable_actions);
        for (const auto action : evaluator.applicable_actions)
        {
            if (stop())
                return false;
            if (!accept(action))
                continue;
            assert(is_applicable(action.get_condition(), state_context));
            if (!evaluator.executor.is_applicable_if_fires(action, state_context))
                continue;
            assert(evaluator.executor.is_applicable(action, state_context));
            if (!callback(action))
                return false;
        }
        return true;
    }

    /// Validates the arguments of generating successors of `node` into `storage`, and gives the match tree of the action
    /// schema, or of all actions.
    template<typename Storage>
    MatchTree& prepare(const Source& node,
                       std::optional<formalism::ActionView<LiftedTag>> action,
                       const Storage& storage,
                       const AxiomEvaluator<GroundTag>& axiom_evaluator)
    {
        validate(node, storage, axiom_evaluator);
        return get_match_tree(action);
    }

    template<typename Storage>
    void validate(const Source& node, const Storage& storage, const AxiomEvaluator<GroundTag>& axiom_evaluator) const
    {
        validate_task(definition->task, node);
        validate_storage(definition->task, node, storage);
        validate_task(definition->task, axiom_evaluator);
    }

    /// The ground action a binding names (an interned row: its ground action; a row being built: the ground action of its
    /// interned row, invalid_argument if there is none).
    template<formalism::ActionRowViewConcept Binding>
    formalism::ActionView<GroundTag> ground_action_of(Binding binding)
    {
        if constexpr (std::same_as<Binding, formalism::ActionRowView>)
        {
            const auto found = definition->action_binding_to_ground_action.find(binding);
            if (found == definition->action_binding_to_ground_action.end())
                throw std::invalid_argument("SuccessorGenerator: the binding is no ground action of the task.");
            return found->second;
        }
        else
        {
            // A row being built names a ground action of the task by its values: its interned row's.
            auto& scratch = evaluator.checked_binding;
            scratch.relation = action_of(binding).index();
            scratch.values.clear();
            for (const auto value : binding.get_values())
                scratch.values.push_back(value);
            const auto row = definition->task->get_repository()->find(scratch);
            if (!row)
                throw std::invalid_argument("SuccessorGenerator: the binding is no ground action of the task.");
            return ground_action_of(*row);
        }
    }

    /// The core: writes the successor of `node` by `action` into `out_state`, without axiom closure. An action with several
    /// outcomes or happenings has no single successor state.
    ygg::float_t generate(const Source& node, formalism::ActionView<GroundTag> action, Builder& out_state, StateDelta& out_delta)
    {
        if (action.get_temporal_part().has_value())
            throw std::invalid_argument("SuccessorGenerator: a durative action happens at its start and its end, within a temporal network: use "
                                        "generate_successor or for_each_successor.");
        if (!action.get_choices().empty())
            throw std::invalid_argument(
                "SuccessorGenerator: an action with oneof or probabilistic effects has several outcomes: use generate_successor or for_each_successor.");
        const auto state_context = state_context_of(node);
        return evaluator.executor.apply_action_unregistered(state_context, action, out_state, out_delta);
    }

    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<GroundTag>::Impl::Definition::Definition(TaskPtr<GroundTag> task_) :
    task(std::move(task_)),
    action_match_tree_prototype(match_tree::MatchTree<formalism::Action<GroundTag>>::create(
        formalism::ActionViewList<GroundTag>(task->get_task().get_ground_actions().begin(), task->get_task().get_ground_actions().end()),
        task->get_repository())),
    action_binding_to_ground_action(),
    temporal(is_temporal(*task))
{
    auto schema_actions = urd::Store<urd::map<formalism::ActionView<LiftedTag>, formalism::ActionViewList<GroundTag>>> {};
    for (const auto action : task->get_task().get_domain().get_actions())
        schema_actions.try_emplace(action);
    for (const auto action : task->get_task().get_ground_actions())
    {
        action_binding_to_ground_action.emplace(action.get_row(), action);
        schema_actions.at(action.get_action()).push_back(action);
    }
    for (auto& [schema, actions] : schema_actions)
        schema_match_tree_prototypes.emplace(schema, match_tree::MatchTree<formalism::Action<GroundTag>>::create(std::move(actions), task->get_repository()));
}

SuccessorGenerator<GroundTag>::Impl::Evaluator::Evaluator(const Definition& definition) :
    action_match_tree(definition.action_match_tree_prototype->make_worker()),
    applicable_actions(),
    executor()
{
    for (const auto& [schema, tree] : definition.schema_match_tree_prototypes)
        schema_match_trees.emplace(schema, tree->make_worker());
}

SuccessorGenerator<GroundTag>::SuccessorGenerator(TaskPtr<GroundTag> task, const ygg::ExecutionContextPtr& execution_context) :
    m_impl(std::make_unique<Impl>(std::move(task)))
{ static_cast<void>(execution_context); }

SuccessorGenerator<GroundTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<GroundTag>::~SuccessorGenerator() = default;
SuccessorGenerator<GroundTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<GroundTag>& SuccessorGenerator<GroundTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<GroundTag> SuccessorGenerator<GroundTag>::make_worker(const ygg::ExecutionContextPtr& execution_context) const
{
    static_cast<void>(execution_context);
    return SuccessorGeneratorPtr<GroundTag>(new SuccessorGenerator<GroundTag>(std::make_unique<Impl>(m_impl->definition)));
}

template<typename Objects>
ActionBindingResult
SuccessorGenerator<GroundTag>::Impl::try_get_applicable_action_binding(const Source& node, formalism::ActionView<LiftedTag> action, Objects objects)
{
    const auto& task = definition->task;
    validate_task(task, node);
    get_schema_match_tree(action);
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object count does not match action arity.");

    const auto indices = objects.base();
    const auto& source_repository = objects.context();
    const auto& target_repository = *task->get_repository();
    if (!std::ranges::all_of(indices, [&](const auto& value) { return source_repository.contains(value); })
        || !std::ranges::all_of(indices, [&](const auto& value) { return target_repository.contains(value); }))
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    if (!std::ranges::all_of(objects, [&](const auto& value) { return target_repository.contains(value); }))
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object does not belong to the task repository.");

    const auto constants = task->get_task().get_domain().get_constants();
    const auto task_objects = task->get_task().get_objects();
    const auto by_index = [](const auto view) { return view.index(); };
    for (const auto object : indices)
        if (!std::ranges::binary_search(constants, object, {}, by_index) && !std::ranges::binary_search(task_objects, object, {}, by_index))
            return { ActionBindingStatus::INAPPLICABLE, std::nullopt };

    auto& scratch = evaluator.checked_binding;
    scratch.relation = action.index();
    scratch.values.clear();
    for (const auto object : indices)
        scratch.values.push_back(object.value);
    const auto binding = task->get_repository()->find(scratch);
    if (!binding)
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    const auto found = definition->action_binding_to_ground_action.find(*binding);
    if (found == definition->action_binding_to_ground_action.end())
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    const auto state = state_context_of(node);
    if (!evaluator.executor.is_applicable(found->second, state))
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    return { ActionBindingStatus::APPLICABLE, binding };
}

template<formalism::ActionRowViewConcept Binding>
ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Source& node, Binding binding)
{ return m_impl->try_get_applicable_action_binding(node, action_of(binding), binding.template resolve<formalism::Object>()).status; }

void SuccessorGenerator<GroundTag>::validate_action_binding(ActionBindingView<GroundTag> binding)
{
    const auto& repository = *m_impl->definition->task->get_repository();
    if (!repository.contains(binding))
        throw std::invalid_argument("SuccessorGenerator: action binding does not belong to the task repository.");
    // Bindings in the grounded task already have validated schemas and objects.
    if (m_impl->definition->action_binding_to_ground_action.contains(binding))
        return;
    const auto action = binding.get_relation();
    m_impl->get_schema_match_tree(action);
    const auto objects = binding.template resolve<formalism::Object>();
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator: object count does not match action arity.");
    if (!std::ranges::all_of(objects, [&](const auto& value) { return repository.contains(value); }))
        throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
}

bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Source& node,
                                                                       const ActionBindingFilter<GroundTag>& filter,
                                                                       const std::function<bool(ActionBindingView<GroundTag>)>& callback)
{
    validate_task(m_impl->definition->task, node);
    auto& tree = filter.action.has_value() ? m_impl->get_schema_match_tree(*filter.action) : *m_impl->evaluator.action_match_tree;
    return m_impl->for_each_applicable_action(
        node,
        tree,
        [&](const auto candidate) { return !filter.accept || filter.accept(candidate.get_row()); },
        [&](const auto candidate) { return callback(candidate.get_row()); },
        [&] { return filter.stop && filter.stop(); });
}

template<formalism::ActionRowViewConcept Binding>
ygg::float_t SuccessorGenerator<GroundTag>::generate_successor_state(const Source& node, Binding binding, Builder& out_state, StateDelta& out_delta)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->generate(node, m_impl->ground_action_of(binding), out_state, out_delta);
}

formalism::ActionView<GroundTag> SuccessorGenerator<GroundTag>::ground_action(formalism::ActionRowView binding) const
{
    const auto it = m_impl->definition->action_binding_to_ground_action.find(binding);
    assert(it != m_impl->definition->action_binding_to_ground_action.end() && "Ground action binding not found.");
    return it->second;
}

const TaskPtr<GroundTag>& SuccessorGenerator<GroundTag>::get_task() const noexcept { return m_impl->definition->task; }

namespace
{
using GroundStateBuilder = urd::builder<State<GroundTag>, StateRepository<GroundTag>>;

/// The reads of the choices (their conditions and their outcomes' effects' conditions, nested ones too).
template<typename Choices>
void add_choice_reads(Choices choices, Happening& happening)
{
    for (const auto choice : choices)
    {
        add_reads(choice.get_condition(), happening.reads, happening.numeric_reads);
        for (const auto outcome : choice.get_outcomes())
        {
            for (const auto effect : outcome.get_effects())
                add_reads(effect.get_condition(), happening.reads, happening.numeric_reads);
            add_choice_reads(outcome.get_choices(), happening);
        }
    }
}

/// The reads of effects (their conditions: evaluated, whether they fire or not).
template<typename Effects>
void add_effect_reads(Effects effects, Happening& happening)
{
    for (const auto effect : effects)
        add_reads(effect.get_condition(), happening.reads, happening.numeric_reads);
}

/// Whether every open durative action's over-all condition holds in `state` (an outcome's).
template<typename Generator>
bool over_all_holds(Generator& generator, const StateBuilderView<GroundTag>& state)
{
    const auto& task = *state.context().get_task();
    const auto context = StateContext<GroundTag>(task, state, ygg::float_t { 0 });
    return std::ranges::all_of(context.temporal_storage().get_parts().open,
                               [&](const OpenAction& open)
                               {
                                   const auto action = generator.ground_action(urd::resolve(open.action, *task.get_repository()));
                                   return is_applicable(running_temporal_part(action).get_over_all(), context);
                               });
}

/// Writes into `out` the successor of `node` by the happening labeled `label`: each joint outcome of `set` applies `effects`
/// and its picks, then adds the happening to the temporal parts (`happening`'s reads given, its writes the effects fired),
/// its outcome filled by `make(apply, outcome, valid)`; failed where the temporal network becomes inconsistent or an open
/// over-all condition does not hold. Under (minimize (total-time)), the metric is the makespan.
template<typename Generator, typename Impl, typename Make>
void ground_transition(Generator& generator,
                       Impl& impl,
                       const NodeBuilderView<GroundTag>& node,
                       const urd::data<Transition>& label,
                       std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                       OutcomeRange set,
                       const TemporalStepper::Kind& temporal,
                       Make make,
                       SuccessorBuilder<GroundTag>& out)
{
    auto& evaluator = impl.evaluator;
    const auto state_context = state_context_of(node);
    const auto time_metric = minimizes_total_time(*node.context().get_task());
    generate_successor<GroundTag>(
        out,
        node.context(),
        label,
        evaluator.outcome_distributions,
        set,
        [&](std::span<const formalism::OutcomeView<GroundTag>> picks) { return evaluator.executor.is_applicable_if_fires(effects, picks, state_context); },
        [&](std::span<const formalism::OutcomeView<GroundTag>> picks, auto& outcome)
        {
            return make(
                [&](GroundStateBuilder& state)
                {
                    const auto auxiliary_value = evaluator.executor.apply_effects_unregistered(state_context, effects, picks, state, evaluator.successor_delta);
                    auto& happening = evaluator.happening;
                    happening.writes.clear();
                    happening.numeric_writes.clear();
                    evaluator.executor.fired_writes(happening.writes, happening.numeric_writes);
                    auto makespan = time_metric ? node.get_metric() : ygg::float_t { 0 };
                    evaluator.scheduled = !impl.definition->temporal || evaluator.stepper.add(state.temporal_storage->parts, happening, temporal, makespan);
                    return time_metric ? std::max(auxiliary_value, makespan) : auxiliary_value;
                },
                outcome,
                [&](const auto& successor_state) { return evaluator.scheduled && over_all_holds(generator, successor_state); });
        });
}

/// The successor of applying the instantaneous `action`, or of starting the durative `action` (none if its duration is not
/// a nonnegative number).
template<typename Generator, typename Impl, typename Make>
void action_transition(Generator& generator,
                       Impl& impl,
                       const NodeBuilderView<GroundTag>& node,
                       formalism::ActionView<GroundTag> action,
                       Make make,
                       SuccessorBuilder<GroundTag>& out)
{
    const auto state_context = state_context_of(node);
    auto& distributions = impl.evaluator.outcome_distributions;
    auto& happening = impl.evaluator.happening;
    distributions.clear();
    happening.clear();
    add_reads(action.get_condition(), happening.reads, happening.numeric_reads);
    add_effect_reads(action.get_effects(), happening);
    add_choice_reads(action.get_choices(), happening);
    const auto effects = std::array { action.get_effects() };
    const auto row = action.get_row();
    auto temporal = TemporalStepper::Kind {};
    auto label = urd::data<Transition>(TransitionKind::APPLY, urd::flat_optional<ActionRowKey>(row.index()), 0);

    if (const auto temporal_part = action.get_temporal_part(); temporal_part.has_value())
    {
        label.kind = TransitionKind::START;
        const auto [min_duration, max_duration] = duration_bounds(temporal_part->get_min_durations(), temporal_part->get_max_durations(), state_context);
        if (!(min_duration <= max_duration))
        {
            start_successor(out, node.context(), label);
            return;
        }
        temporal = TemporalStepper::Kind { TransitionKind::START, 0, row.index(), min_duration, max_duration, 0 };
        add_reads(temporal_part->get_over_all(), happening.over_all, happening.numeric_over_all);
    }
    const auto set = build_ground_choices(distributions, action.get_choices(), state_context);
    ground_transition(generator, impl, node, label, effects, set, temporal, make, out);
}

/// The successor of ending the open durative action at position `open` (none if its end condition does not hold): its end
/// effects and choices apply, and it reads its over-all condition up to here.
template<typename Generator, typename Impl, typename Make>
void end_transition(Generator& generator, Impl& impl, const NodeBuilderView<GroundTag>& node, std::uint32_t open, Make make, SuccessorBuilder<GroundTag>& out)
{
    const auto state_context = state_context_of(node);
    const auto& task = *node.context().get_task();
    const auto row = urd::resolve(state_context.temporal_storage().get_parts().open[open].action, *task.get_repository());
    const auto label = urd::data<Transition>(TransitionKind::END, urd::flat_optional<ActionRowKey>(row.index()), open);
    const auto temporal_part = running_temporal_part(generator.ground_action(row));
    if (!is_applicable(temporal_part.get_end_condition(), state_context))
    {
        start_successor(out, node.context(), label);
        return;
    }

    auto& distributions = impl.evaluator.outcome_distributions;
    auto& happening = impl.evaluator.happening;
    distributions.clear();
    happening.clear();
    add_reads(temporal_part.get_end_condition(), happening.reads, happening.numeric_reads);
    add_reads(temporal_part.get_over_all(), happening.reads, happening.numeric_reads);
    add_effect_reads(temporal_part.get_end_effects(), happening);
    add_choice_reads(temporal_part.get_end_choices(), happening);
    const auto effects = std::array { temporal_part.get_end_effects() };
    const auto set = build_ground_choices(distributions, temporal_part.get_end_choices(), state_context);
    const auto [end_min, end_max] = duration_bounds(temporal_part.get_end_min_durations(), temporal_part.get_end_max_durations(), state_context);
    if (!(end_min <= end_max))
    {
        start_successor(out, node.context(), label);
        return;
    }
    const auto temporal = TemporalStepper::Kind { TransitionKind::END, open, {}, 0, 0, 0, 0, end_min, end_max };
    ground_transition(generator, impl, node, label, effects, set, temporal, make, out);
}

/// Fills an outcome by writing the applied state into a state builder of `node`'s repository, closed under the axioms.
inline auto building(const NodeBuilderView<GroundTag>& node, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    return [&node, &axiom_evaluator](auto apply, auto& outcome, auto valid) { return build_outcome(node, axiom_evaluator, outcome, apply, valid); };
}
}

template<formalism::ActionRowViewConcept Binding>
void SuccessorGenerator<GroundTag>::generate_successor(const Source& node, Binding binding, AxiomEvaluator<GroundTag>& axiom_evaluator, SuccessorBuilder<GroundTag>& out)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    action_transition(*this, *m_impl, node, m_impl->ground_action_of(binding), building(node, axiom_evaluator), out);
}

void SuccessorGenerator<GroundTag>::generate_end_successor(const Source& node,
                                                           std::uint32_t open,
                                                           AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                           SuccessorBuilder<GroundTag>& out)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    end_transition(*this, *m_impl, node, open, building(node, axiom_evaluator), out);
}

void SuccessorGenerator<GroundTag>::generate_timed_literal_successor(const Source& node,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                     SuccessorBuilder<GroundTag>& out)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    timed_literal_transition(
        m_impl->evaluator,
        node,
        building(node, axiom_evaluator),
        [&](const auto& state) { return over_all_holds(*this, state); },
        out);
}

bool SuccessorGenerator<GroundTag>::for_each_successor(const Source& node,
                                                       AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                       const std::function<bool(SuccessorBuilder<GroundTag>&)>& callback)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    // One of the node's repository's builders, reused across its successors: back to its pool when this returns.
    auto checked_out = ygg::formalism::checkout<Successor<GroundTag>>(node.context().builders());
    auto& successor = *checked_out;
    const auto exhausted = m_impl->for_each_applicable_action(node,
                                                              *m_impl->evaluator.action_match_tree,
                                                              [&](const auto action)
                                                              {
                                                                  action_transition(*this, *m_impl, node, action, building(node, axiom_evaluator), successor);
                                                                  return successor.distributions.empty() || callback(successor);
                                                              });
    if (!exhausted)
        return false;
    const auto open_count = static_cast<std::uint32_t>(state_context_of(node).temporal_storage().get_parts().open.size());
    for (std::uint32_t open = 0; open < open_count; ++open)
    {
        generate_end_successor(node, open, axiom_evaluator, successor);
        if (!successor.distributions.empty() && !callback(successor))
            return false;
    }
    generate_timed_literal_successor(node, axiom_evaluator, successor);
    return successor.distributions.empty() || callback(successor);
}

template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Source& node, formalism::ActionRowView binding);
template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Source& node, formalism::ActionRowBuilderView binding);
template ygg::float_t
SuccessorGenerator<GroundTag>::generate_successor_state(const Source& node, formalism::ActionRowView binding, Builder& out_state, StateDelta& out_delta);
template ygg::float_t
SuccessorGenerator<GroundTag>::generate_successor_state(const Source& node, formalism::ActionRowBuilderView binding, Builder& out_state, StateDelta& out_delta);

template void SuccessorGenerator<GroundTag>::generate_successor(const Source& node,
                                                             formalism::ActionRowView binding,
                                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                             SuccessorBuilder<GroundTag>& out);
template void SuccessorGenerator<GroundTag>::generate_successor(const Source& node,
                                                             formalism::ActionRowBuilderView binding,
                                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                             SuccessorBuilder<GroundTag>& out);

}
