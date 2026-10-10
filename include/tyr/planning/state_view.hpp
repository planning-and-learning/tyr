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

#ifndef TYR_PLANNING_STATE_VIEW_HPP_
#define TYR_PLANNING_STATE_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_storage/iterators.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/core/concepts.hpp>

namespace ygg
{
namespace planning = ::tyr::planning;

/// Borrows the builder and task; both must outlive this view and its ranges.
template<::tyr::TaskKind Kind>
struct View<Builder<planning::State<Kind>>, planning::Task<Kind>> : BuilderViewBase<planning::State<Kind>, planning::Task<Kind>>
{
public:
    using KindType = Kind;
    using TaskType = planning::Task<Kind>;

    View(const Builder<planning::State<Kind>>& builder, const TaskType& task) noexcept : BuilderViewBase<planning::State<Kind>, TaskType>(builder, task) {}

    const auto& get_state_builder() const noexcept { return this->get_handle(); }
    const auto& get_task() const noexcept { return this->get_context(); }
    const auto& get_formalism_repository() const noexcept { return get_task().get_repository(); }

    bool test(Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    {
        return this->get_context().get_static_state().test(index);
    }
    float_t get(Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    {
        return this->get_context().get_static_state().get(index);
    }
    ::tyr::formalism::planning::FDRValue get(Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
    {
        return this->get_handle().get(index);
    }
    float_t get(Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const { return this->get_handle().get(index); }
    bool test(Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const { return this->get_handle().test(index); }

    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const { return test(view.get_index()); }
    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const;
    float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const { return get(view.get_index()); }
    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
    {
        return get(view.get_index());
    }
    float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const { return get(view.get_index()); }
    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const { return test(view.get_index()); }

    auto get_fluent_facts() const noexcept { return this->get_handle().get_fluent_facts(); }
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept;

    template<::tyr::formalism::FactKind F>
    auto get_atoms_view() const noexcept;

    template<::tyr::formalism::FactKind F, typename C>
    auto get_atoms_view(ygg::View<ygg::Index<::tyr::formalism::Predicate<F>>, C> predicate) const;

    auto get_fluent_facts_view() const noexcept { return this->get_handle().get_fluent_facts_view(*get_formalism_repository()); }
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values_view() const noexcept;

};

template<::tyr::TaskKind Kind, typename Context>
struct View<ygg::Index<planning::PackedState<Kind>>, Context>
{
    static_assert(ygg::dependent_false<Context>::value, "Packed state views require a StateRepositoryPtr context.");
};

/// Packed states are addressed by the index of the registered state they pack.
template<::tyr::TaskKind Kind>
struct View<ygg::Index<planning::PackedState<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>> :
    IndexViewBase<planning::State<Kind>, std::shared_ptr<planning::StateRepository<Kind>>>
{
public:
    using TaskType = planning::Task<Kind>;

    using IndexViewBase<planning::State<Kind>, std::shared_ptr<planning::StateRepository<Kind>>>::IndexViewBase;

    const std::shared_ptr<planning::StateRepository<Kind>>& get_state_repository() const noexcept { return this->get_context(); }

    planning::StateView<Kind> unpack() const;
};

template<::tyr::TaskKind Kind, typename Context>
struct View<ygg::Index<planning::State<Kind>>, Context>
{
    static_assert(ygg::dependent_false<Context>::value, "State views require a StateRepositoryPtr context.");
};

template<::tyr::TaskKind Kind>
struct View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>> :
    IndexViewBase<planning::State<Kind>, std::shared_ptr<planning::StateRepository<Kind>>>
{
public:
    using KindType = Kind;
    using TaskType = planning::Task<Kind>;

    View(std::shared_ptr<planning::StateRepository<Kind>> owner, ygg::SharedObjectPoolPtr<Builder<planning::State<Kind>>, true> state_builder) noexcept;
    View(const View&);
    View(View&&) noexcept;
    View& operator=(const View&);
    View& operator=(View&&) noexcept;
    ~View();

    planning::PackedStateView<Kind> pack() const noexcept;

    bool test(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const;
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const;
    ::tyr::formalism::planning::FDRValue get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const;
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const;
    bool test(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const;

    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const;
    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const;
    ygg::float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const;
    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const;
    ygg::float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const;
    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const;

    planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag> get_fluent_facts() const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept;
    template<::tyr::formalism::FactKind F>
    planning::FunctionTermValueRange<F> get_fterm_values() const noexcept;

    template<::tyr::formalism::FactKind F>
    auto get_atoms_view() const noexcept;
    template<::tyr::formalism::FactKind F, typename C>
    auto get_atoms_view(ygg::View<ygg::Index<::tyr::formalism::Predicate<F>>, C> predicate) const;

    auto get_fluent_facts_view() const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values_view() const noexcept;

    const std::shared_ptr<::tyr::formalism::planning::Repository>& get_formalism_repository() const noexcept;
    const TaskType& get_task() const noexcept;
    const std::shared_ptr<planning::StateRepository<Kind>>& get_state_repository() const noexcept;
    const Builder<planning::State<Kind>>& get_state_builder() const noexcept;

private:
    using Base = IndexViewBase<planning::State<Kind>, std::shared_ptr<planning::StateRepository<Kind>>>;

    // The pooled builder must be released before its owning repository, which the base holds.
    ygg::SharedObjectPoolPtr<Builder<planning::State<Kind>>, true> m_state_builder;
};

template<::tyr::TaskKind Kind>
bool View<Builder<planning::State<Kind>>, planning::Task<Kind>>::test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
{
    const auto fact = std::as_const(*get_task().get_fdr_context()).get_fact(view);
    return fact && get(fact->get_variable()) == fact->get_value();
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<Builder<planning::State<Kind>>, planning::Task<Kind>>::get_atoms() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return this->get_context().get_static_state().get_atoms();
    else
        return this->get_handle().template get_atoms<F>(*get_formalism_repository());
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<Builder<planning::State<Kind>>, planning::Task<Kind>>::get_fterm_values() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return this->get_context().get_static_state().get_fterm_values();
    else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return this->get_handle().template get_fterm_values<F>();
    else
        static_assert(ygg::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<Builder<planning::State<Kind>>, planning::Task<Kind>>::get_atoms_view() const noexcept
{
    return get_atoms<F>() | std::views::transform([repository = get_formalism_repository().get()](auto id) { return make_view(id, *repository); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F, typename C>
auto View<Builder<planning::State<Kind>>, planning::Task<Kind>>::get_atoms_view(ygg::View<ygg::Index<::tyr::formalism::Predicate<F>>, C> predicate) const
{
    return get_atoms_view<F>() | std::views::filter([predicate](auto atom) { return atom.get_predicate() == predicate; });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<Builder<planning::State<Kind>>, planning::Task<Kind>>::get_fterm_values_view() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return get_fterm_values<F>()
               | std::views::transform([repository = get_formalism_repository().get()](auto&& pair)
                                       { return std::make_pair(make_view(pair.first, *repository), pair.second); });
    else
        return this->get_handle().template get_fterm_values_view<F>(*get_formalism_repository());
}

template<::tyr::TaskKind Kind>
planning::StateView<Kind> View<Index<planning::PackedState<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::unpack() const
{
    return this->get_context()->get_registered_state(this->get_handle());
}

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::View(
    std::shared_ptr<planning::StateRepository<Kind>> owner,
    SharedObjectPoolPtr<Builder<planning::State<Kind>>, true> state_builder) noexcept :
    Base(state_builder->get_index(), std::move(owner)),
    m_state_builder(std::move(state_builder))
{
}

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::~View() = default;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::View(const View&) = default;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::View(View&&) noexcept = default;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::operator=(const View& other)
{
    if (this != &other)
    {
        m_state_builder = other.m_state_builder;
        Base::operator=(other);
    }
    return *this;
}

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::operator=(View&& other) noexcept
{
    if (this != &other)
    {
        m_state_builder = std::move(other.m_state_builder);
        Base::operator=(std::move(other));
    }
    return *this;
}

template<::tyr::TaskKind Kind>
planning::PackedStateView<Kind> View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::pack() const noexcept
{
    return planning::PackedStateView<Kind>(this->get_handle(), this->get_context());
}

template<::tyr::TaskKind Kind>
::tyr::formalism::planning::FDRValue View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
{
    return m_state_builder->test(index);
}

template<::tyr::TaskKind Kind>
const std::shared_ptr<planning::StateRepository<Kind>>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_state_repository() const noexcept
{
    return this->get_context();
}

template<::tyr::TaskKind Kind>
const planning::Task<Kind>& View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_task() const noexcept
{
    return *get_state_repository()->get_task();
}

template<::tyr::TaskKind Kind>
const Builder<planning::State<Kind>>& View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_state_builder() const noexcept
{
    return *m_state_builder;
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    ::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const
{
    return test(view.get_index());
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    ::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
{
    const auto fact = std::as_const(*get_task().get_fdr_context()).get_fact(view);
    return fact && get(fact->get_variable()) == fact->get_value();
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    ::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
::tyr::formalism::planning::FDRValue View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    ::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    ::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    ::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const
{
    return test(view.get_index());
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
{
    return get_state_repository()->get_task()->get_static_state().test(index);
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
{
    return get_state_repository()->get_task()->get_static_state().get(index);
}

template<::tyr::TaskKind Kind>
planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fluent_facts() const noexcept
{
    return m_state_builder->get_fluent_facts();
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_atoms() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return get_task().get_static_state().get_atoms();
    else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return get_fluent_facts_view() | std::views::transform([](auto fact) { return *fact.get_atom_index(); });
    else
        return get_state_builder().template get_atoms<F>(*get_formalism_repository());
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
planning::FunctionTermValueRange<F> View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fterm_values() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return get_task().get_static_state().get_fterm_values();
    else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return get_state_builder().template get_fterm_values<F>();
    else
        static_assert(ygg::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind>
const std::shared_ptr<::tyr::formalism::planning::Repository>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_formalism_repository() const noexcept
{
    return get_state_repository()->get_task()->get_repository();
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_atoms_view() const noexcept
{
    return get_atoms<F>() | std::views::transform([context = this->get_formalism_repository()](auto id) { return ygg::make_view(id, *context); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F, typename C>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_atoms_view(
    ygg::View<ygg::Index<::tyr::formalism::Predicate<F>>, C> predicate) const
{
    return get_atoms_view<F>() | std::views::filter([predicate](auto atom) { return atom.get_predicate() == predicate; });
}

template<::tyr::TaskKind Kind>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fluent_facts_view() const noexcept
{
    return get_fluent_facts() | std::views::transform([context = this->get_formalism_repository()](auto id) { return ygg::make_view(id, *context); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fterm_values_view() const noexcept
{
    return get_fterm_values<F>()
           | std::views::transform([context = this->get_formalism_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}

extern template class View<Index<planning::State<::tyr::GroundTag>>, std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>>;
extern template class View<Index<planning::PackedState<::tyr::GroundTag>>, std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>>;
extern template class View<Index<planning::State<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>;
extern template class View<Index<planning::PackedState<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>;
}

namespace tyr::planning
{
/**
 * IterableStateConcept
 */

template<class R, class Tag>
concept AtomRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, ygg::Index<formalism::planning::Atom<GroundTag, Tag>>>;

template<class R, class Tag>
concept FactRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, ygg::Data<formalism::planning::FDRFact<Tag>>>;

template<class R, class Tag>
concept FunctionTermValueRangeConcept = std::ranges::input_range<R>
                                        && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>,
                                                        std::pair<ygg::Index<formalism::planning::FunctionTerm<GroundTag, Tag>>, ygg::float_t>>;

template<typename T>
concept IterableStateConcept = requires (const std::remove_reference_t<T>& cs) {
    { cs.template get_atoms<formalism::StaticTag>() } -> AtomRangeConcept<formalism::StaticTag>;
    { cs.get_fluent_facts() } -> FactRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms<formalism::FluentTag>() } -> AtomRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms<formalism::DerivedTag>() } -> AtomRangeConcept<formalism::DerivedTag>;
    { cs.template get_fterm_values<formalism::StaticTag>() } -> FunctionTermValueRangeConcept<formalism::StaticTag>;
    { cs.template get_fterm_values<formalism::FluentTag>() } -> FunctionTermValueRangeConcept<formalism::FluentTag>;
};

/**
 * IterableViewStateConcept
 */

template<class R, class Tag>
concept AtomViewRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::planning::AtomView<GroundTag, Tag>>;

template<class R, class Tag>
concept FactViewRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::planning::FDRFactView<Tag>>;

template<class R, class Tag>
concept FunctionTermViewValueRangeConcept =
    std::ranges::input_range<R>
    && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::planning::FunctionTermViewValuePair<GroundTag, Tag>>;

template<typename T>
concept IterableViewStateConcept = requires (const std::remove_reference_t<T>& cs) {
    { cs.template get_atoms_view<formalism::StaticTag>() } -> AtomViewRangeConcept<formalism::StaticTag>;
    { cs.get_fluent_facts_view() } -> FactViewRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms_view<formalism::FluentTag>() } -> AtomViewRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms_view<formalism::DerivedTag>() } -> AtomViewRangeConcept<formalism::DerivedTag>;
    { cs.template get_fterm_values_view<formalism::StaticTag>() } -> FunctionTermViewValueRangeConcept<formalism::StaticTag>;
    { cs.template get_fterm_values_view<formalism::FluentTag>() } -> FunctionTermViewValueRangeConcept<formalism::FluentTag>;
};

/// State contents and task context, accepting values and references without requiring repository identity.
template<typename T, typename Kind>
concept StateViewConcept = TaskKind<Kind> && IterableStateConcept<T> && IterableViewStateConcept<T>
                           && requires (const std::remove_reference_t<T>& state,
                                       ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> variable,
                                       ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::StaticTag>> static_fterm,
                                       ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> fluent_fterm,
                                       ygg::Index<formalism::planning::Atom<GroundTag, formalism::StaticTag>> static_atom,
                                       ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> derived_atom,
                                       formalism::planning::FDRVariableView<formalism::FluentTag> variable_view,
                                       formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> static_fterm_view,
                                       formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag> fluent_fterm_view,
                                       formalism::planning::AtomView<GroundTag, formalism::StaticTag> static_atom_view,
                                       formalism::planning::AtomView<GroundTag, formalism::FluentTag> fluent_atom_view,
                                       formalism::planning::AtomView<GroundTag, formalism::DerivedTag> derived_atom_view,
                                       formalism::planning::PredicateView<formalism::StaticTag> static_predicate,
                                       formalism::planning::PredicateView<formalism::FluentTag> fluent_predicate,
                                       formalism::planning::PredicateView<formalism::DerivedTag> derived_predicate) {
                                  { state.get_task() } -> std::same_as<const Task<Kind>&>;
                                  { state.get_formalism_repository() } -> std::same_as<const formalism::planning::RepositoryPtr&>;
                                  { state.get(variable) } -> std::same_as<formalism::planning::FDRValue>;
                                  { state.get(static_fterm) } -> std::same_as<ygg::float_t>;
                                  { state.get(fluent_fterm) } -> std::same_as<ygg::float_t>;
                                  { state.test(static_atom) } -> std::same_as<bool>;
                                  { state.test(derived_atom) } -> std::same_as<bool>;
                                  { state.get(variable_view) } -> std::same_as<formalism::planning::FDRValue>;
                                  { state.get(static_fterm_view) } -> std::same_as<ygg::float_t>;
                                  { state.get(fluent_fterm_view) } -> std::same_as<ygg::float_t>;
                                  { state.test(static_atom_view) } -> std::same_as<bool>;
                                  { state.test(fluent_atom_view) } -> std::same_as<bool>;
                                  { state.test(derived_atom_view) } -> std::same_as<bool>;
                                  { state.get_atoms_view(static_predicate) } -> AtomViewRangeConcept<formalism::StaticTag>;
                                  { state.get_atoms_view(fluent_predicate) } -> AtomViewRangeConcept<formalism::FluentTag>;
                                  { state.get_atoms_view(derived_predicate) } -> AtomViewRangeConcept<formalism::DerivedTag>;
                              };

}

// State identity resolves through the owning StateRepository, which itself includes this header.
#include "tyr/planning/state_repository.hpp"

#endif
