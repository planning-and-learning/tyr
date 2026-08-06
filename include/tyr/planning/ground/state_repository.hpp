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

#ifndef TYR_PLANNING_GROUND_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_GROUND_STATE_REPOSITORY_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/state_index.hpp"

#include <atomic>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace tyr::planning
{

template<>
class StateRepository<GroundTag> : public std::enable_shared_from_this<StateRepository<GroundTag>>
{
    friend class StateRepositoryFactory<GroundTag>;
    friend class SuccessorGenerator<GroundTag>;
    friend struct ::ygg::View<ygg::Index<State<GroundTag>>, StateRepositoryPtr<GroundTag>>;

private:
    struct Impl;

    StateRepository(ygg::uint_t index,
                    TaskPtr<GroundTag> task,
                    AxiomEvaluatorPtr<GroundTag> axiom_evaluator,
                    std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit StateRepository(std::unique_ptr<Impl> impl);
    [[nodiscard]] std::vector<StateRepositoryPtr<GroundTag>> make_shared_workers(std::span<const ygg::ExecutionContextPtr> execution_contexts) const;
    ygg::uint_t get_storage_identity() const noexcept;

public:
    ~StateRepository();

    StateView<GroundTag> get_initial_state();

    StateView<GroundTag> get_registered_state(ygg::Index<State<GroundTag>> state_index);

    StateView<GroundTag> create_state(
        const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>& fluent_facts,
        const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>>, ygg::float_t>>& fterm_values);

    StateView<GroundTag>
    create_state(const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>& fluent_facts,
                 const std::vector<::tyr::formalism::planning::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>& fterm_values);

    ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> get_state_builder();

    /// The builder must come from this repository and have no retained mutable aliases.
    StateView<GroundTag> register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> state);
    [[nodiscard]] StateRepositoryPtr<GroundTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    /// All repositories sharing this storage must be quiescent while inspecting memory usage.
    size_t memory_usage() const noexcept;

    const TaskPtr<GroundTag>& get_task() const noexcept;
    const AxiomEvaluatorPtr<GroundTag>& get_axiom_evaluator() const noexcept;
    ygg::uint_t get_index() const noexcept;
    size_t num_states() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

}

#endif
