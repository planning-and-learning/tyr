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

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <atomic>
#include <boost/mp11.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/config.hpp>
#include <urd/elements/builder.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/keyed.hpp>
#include <yggdrasil/formalism/nested.hpp>
#include <yggdrasil/formalism/part_instantiations.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::planning
{

/// Always owned by a StateRepositoryPtr (std::make_shared): the states it hands out keep it alive. A keyed repository
/// (ygg::formalism::KeyedRepository) of its own parts, each a store shared by its workers: its states' records, and the
/// pools of their storages (StoragePool: a storage's key is its pool's handle); over its task's formalism repository,
/// nested read only, through which a state's storages read the task's facts. Its records are read, viewed, found and
/// inserted, and a state's builder interned (its storages first, into their pools), by the keyed members (operator[],
/// at, view, find, insert); its domain operations below (register_state: the axiom closure, deltas to a parent) store
/// through them.
template<TaskKind Kind>
class StateRepository :
    public ygg::formalism::KeyedRepository<StateRepository<Kind>>,
    public ygg::formalism::Nests<StateRepository<Kind>, ygg::formalism::Nested<const formalism::Repository>>,
    public std::enable_shared_from_this<StateRepository<Kind>>
{
private:
    struct Impl;

    explicit StateRepository(std::unique_ptr<Impl> impl);

public:
    /// The states' records, each once (references stay valid; thread-safe, so that workers share it).
    using PackedStates = urd::Store<urd::pool<urd::data<State<Kind>>, urd::index<State<Kind>>>, urd::stable_refs, urd::layered, urd::concurrent>;

    /// A private repository; with `concurrent`, one whose workers (make_worker) share canonical state storage.
    explicit StateRepository(const TaskPtr<Kind>& task, bool concurrent = false);
    ~StateRepository();

    StateRepository(const StateRepository&) = delete;
    StateRepository& operator=(const StateRepository&) = delete;
    StateRepository(StateRepository&&) = delete;
    StateRepository& operator=(StateRepository&&) = delete;

    StateView<Kind> get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator);

    /// The kind of record it holds: a repository nesting it resolves states here (ygg::formalism::Nests).
    using SymbolTypes = boost::mp11::mp_list<State<Kind>>;
    /// The keys it resolves itself: its states' and their storages' (each its pool's handle).
    using KeyTypes = boost::mp11::mp_list<urd::index<State<Kind>>, urd::key<FactStorage<Kind>>, urd::key<AtomStorage<Kind>>, urd::key<NumericStorage<Kind>>>;

    /// Its own parts' stores and the keys each resolves (ygg::formalism::HeldPart<StateRepository, P>): its states, and the
    /// pools of their fluent facts, derived atoms and numeric values.
    using PartStores = boost::mp11::mp_list<PackedStates, StoragePool<FactStorage<Kind>>, StoragePool<AtomStorage<Kind>>, StoragePool<NumericStorage<Kind>>>;
    using PartKeys = boost::mp11::mp_list<boost::mp11::mp_list<urd::key<State<Kind>>>,
                                          boost::mp11::mp_list<urd::key<FactStorage<Kind>>>,
                                          boost::mp11::mp_list<urd::key<AtomStorage<Kind>>>,
                                          boost::mp11::mp_list<urd::key<NumericStorage<Kind>>>>;

    /// Part P of its own, read (Writable false) or written into.
    template<bool Writable, std::size_t P>
    using Part = ygg::formalism::HeldPart<std::conditional_t<Writable, StateRepository, const StateRepository>, P>;

    /// Its own parts (ygg::formalism::key_parts).
    template<bool Writable>
    using OwnParts = std::tuple<Part<Writable, 0>, Part<Writable, 1>, Part<Writable, 2>, Part<Writable, 3>>;

    /// No layer below it: its workers share its states instead.
    [[nodiscard]] const StateRepository* parent() const noexcept { return nullptr; }

    /// Const: the state's builder comes from the repository's pool of scratch builders.
    StateView<Kind> get_registered_state(urd::index<State<Kind>> state_index) const;

    /// A state record unpacked, its storages read from this repository's: a registered one's (get_registered_state, its
    /// index set) or a record held outside it, packed here but not registered (no index).
    StateView<Kind> unpack(const urd::data<State<Kind>>& packed) const;

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
    friend class ygg::formalism::KeyedRepository<StateRepository>;

    /// Its own parts as read; written into, likewise: its Impl's stores (state_repository.cpp).
    [[nodiscard]] OwnParts<false> own_parts() const;
    [[nodiscard]] OwnParts<true> own_parts();

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

// The keyed members of the repositories' parts (their states and their storages' pools), instantiated once, in
// src/planning/state_repository.cpp (brokkr's include rule I7).
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 0, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 1, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 2, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 3, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 0, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 1, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 2, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 3, 1)

#endif
