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

#ifndef TYR_PLANNING_LIFTED_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_LIFTED_STATE_REPOSITORY_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/state_index.hpp"

#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace tyr::planning
{

template<>
class StateRepository<LiftedTag> : public std::enable_shared_from_this<StateRepository<LiftedTag>>
{
    friend class StateRepositoryFactory<LiftedTag>;

private:
    struct Impl;

    StateRepository(ygg::uint_t index,
                    TaskPtr<LiftedTag> task,
                    AxiomEvaluatorPtr<LiftedTag> axiom_evaluator,
                    std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit StateRepository(std::unique_ptr<Impl> impl);

public:
    ~StateRepository();

    StateView<LiftedTag> get_initial_state();

    StateView<LiftedTag> get_registered_state(ygg::Index<State<LiftedTag>> state_index);

    StateView<LiftedTag> create_state(
        const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>& fluent_facts,
        const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>>, ygg::float_t>>& fterm_values);

    StateView<LiftedTag>
    create_state(const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>& fluent_facts,
                 const std::vector<::tyr::formalism::planning::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>& fterm_values);

    ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> get_state_builder();

    /// The builder must come from this repository and have no retained mutable aliases.
    StateView<LiftedTag> register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state);
    [[nodiscard]] StateRepositoryPtr<LiftedTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    size_t memory_usage() const noexcept;

    const TaskPtr<LiftedTag>& get_task() const noexcept;
    const AxiomEvaluatorPtr<LiftedTag>& get_axiom_evaluator() const noexcept;
    const ygg::ExecutionContextPtr& get_execution_context() const noexcept;
    ygg::uint_t get_index() const noexcept;
    size_t num_states() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

}

#endif
