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
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_storage/iterators.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <utility>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/core/concepts.hpp>

namespace ygg
{
namespace planning = ::tyr::planning;

template<::tyr::TaskKind Kind, typename Context>
struct View<ygg::Index<planning::State<Kind>>, Context>
{
    static_assert(ygg::dependent_false<Context>::value, "State views require a StateRepositoryPtr context.");
};

template<::tyr::TaskKind Kind>
struct View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>
{
public:
    using TaskType = planning::Task<Kind>;

    View(std::shared_ptr<planning::StateRepository<Kind>> owner, ygg::SharedObjectPoolPtr<Builder<planning::State<Kind>>, true> state_builder) noexcept;
    View(const View&);
    View(View&&) noexcept;
    View& operator=(const View&);
    View& operator=(View&&) noexcept;
    ~View();

    ygg::Index<planning::State<Kind>> get_index() const;

    bool test(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const;
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const;
    ::tyr::formalism::planning::FDRValue get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const;
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const;
    bool test(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const;

    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const;
    ygg::float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const;
    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const;
    ygg::float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const;
    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const;

    planning::AtomRange<::tyr::formalism::StaticTag> get_static_atoms() const noexcept;
    planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag> get_fluent_facts() const noexcept;
    planning::AtomRange<::tyr::formalism::DerivedTag> get_derived_atoms() const noexcept;
    planning::FunctionTermValueRange<::tyr::formalism::StaticTag> get_static_fterm_values() const noexcept;
    planning::FunctionTermValueRange<::tyr::formalism::FluentTag> get_fluent_fterm_values() const noexcept;

    auto get_static_atoms_view() const noexcept;
    auto get_fluent_facts_view() const noexcept;
    auto get_derived_atoms_view() const noexcept;
    auto get_static_fterm_values_view() const noexcept;
    auto get_fluent_fterm_values_view() const noexcept;

    const std::shared_ptr<::tyr::formalism::planning::Repository>& get_repository() const noexcept;
    const std::shared_ptr<planning::StateRepository<Kind>>& get_state_repository() const noexcept;
    const Builder<planning::State<Kind>>& get_state_builder() const noexcept;

    std::tuple<ygg::Index<planning::State<Kind>>, ygg::uint_t> identifying_members() const noexcept;

private:
    // The pooled builder must be released before its owning repository.
    std::shared_ptr<planning::StateRepository<Kind>> m_state_repository;
    ygg::SharedObjectPoolPtr<Builder<planning::State<Kind>>, true> m_state_builder;
};

template<::tyr::TaskKind Kind>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_static_atoms_view() const noexcept
{
    return get_static_atoms() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}

template<::tyr::TaskKind Kind>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fluent_facts_view() const noexcept
{
    return get_fluent_facts() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}

template<::tyr::TaskKind Kind>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_derived_atoms_view() const noexcept
{
    return get_derived_atoms() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}

template<::tyr::TaskKind Kind>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_static_fterm_values_view() const noexcept
{
    return get_static_fterm_values()
           | std::views::transform([context = this->get_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}

template<::tyr::TaskKind Kind>
auto View<ygg::Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fluent_fterm_values_view() const noexcept
{
    return get_fluent_fterm_values()
           | std::views::transform([context = this->get_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}
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
concept IterableStateConcept = requires(const T& cs) {
    requires AtomRangeConcept<decltype(cs.get_static_atoms()), formalism::StaticTag>;
    requires FactRangeConcept<decltype(cs.get_fluent_facts()), formalism::FluentTag>;
    requires AtomRangeConcept<decltype(cs.get_derived_atoms()), formalism::DerivedTag>;
    requires FunctionTermValueRangeConcept<decltype(cs.get_static_fterm_values()), formalism::StaticTag>;
    requires FunctionTermValueRangeConcept<decltype(cs.get_fluent_fterm_values()), formalism::FluentTag>;
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
concept IterableViewStateConcept = requires(const T& cs) {
    requires AtomViewRangeConcept<decltype(cs.get_static_atoms_view()), formalism::StaticTag>;
    requires FactViewRangeConcept<decltype(cs.get_fluent_facts_view()), formalism::FluentTag>;
    requires AtomViewRangeConcept<decltype(cs.get_derived_atoms_view()), formalism::DerivedTag>;
    requires FunctionTermViewValueRangeConcept<decltype(cs.get_static_fterm_values_view()), formalism::StaticTag>;
    requires FunctionTermViewValueRangeConcept<decltype(cs.get_fluent_fterm_values_view()), formalism::FluentTag>;
};

/**
 * IndexableStateConcept
 */

template<typename T, typename Kind>
concept IndexableStateConcept = requires(const T& cs,
                                         ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> variable,
                                         ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::StaticTag>> static_fterm,
                                         ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> fluent_fterm,
                                         ygg::Index<formalism::planning::Atom<GroundTag, formalism::StaticTag>> static_atom,
                                         ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> derived_atom) {
    requires TaskKind<Kind>;
    requires std::same_as<typename T::TaskType, Task<Kind>>;
    { cs.get_index() } -> std::same_as<ygg::Index<State<Kind>>>;
    { cs.get(variable) } -> std::same_as<formalism::planning::FDRValue>;
    { cs.get(static_fterm) } -> std::same_as<ygg::float_t>;
    { cs.get(fluent_fterm) } -> std::same_as<ygg::float_t>;
    { cs.test(static_atom) } -> std::same_as<bool>;
    { cs.test(derived_atom) } -> std::same_as<bool>;
    { cs.get_state_repository() } -> std::same_as<const std::shared_ptr<StateRepository<Kind>>&>;
};

/**
 * IndexableStateConcept
 */

template<typename T, typename Kind>
concept IndexableViewStateConcept = requires(const T& cs,
                                             formalism::planning::FDRVariableView<formalism::FluentTag> variable,
                                             formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> static_fterm,
                                             formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag> fluent_fterm,
                                             formalism::planning::AtomView<GroundTag, formalism::StaticTag> static_atom,
                                             formalism::planning::AtomView<GroundTag, formalism::DerivedTag> derived_atom) {
    requires TaskKind<Kind>;
    requires std::same_as<typename T::TaskType, Task<Kind>>;
    { cs.get_index() } -> std::same_as<ygg::Index<State<Kind>>>;
    { cs.get(variable) } -> std::same_as<formalism::planning::FDRValue>;
    { cs.get(static_fterm) } -> std::same_as<ygg::float_t>;
    { cs.get(fluent_fterm) } -> std::same_as<ygg::float_t>;
    { cs.test(static_atom) } -> std::same_as<bool>;
    { cs.test(derived_atom) } -> std::same_as<bool>;
    { cs.get_state_repository() } -> std::same_as<const std::shared_ptr<StateRepository<Kind>>&>;
};

}

#endif
