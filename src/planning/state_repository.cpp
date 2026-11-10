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

#include "tyr/planning/state_repository.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/ground/state_storage/vectors.hpp"
#include "tyr/planning/state_storage/numeric.hpp"
#include "tyr/planning/state_storage/tree_bitset/atom.hpp"
#include "tyr/planning/state_storage/tree_bitset/fact.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_storage/vectors.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <span>
#include <type_traits>
#include <urd/elements/less.hpp>
#include <utility>
#include <urd/elements/prehashed.hpp>
#include <urd/repr/records/records.hpp>

namespace tyr::planning
{
namespace
{

/// Identifies the storage cohorts of a task kind; repositories share storage iff their identities match.
template<TaskKind Kind>
ygg::uint_t next_state_storage_identity() noexcept
{
    static auto next = std::atomic<ygg::uint_t> { 0 };
    return next.fetch_add(1, std::memory_order_relaxed);
}

void insert_initial_fluent_facts(const Task<GroundTag>& task, urd::builder<State<GroundTag>>& state)
{
    for (const auto fact : task.get_task().get_fluent_facts())
        state.set(fact.data());
}

void insert_initial_fluent_facts(const Task<LiftedTag>& task, urd::builder<State<LiftedTag>>& state)
{
    for (const auto atom : task.get_task().template get_atoms<formalism::FluentTag>())
        state.set(task.get_fdr_context()->get_fact(atom).data());
}

void prepare_builder(const Task<GroundTag>& task, urd::builder<State<GroundTag>>& state)
{
    state.clear();
    state.resize_fluent_facts(task.get_task().get_fluent_variables().size());
}

void prepare_builder(const Task<LiftedTag>&, urd::builder<State<LiftedTag>>& state) { state.clear(); }

}

template<TaskKind Kind>
struct StateRepository<Kind>::Impl
{
    template<bool ThreadSafe>
    struct Storage
    {
        /// The packed states, interned whole in a urd pool (references stay valid; thread-safe where ThreadSafe): its
        /// handle is the state's index.
        using PackedStates = urd::Store<urd::pool<PackedState<Kind>, urd::index<State<Kind>>>,
                                        urd::stable_refs,
                                        std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

        explicit Storage(const Task<Kind>& task) :
            identity(next_state_storage_identity<Kind>()),
            context(task),
            packed_states(urd::layer { 0 })
        {
        }

        ygg::uint_t identity;
        StateStorageContext<Kind, StateStoragePolicyTag, ThreadSafe> context;
        PackedStates packed_states;
    };

    template<bool ThreadSafe>
    using StoragePtr = std::conditional_t<ThreadSafe, std::shared_ptr<Storage<ThreadSafe>>, std::unique_ptr<Storage<ThreadSafe>>>;

    template<bool ThreadSafe>
    struct Evaluator
    {
        explicit Evaluator(StoragePtr<ThreadSafe> storage_) :
            storage(std::move(storage_)),
            fluent_backend(storage->context),
            derived_backend(storage->context),
            numeric_backend(storage->context),
            state_builder_pool()
        {
        }

        StoragePtr<ThreadSafe> storage;
        FactStorageBackend<Kind, StateStoragePolicyTag, ThreadSafe> fluent_backend;
        AtomStorageBackend<Kind, StateStoragePolicyTag, ThreadSafe> derived_backend;
        NumericStorageBackend<Kind, StateStoragePolicyTag, ThreadSafe> numeric_backend;
        urd::Store<urd::slots<urd::builder<State<Kind>>>, urd::concurrent> state_builder_pool;

        /// Inserts the fluent facts of `state`, as `delta` to `parent` if given.
        auto insert_fluents(const urd::data<State<Kind>>& state, const PackedState<Kind>* parent, const StateDelta* delta)
        {
            const auto& unpacked = state.fact_storage;
            return parent ? fluent_backend.insert(unpacked, parent->fact_storage, std::span(delta->fluents)) :
                            fluent_backend.insert(unpacked);
        }

        /// Inserts the derived atoms of `state`, as a difference to `parent_state` if given.
        /// Axioms recompute derived atoms, so their changes are not part of a StateDelta.
        auto insert_derived(const urd::data<State<Kind>>& state, const PackedState<Kind>* parent, const urd::data<State<Kind>>* parent_state)
        {
            const auto& unpacked = state.atom_storage;
            return parent ? derived_backend.insert(unpacked,
                                                   parent->atom_storage,
                                                   parent_state->atom_storage) :
                            derived_backend.insert(unpacked);
        }

        /// Inserts the numeric values of `state`, as `delta` to `parent` if given.
        auto insert_numeric(urd::builder<State<Kind>>& state, const PackedState<Kind>* parent, const StateDelta* delta)
        {
            auto& unpacked = state.numeric_storage;
            return parent ? numeric_backend.insert(unpacked, parent->numeric_storage, std::span(delta->numerics)) : numeric_backend.insert(unpacked);
        }

        /// `parent_state` and `delta` are either both null, or a registered state of this storage and the exact changes from it to `state`.
        template<typename ExtendState>
        void register_state(urd::builder<State<Kind>>& state,
                            ExtendState&& extend_state,
                            const urd::data<State<Kind>>* parent_state = nullptr,
                            const StateDelta* delta = nullptr)
        {
            assert((parent_state == nullptr) == (delta == nullptr));
#ifndef NDEBUG
            if (delta)
            {
                auto expected = StateDelta {};
                compute_state_delta(*parent_state, state, expected);
                assert(expected == *delta && "register_state: delta does not match the changes from the parent.");
            }
#endif
            auto& packed_states = storage->packed_states;
            const auto* parent = parent_state ? &packed_states[parent_state->index] : nullptr;
            const auto fluent = insert_fluents(state, parent, delta);
            const auto numeric = insert_numeric(state, parent, delta);
            const auto key = PackedState<Kind>(fluent, {}, numeric);
            const auto hash = urd::Hash<PackedState<Kind>> {}(key);
            const auto found = packed_states.find(urd::prehashed<PackedState<Kind>> { key, hash });
            auto state_index = found.is_none() ? std::optional<urd::index<State<Kind>>>() : std::optional(found);

            if (!state_index)
            {
                std::forward<ExtendState>(extend_state)();
                const auto derived = insert_derived(state, parent, parent_state);
                const auto [inserted_index, inserted] = packed_states.insert(urd::prehashed<PackedState<Kind>> { key, hash },
                                                                              [&](urd::index<State<Kind>> new_index)
                                                                              {
                                                                                  auto data = PackedState<Kind>(fluent, derived, numeric);
                                                                                  data.index = new_index;
                                                                                  return data;
                                                                              });
                state_index = inserted_index;
                if (inserted)
                {
                    state.set(inserted_index);
                    return;
                }
            }

            derived_backend.unpack(packed_states[*state_index].atom_storage, state.atom_storage);
            state.set(*state_index);
        }
    };

    Impl(ygg::uint_t index_, TaskPtr<Kind> task_, bool concurrent, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        task(std::move(task_)),
        evaluator(concurrent ? nullptr : std::make_unique<Evaluator<false>>(std::make_unique<Storage<false>>(*task))),
        shared_evaluator(concurrent ? std::make_unique<Evaluator<true>>(std::make_shared<Storage<true>>(*task)) : nullptr),
        storage_identity(concurrent ? shared_evaluator->storage->identity : evaluator->storage->identity)
    {
    }

    Impl(ygg::uint_t index_,
         TaskPtr<Kind> task_,
         std::shared_ptr<Storage<true>> storage,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        task(std::move(task_)),
        evaluator(),
        shared_evaluator(std::make_unique<Evaluator<true>>(std::move(storage))),
        storage_identity(shared_evaluator->storage->identity)
    {
    }

    template<typename Callback>
    decltype(auto) visit_evaluator(Callback&& callback)
    {
        if (evaluator)
            return std::forward<Callback>(callback)(*evaluator);
        return std::forward<Callback>(callback)(*shared_evaluator);
    }

    template<typename Callback>
    decltype(auto) visit_evaluator(Callback&& callback) const
    {
        if (evaluator)
            return std::forward<Callback>(callback)(static_cast<const Evaluator<false>&>(*evaluator));
        return std::forward<Callback>(callback)(static_cast<const Evaluator<true>&>(*shared_evaluator));
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    TaskPtr<Kind> task;
    std::unique_ptr<Evaluator<false>> evaluator;
    std::unique_ptr<Evaluator<true>> shared_evaluator;
    ygg::uint_t storage_identity;
};

/// The root of a cohort is worker 0; make_worker numbers the others.
template<TaskKind Kind>
StateRepository<Kind>::StateRepository(TaskPtr<Kind> task, bool concurrent) :
    m_impl(std::make_unique<Impl>(0, std::move(task), concurrent, std::make_shared<std::atomic<ygg::uint_t>>(1)))
{
}

template<TaskKind Kind>
StateRepository<Kind>::StateRepository(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
StateRepository<Kind>::~StateRepository() = default;

template<TaskKind Kind>
StateRepositoryPtr<Kind> StateRepository<Kind>::make_worker() const
{
    const auto index = m_impl->next_index->fetch_add(1, std::memory_order_relaxed);
    if (m_impl->evaluator)
        return StateRepositoryPtr<Kind>(new StateRepository<Kind>(std::make_unique<Impl>(index, m_impl->task, false, m_impl->next_index)));

    return StateRepositoryPtr<Kind>(
        new StateRepository<Kind>(std::make_unique<Impl>(index, m_impl->task, m_impl->shared_evaluator->storage, m_impl->next_index)));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto state_builder = get_state_builder();
    insert_initial_fluent_facts(*m_impl->task, *state_builder);

    for (const auto fterm_value : m_impl->task->get_task().template get_fterm_values<formalism::FluentTag>())
        state_builder->set(fterm_value.get_fterm().index(), fterm_value.get_value());

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::get_registered_state(urd::index<State<Kind>> state_index) const
{
    auto state_builder = get_state_builder();

    state_builder->set(state_index);
    m_impl->visit_evaluator(
        [&](auto& evaluator)
        {
            const auto& packed_state = packed(state_index);
            evaluator.fluent_backend.unpack(packed_state.fact_storage,
                                            state_builder->fact_storage);
            evaluator.derived_backend.unpack(packed_state.atom_storage,
                                             state_builder->atom_storage);
            evaluator.numeric_backend.unpack(packed_state.numeric_storage, state_builder->numeric_storage);
        });

    return StateView<Kind>(std::move(state_builder), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::create_state(
    AxiomEvaluator<Kind>& axiom_evaluator,
    const urd::Store<urd::sequence<urd::data<formalism::planning::FDRFact<formalism::FluentTag>>>>& fluent_facts,
    const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm, value);

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
template<typename FactView>
    requires std::same_as<typename FactView::value_type, formalism::planning::FDRFact<formalism::FluentTag>>
StateView<Kind>
StateRepository<Kind>::create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                    const urd::Store<urd::sequence<FactView>>& fluent_facts,
                                    const urd::Store<urd::sequence<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact.data());
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm.index(), value);

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
const PackedState<Kind>& StateRepository<Kind>::packed(urd::index<State<Kind>> state_index) const
{
    return m_impl->visit_evaluator(
        [&](const auto& evaluator) -> const PackedState<Kind>&
        {
            return evaluator.storage->packed_states[state_index];
        });
}

template<TaskKind Kind>
std::shared_ptr<urd::builder<State<Kind>>> StateRepository<Kind>::get_state_builder() const
{
    // Each owner keeps the repository alive; the slot goes back to the pool with the last one.
    auto state_builder = m_impl->visit_evaluator(
        [this](auto& evaluator) { return evaluator.state_builder_pool.make_shared(this->shared_from_this()); });
    prepare_builder(*m_impl->task, *state_builder);
    return state_builder;
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_state(AxiomEvaluator<Kind>& axiom_evaluator, std::shared_ptr<urd::builder<State<Kind>>> state)
{
    if (axiom_evaluator.get_task() != m_impl->task)
        throw std::invalid_argument("StateRepository::register_state(...): axiom evaluator belongs to a different task.");
    m_impl->visit_evaluator([&](auto& evaluator) { evaluator.register_state(*state, [&] { axiom_evaluator.compute_extended_state(*state); }); });
    return StateView<Kind>(std::move(state), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                                      std::shared_ptr<urd::builder<State<Kind>>> state,
                                                      const StateView<Kind>& parent,
                                                      const StateDelta& delta)
{
    if (axiom_evaluator.get_task() != m_impl->task)
        throw std::invalid_argument("StateRepository::register_state(...): axiom evaluator belongs to a different task.");
    // A delta insert reuses the parent's unchanged subtrees by their node indices, so the parent's trees must live in
    // this storage. A parent from separate storage would need its subtrees inserted here first, which is a full insert.
    const auto shares_storage = parent.get_state_repository().shares_storage_with(*this);
    const auto* parent_state = shares_storage ? &parent.data() : nullptr;
    const auto* parent_delta = shares_storage ? &delta : nullptr;
    m_impl->visit_evaluator([&](auto& evaluator)
                            { evaluator.register_state(*state, [&] { axiom_evaluator.compute_extended_state(*state); }, parent_state, parent_delta); });
    return StateView<Kind>(std::move(state), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_extended_state(std::shared_ptr<urd::builder<State<Kind>>> state)
{
    m_impl->visit_evaluator([&](auto& evaluator) { evaluator.register_state(*state, [] {}); });
    return StateView<Kind>(std::move(state), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_extended_state(const BuilderStateView<Kind>& source)
{
    if (&source.get_task() != get_task().get())
        throw std::invalid_argument("StateRepository::register_extended_state(...): source state belongs to a different task.");
    auto state = get_state_builder();
    *state = source.data();
    return register_extended_state(std::move(state));
}

template<TaskKind Kind>
size_t StateRepository<Kind>::memory_usage() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator)
                                   { return evaluator.storage->context.memory_usage() + urd::memory_usage(evaluator.storage->packed_states); });
}

template<TaskKind Kind>
const TaskPtr<Kind>& StateRepository<Kind>::get_task() const noexcept
{ return m_impl->task; }

template<TaskKind Kind>
bool StateRepository<Kind>::shares_storage_with(const StateRepository& other) const noexcept
{ return m_impl->storage_identity == other.m_impl->storage_identity; }

template<TaskKind Kind>
bool StateRepository<Kind>::is_concurrent() const noexcept
{ return static_cast<bool>(m_impl->shared_evaluator); }

template<TaskKind Kind>
ygg::uint_t StateRepository<Kind>::get_worker_index() const noexcept
{ return m_impl->index; }

template<TaskKind Kind>
std::size_t StateRepository<Kind>::layer() const noexcept
{ return m_impl->storage_identity; }

template<TaskKind Kind>
size_t StateRepository<Kind>::num_states() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator) { return evaluator.storage->packed_states.size(); });
}

template class StateRepository<GroundTag>;
template class StateRepository<LiftedTag>;

template StateView<GroundTag> StateRepository<GroundTag>::create_state(
    AxiomEvaluator<GroundTag>&,
    const urd::Store<urd::sequence<formalism::planning::FDRFactView<formalism::FluentTag>>>&,
    const urd::Store<urd::sequence<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateView<GroundTag> StateRepository<GroundTag>::create_state(
    AxiomEvaluator<GroundTag>&,
    const urd::Store<urd::sequence<urd::owning_view<formalism::planning::FDRFact<formalism::FluentTag>, formalism::planning::Repository>>>&,
    const urd::Store<urd::sequence<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateView<LiftedTag> StateRepository<LiftedTag>::create_state(
    AxiomEvaluator<LiftedTag>&,
    const urd::Store<urd::sequence<formalism::planning::FDRFactView<formalism::FluentTag>>>&,
    const urd::Store<urd::sequence<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateView<LiftedTag> StateRepository<LiftedTag>::create_state(
    AxiomEvaluator<LiftedTag>&,
    const urd::Store<urd::sequence<urd::owning_view<formalism::planning::FDRFact<formalism::FluentTag>, formalism::planning::Repository>>>&,
    const urd::Store<urd::sequence<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);

template StateView<GroundTag>
materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target, AxiomEvaluator<GroundTag>& axiom_evaluator);
template StateView<LiftedTag>
materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target, AxiomEvaluator<LiftedTag>& axiom_evaluator);

}
