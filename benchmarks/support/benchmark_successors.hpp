#pragma once

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/successors/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

// The successors the benchmarks time, composed from a generator and tyr/planning/successors.hpp as a caller does.
namespace tyr::benchmarks
{

/// The registered successor of `node` by `binding`.
template<TaskKind Kind, typename Binding>
planning::NodeBuilder<Kind> registered_successor(planning::SuccessorGenerator<Kind>& generator,
                                                 const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                                 Binding binding,
                                                 planning::StateRepository<Kind>& repository,
                                                 planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                                 planning::StateDelta& delta)
{
    // The caller's delta lends its buffers to the successor state for the three steps.
    auto successor = planning::SuccessorState<Kind> {};
    std::swap(successor.delta, delta);
    successor.state = ::tyr::planning::get_state_builder(repository);
    successor.auxiliary_value = generator.generate_successor_state(node, binding, *successor.state, successor.delta);
    planning::find_successor(repository, node, successor);
    planning::extend_successor(axiom_evaluator, node, successor);
    auto result = planning::register_successor(repository, node, successor);
    std::swap(successor.delta, delta);
    return result;
}

template<TaskKind Kind, typename Binding>
planning::NodeBuilder<Kind> registered_successor(planning::SuccessorGenerator<Kind>& generator,
                                                 const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                                 Binding binding,
                                                 planning::StateRepository<Kind>& repository,
                                                 planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto delta = planning::StateDelta {};
    return registered_successor(generator, node, binding, repository, axiom_evaluator, delta);
}

/// The registered successors of the applicable bindings `filter` reports, each labeled by its binding, interned.
template<TaskKind Kind>
void labeled_successors(planning::SuccessorGenerator<Kind>& generator,
                        formalism::Builder& builder,
                        const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                        const planning::ActionBindingFilter<Kind>& filter,
                        planning::StateRepository<Kind>& repository,
                        planning::AxiomEvaluator<Kind>& axiom_evaluator,
                        planning::LabeledNodeList<Kind>& out_successors)
{
    out_successors.clear();
    auto delta = planning::StateDelta {};
    generator.for_each_applicable_action_binding(
        node,
        filter,
        [&](planning::ActionBindingView<Kind> borrowed)
        {
            const auto binding = planning::intern_action_binding(generator, builder, borrowed);
            out_successors.push_back(
                planning::LabeledNodeBuilder<Kind> { binding.index(), registered_successor(generator, node, binding, repository, axiom_evaluator, delta) });
            return true;
        });
}

/// The applicable bindings `filter` reports, interned (through scratch rows of `builder`).
template<TaskKind Kind>
void interned_bindings(planning::SuccessorGenerator<Kind>& generator,
                       formalism::Builder& builder,
                       const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                       const planning::ActionBindingFilter<Kind>& filter,
                       urd::Store<urd::sequence<formalism::ActionRowView>>& out_bindings)
{
    out_bindings.clear();
    generator.for_each_applicable_action_binding(node,
                                                 filter,
                                                 [&](planning::ActionBindingView<Kind> binding)
                                                 {
                                                     out_bindings.push_back(planning::intern_action_binding(generator, builder, binding));
                                                     return true;
                                                 });
}
}
