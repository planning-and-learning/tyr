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

#ifndef TYR_PLANNING_LIFTED_AXIOM_EVALUATOR_HPP_
#define TYR_PLANNING_LIFTED_AXIOM_EVALUATOR_HPP_

#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/programs/axiom.hpp"

#include <atomic>
#include <memory>

namespace tyr::planning
{

template<>
class AxiomEvaluator<LiftedTag>
{
    friend class AxiomEvaluatorFactory<LiftedTag>;

private:
    struct Impl;

    AxiomEvaluator(ygg::uint_t index,
                   TaskPtr<LiftedTag> task,
                   ygg::ExecutionContextPtr execution_context,
                   std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit AxiomEvaluator(std::unique_ptr<Impl> impl);

public:
    ~AxiomEvaluator();

    void compute_extended_state(ygg::Builder<State<LiftedTag>>& state_builder);
    [[nodiscard]] AxiomEvaluatorPtr<LiftedTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    const AxiomEvaluatorProgram<LiftedTag>& get_axiom_program() const noexcept;
    const ygg::ExecutionContextPtr& get_execution_context() const noexcept;
    ygg::uint_t get_index() const noexcept;

    void print_summary(size_t verbosity) const;

private:
    std::unique_ptr<Impl> m_impl;
};

}

#endif
