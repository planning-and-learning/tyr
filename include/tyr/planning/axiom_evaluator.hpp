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

#ifndef TYR_PLANNING_AXIOM_EVALUATOR_HPP_
#define TYR_PLANNING_AXIOM_EVALUATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class AxiomEvaluator
{
    friend class AxiomEvaluatorFactory<Kind>;

private:
    struct Impl;

    AxiomEvaluator(ygg::uint_t index, TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit AxiomEvaluator(std::unique_ptr<Impl> impl);

public:
    ~AxiomEvaluator();

    AxiomEvaluator(const AxiomEvaluator&) = delete;
    AxiomEvaluator& operator=(const AxiomEvaluator&) = delete;
    AxiomEvaluator(AxiomEvaluator&&) = delete;
    AxiomEvaluator& operator=(AxiomEvaluator&&) = delete;

    void compute_extended_state(urd::builder<State<Kind>>& state_builder);
    [[nodiscard]] AxiomEvaluatorPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const;

    [[nodiscard]] const TaskPtr<Kind>& get_task() const noexcept;
    [[nodiscard]] const ygg::ExecutionContextPtr& get_execution_context() const noexcept;
    [[nodiscard]] ygg::uint_t get_index() const noexcept;

    /// Lifted axioms are evaluated by a Datalog program, whose statistics this prints.
    void print_summary(size_t verbosity) const
        requires std::same_as<Kind, LiftedTag>;

private:
    std::unique_ptr<Impl> m_impl;
};

// Each kind defines compute_extended_state in its own source file (src/planning/{ground,lifted}/axiom_evaluator.cpp).
template<>
void AxiomEvaluator<GroundTag>::compute_extended_state(urd::builder<State<GroundTag>>& state_builder);
template<>
void AxiomEvaluator<LiftedTag>::compute_extended_state(urd::builder<State<LiftedTag>>& state_builder);
template<>
void AxiomEvaluator<LiftedTag>::print_summary(size_t verbosity) const;

extern template class AxiomEvaluator<GroundTag>;
extern template class AxiomEvaluator<LiftedTag>;

}

#endif
