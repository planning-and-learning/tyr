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

#ifndef TYR_PLANNING_SUCCESSORS_LIFTED_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_SUCCESSORS_LIFTED_SUCCESSOR_GENERATOR_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/successor_builder.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/successor_generator.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::planning
{

class ApplicableActionProgram;

template<>
class SuccessorGenerator<LiftedTag>
{
private:
    struct Impl;
    using Source = NodeBuilderView<LiftedTag>;
    using Builder = urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>;
    using BindingData = urd::builder<ygg::Row<formalism::Action<LiftedTag>>, formalism::Repository>;

    explicit SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept;

public:
    SuccessorGenerator(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context);
    ~SuccessorGenerator();

    SuccessorGenerator(const SuccessorGenerator&) = delete;
    SuccessorGenerator& operator=(const SuccessorGenerator&) = delete;
    SuccessorGenerator(SuccessorGenerator&&) noexcept;
    SuccessorGenerator& operator=(SuccessorGenerator&&) noexcept;

    /// Checks full applicability without interning the offered binding. The borrowed object row must resolve to this task's canonical owners.
    /// Foreign states, schemas or objects and wrong arity throw invalid_argument. May be called from a binding callback for the
    /// callback's state; for another state it throws logic_error.
    /// Ground tuples absent from the compiled action set are INAPPLICABLE; lifted domain failures are OUTSIDE_PARAMETER_DOMAIN.
    /// Accepts repository-backed and data-backed action bindings without publishing them.
    template<formalism::ActionRowViewConcept Binding>
    ActionBindingStatus check_action_binding(const Source& node, Binding binding);

    /// Validates a borrowed binding's schema, object ownership and arity (invalid_argument otherwise), without checking
    /// applicability: what interning it takes (intern_action_binding, tyr/planning/successors.hpp).
    void validate_action_binding(ActionBindingView<LiftedTag> binding);

    // The generator never interns: it enumerates applicable bindings borrowed, and writes successor states into builders,
    // neither closed under the axioms nor registered. Successors as nodes, into builders or registered, and interned
    // bindings: tyr/planning/successors.hpp.

    formalism::ActionView<GroundTag> ground_action(formalism::ActionRowView binding);

    /// Calls `callback` with each applicable binding `filter` reports (ActionBindingFilter), borrowed: a lifted binding and
    /// its object row are valid only during the callback (a ground one is its ground action's row). Callbacks return true to
    /// continue, false to stop; the result is true iff enumeration was exhausted. Enumeration must not reenter this
    /// generator's enumeration: they share scratch storage, and reentering throws logic_error. Checking, validating,
    /// interning and generating a successor of the borrowed binding (for the callback's state) are supported inside.
    bool for_each_applicable_action_binding(const Source& node,
                                            const ActionBindingFilter<LiftedTag>& filter,
                                            const std::function<bool(ActionBindingView<LiftedTag>)>& callback);

    /// Writes the successor of `node` by `binding` (an interned or a borrowed row of this task: one this generator handed
    /// out, or one validate_action_binding accepted) into `out_state`, neither closed under the axioms nor registered, and
    /// its exact changes relative to `node` into `out_delta`; returns its auxiliary value. Nodes from it, into builders or
    /// registered: tyr/planning/successors.hpp.
    template<formalism::ActionRowViewConcept Binding>
    ygg::float_t generate_successor_state(const Source& node, Binding binding, Builder& out_state, StateDelta& out_delta);

    // Lookup
    // Successors with outcomes, over happenings: applying an instantaneous action binding, starting a durative one, ending an
    // open one, or the next timed initial literals; their times stay flexible within the state's temporal network; no action
    // is grounded. An action with oneof or probabilistic effects has several outcomes, a deterministic one a distribution of
    // one. Each is written into `out` (its earlier contents go): its label's binding as given (an interned row as its key, a
    // borrowed one copied), its outcome states into state builders of `node`'s repository, closed under the axioms, not
    // registered (register_outcomes, tyr/planning/successors/successors.hpp). A transition whose effects conflict in some
    // joint outcome, or whose outcomes all fail, has no distributions (it is not applicable).

    template<formalism::ActionRowViewConcept Binding>
    void generate_successor(const Source& node, Binding binding, AxiomEvaluator<LiftedTag>& axiom_evaluator, SuccessorBuilder<LiftedTag>& out);

    /// The successor of ending the open durative action at position `open` among the node's (no distributions if its end
    /// condition does not hold, or it cannot be scheduled).
    void generate_end_successor(const Source& node, std::uint32_t open, AxiomEvaluator<LiftedTag>& axiom_evaluator, SuccessorBuilder<LiftedTag>& out);

    /// The successor of the next timed initial literals (no distributions once all happened, or if they cannot be scheduled).
    void generate_timed_literal_successor(const Source& node, AxiomEvaluator<LiftedTag>& axiom_evaluator, SuccessorBuilder<LiftedTag>& out);

    /// Calls `callback` with the successor of each applicable happening (the action bindings', the open actions' ends, then
    /// the next timed initial literals), written into a builder of this generator, valid during the callback (registering its
    /// outcomes there is supported). Callbacks return true to continue, false to stop; the result is true iff enumeration was
    /// exhausted. It must not reenter this generator's enumeration.
    bool for_each_successor(const Source& node, AxiomEvaluator<LiftedTag>& axiom_evaluator, const std::function<bool(SuccessorBuilder<LiftedTag>&)>& callback);

    [[nodiscard]] SuccessorGeneratorPtr<LiftedTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    // Diagnostics
    void print_summary(size_t verbosity) const;

    // Getters
    [[nodiscard]] const ApplicableActionProgram& get_action_program() const noexcept;
    [[nodiscard]] const TaskPtr<LiftedTag>& get_task() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

}

#endif
