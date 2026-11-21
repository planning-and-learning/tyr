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

#ifndef TYR_PLANNING_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_STATE_REPOSITORY_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <atomic>
#include <boost/mp11.hpp>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <urd/config.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// Always owned by a StateRepositoryPtr (std::make_shared): the states it hands out keep it alive.
template<TaskKind Kind>
class StateRepository : public std::enable_shared_from_this<StateRepository<Kind>>
{
private:
    struct Impl;

    explicit StateRepository(std::unique_ptr<Impl> impl);

public:
    /// A private repository; with `concurrent`, one whose workers (make_worker) share canonical state storage.
    explicit StateRepository(TaskPtr<Kind> task, bool concurrent = false);
    ~StateRepository();

    StateRepository(const StateRepository&) = delete;
    StateRepository& operator=(const StateRepository&) = delete;
    StateRepository(StateRepository&&) = delete;
    StateRepository& operator=(StateRepository&&) = delete;

    StateView<Kind> get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator);

    /// The kind of record it holds: a repository nesting it resolves states here (ygg::formalism::Nests).
    using SymbolTypes = boost::mp11::mp_list<State<Kind>>;

    /// The stored record of a registered state (urd's resolves contract; a PackedStateView is the usual handle).
    [[nodiscard]] const urd::data<State<Kind>>& operator[](urd::index<State<Kind>> state_index) const;

    /// A registered state, packed: the view of its record's index, through this repository (as a state's view reads its
    /// index: StateView::pack).
    [[nodiscard]] PackedStateView<Kind> view(urd::index<State<Kind>> state_index) const noexcept { return { state_index, this }; }

    /// A state's storage being built, read through its task's formalism repository (a state's view reads its storages so).
    [[nodiscard]] FactStorageView<Kind> view(const urd::builder<FactStorage<Kind>, formalism::Repository>& storage) const noexcept
    { return { &storage, get_task()->get_repository().get() }; }

    [[nodiscard]] AtomStorageView<Kind> view(const urd::builder<AtomStorage<Kind>, formalism::Repository>& storage) const noexcept
    { return { &storage, get_task()->get_repository().get() }; }

    [[nodiscard]] NumericStorageView<Kind> view(const urd::builder<NumericStorage<Kind>, formalism::Repository>& storage) const noexcept
    { return { &storage, get_task()->get_repository().get() }; }

    /// A state record or a storage record held outside the repository, as its view here (a storage record's packed key
    /// is unpacked here: StateRepository::unpack); never a temporary.
    [[nodiscard]] urd::borrowed_view<State<Kind>, StateRepository> view(const urd::data<State<Kind>>& state URD_LIFETIMEBOUND) const noexcept
    { return { &state, this }; }

    [[nodiscard]] urd::borrowed_view<FactStorage<Kind>, StateRepository> view(const urd::data<FactStorage<Kind>>& storage URD_LIFETIMEBOUND) const noexcept
    { return { &storage, this }; }

    [[nodiscard]] urd::borrowed_view<AtomStorage<Kind>, StateRepository> view(const urd::data<AtomStorage<Kind>>& storage URD_LIFETIMEBOUND) const noexcept
    { return { &storage, this }; }

    [[nodiscard]] urd::borrowed_view<NumericStorage<Kind>, StateRepository>
    view(const urd::data<NumericStorage<Kind>>& storage URD_LIFETIMEBOUND) const noexcept
    { return { &storage, this }; }

    template<typename T>
    void view(const urd::data<T>&& record) const = delete;

    /// Const: the state's builder comes from the repository's pool of scratch builders.
    StateView<Kind> get_registered_state(urd::index<State<Kind>> state_index) const;

    /// A state record unpacked, its storages read from this repository's: a registered one's (get_registered_state, its
    /// index set) or a record held outside it, packed here but not registered (no index).
    StateView<Kind> unpack(const urd::data<State<Kind>>& packed) const;

    /// A state's storage unpacked from this repository's (urd::resolution<PackedStorage<S>>).
    urd::builder<FactStorage<Kind>, formalism::Repository> unpack(const PackedStorage<FactStorage<Kind>>& packed) const;
    urd::builder<AtomStorage<Kind>, formalism::Repository> unpack(const PackedStorage<AtomStorage<Kind>>& packed) const;
    urd::builder<NumericStorage<Kind>, formalism::Repository> unpack(const PackedStorage<NumericStorage<Kind>>& packed) const;

    StateView<Kind>
    create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                 const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>& fluent_facts,
                 const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values);
    /// The facts as views: borrowed (stored facts, FDRFactView) or owned (a state's, computed on the fly).
    template<typename FactView>
        requires std::same_as<typename FactView::value_type, formalism::FDRFact<formalism::FluentTag>>
    StateView<Kind> create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                 const urd::Store<urd::sequence<FactView>>& fluent_facts,
                                 const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values);

    std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> get_state_builder() const;

    /// The builder must come from this repository and have no retained mutable aliases.
    StateView<Kind> register_state(AxiomEvaluator<Kind>& axiom_evaluator, std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state);
    /// Like register_state, but `delta` must be the exact changes from `parent` to `state`, as produced when generating a successor.
    /// If both share storage, state component repositories may insert `state` as `delta` to `parent`.
    StateView<Kind> register_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                   std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state,
                                   const StateView<Kind>& parent,
                                   const StateDelta& delta);
    /// The builder must come from this repository, already contain its axiom closure, and have no retained mutable aliases.
    StateView<Kind> register_extended_state(std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state);
    /// Copies an already extended state of this task into repository-owned storage; source storage remains borrowed and unchanged.
    StateView<Kind> register_extended_state(const StateBuilderView<Kind>& source);

    /// Interns a state built elsewhere (a program state's, through a repository nesting this one): registered as an
    /// already extended state (register_extended_state), its index. Storage: the builder storage of the interning, unused.
    template<typename Storage>
    urd::index<State<Kind>> insert(const urd::builder<State<Kind>, StateRepository<Kind>>& built, Storage& /*storage*/)
    { return register_extended_state(urd::resolve(built, *this)).pack().index(); }

    [[nodiscard]] StateRepositoryPtr<Kind> make_worker() const;

    /// All repositories sharing this storage must be quiescent while inspecting memory usage.
    [[nodiscard]] size_t memory_usage() const noexcept;

    const TaskPtr<Kind>& get_task() const noexcept;
    bool shares_storage_with(const StateRepository& other) const noexcept;
    [[nodiscard]] bool is_concurrent() const noexcept;
    /// The index of this worker among the repositories sharing its storage.
    [[nodiscard]] ygg::uint_t get_worker_index() const noexcept;
    /// The repository's number, as urd's views ask of a context: the identity of the storage that owns the states,
    /// shared by all workers of a cohort.
    [[nodiscard]] std::size_t layer() const noexcept;
    [[nodiscard]] size_t num_states() const noexcept;

private:
    /// A state record's storages, read from this repository's, into a builder.
    void unpack_storages(const urd::data<State<Kind>>& packed, urd::builder<State<Kind>, StateRepository<Kind>>& state) const;

    std::unique_ptr<Impl> m_impl;
};

extern template class StateRepository<GroundTag>;
extern template class StateRepository<LiftedTag>;

template<TaskKind Kind>
StateView<Kind> materialize_state(const StateView<Kind>& source, StateRepository<Kind>& target, AxiomEvaluator<Kind>& axiom_evaluator)
{
    if (source.context().get_task() != target.get_task())
        throw std::invalid_argument("materialize_state(...): source state belongs to a different task.");
    if (axiom_evaluator.get_task() != target.get_task())
        throw std::invalid_argument("materialize_state(...): axiom evaluator belongs to a different task.");
    if (&source.context() == &target)
        return source;
    if (source.context().shares_storage_with(target))
        return target.get_registered_state(source.pack().index());

    auto builder = target.get_state_builder();
    builder->assign_unextended_part(source);
    return target.register_state(axiom_evaluator, std::move(builder));
}

extern template StateView<GroundTag>
materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target, AxiomEvaluator<GroundTag>& axiom_evaluator);
extern template StateView<LiftedTag>
materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target, AxiomEvaluator<LiftedTag>& axiom_evaluator);
}

#endif
