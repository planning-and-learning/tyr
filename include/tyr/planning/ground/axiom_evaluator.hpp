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

#ifndef TYR_PLANNING_GROUND_AXIOM_EVALUATOR_HPP_
#define TYR_PLANNING_GROUND_AXIOM_EVALUATOR_HPP_

#include "tyr/planning/axiom_evaluator.hpp"

#include <atomic>
#include <memory>

namespace tyr::planning
{
template<>
class AxiomEvaluator<GroundTag>
{
    friend class AxiomEvaluatorFactory<GroundTag>;

private:
    struct Impl;

    AxiomEvaluator(ygg::uint_t index,
                   TaskPtr<GroundTag> task,
                   ygg::ExecutionContextPtr execution_context,
                   std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit AxiomEvaluator(std::unique_ptr<Impl> impl);

public:
    ~AxiomEvaluator();

    AxiomEvaluator(const AxiomEvaluator&) = delete;
    AxiomEvaluator& operator=(const AxiomEvaluator&) = delete;

    void compute_extended_state(ygg::Builder<State<GroundTag>>& state_builder);
    [[nodiscard]] AxiomEvaluatorPtr<GroundTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    ygg::uint_t get_index() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};
}

#endif
