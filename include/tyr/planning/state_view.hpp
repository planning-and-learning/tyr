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
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{
/// What every view of a state reads, through its record (self().data(): a registered state's data<State>, or a
/// builder) and its context, the state repository of its task, which must outlive the view and its ranges.
template<TaskKind Kind, typename V>
struct StateAccessors
{
protected:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] const Task<Kind>& get_task() const noexcept { return *self().context().get_task(); }
    [[nodiscard]] const auto& get_formalism_repository() const noexcept { return get_task().get_repository(); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    { return get_task().get_static_state().test(index); }
    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    { return get_task().get_static_state().get(index); }
    [[nodiscard]] ::tyr::formalism::planning::FDRValue value(urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
    { return self().data().get(index); }
    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
    { return self().data().get(index); }
    [[nodiscard]] bool test(urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
    { return self().data().test(index); }

    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const { return test(view.index()); }
    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const;
    [[nodiscard]] ygg::float_t value(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const
    { return value(view.index()); }
    [[nodiscard]] ::tyr::formalism::planning::FDRValue value(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
    { return value(view.index()); }
    [[nodiscard]] ygg::float_t value(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    { return value(view.index()); }
    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const { return test(view.index()); }

    [[nodiscard]] auto get_fluent_facts() const noexcept { return self().data().get_fluent_facts(); }
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept;

    template<::tyr::formalism::FactKind F>
    auto get_atoms_view() const noexcept;

    template<::tyr::formalism::FactKind F, typename C>
    auto get_atoms_view(urd::index_view<::tyr::formalism::Predicate<F>, C> predicate) const;

    /// The facts, computed on the fly: each view owns its fact.
    [[nodiscard]] auto get_fluent_facts_view() const noexcept { return self().data().get_fluent_facts_view(*get_formalism_repository()); }
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values_view() const noexcept;
};
}  // namespace tyr::planning

template<::tyr::TaskKind Kind, typename V>
bool ::tyr::planning::StateAccessors<Kind, V>::test(
    ::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
{
    const auto fact = std::as_const(*get_task().get_fdr_context()).get_fact(view);
    return fact && value(fact->get_variable()) == fact->get_value();
}

template<::tyr::TaskKind Kind, typename V>
template<::tyr::formalism::FactKind F>
auto ::tyr::planning::StateAccessors<Kind, V>::get_atoms() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return get_task().get_static_state().get_atoms();
    else
        return self().data().template get_atoms<F>(*get_formalism_repository());
}

template<::tyr::TaskKind Kind, typename V>
template<::tyr::formalism::FactKind F>
auto ::tyr::planning::StateAccessors<Kind, V>::get_fterm_values() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return get_task().get_static_state().get_fterm_values();
    else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return self().data().template get_fterm_values<F>();
    else
        static_assert(urd::repr::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind, typename V>
template<::tyr::formalism::FactKind F>
auto ::tyr::planning::StateAccessors<Kind, V>::get_atoms_view() const noexcept
{
    return get_atoms<F>() | std::views::transform([repository = get_formalism_repository().get()](auto id) { return urd::resolve(id, *repository); });
}

template<::tyr::TaskKind Kind, typename V>
template<::tyr::formalism::FactKind F, typename C>
auto ::tyr::planning::StateAccessors<Kind, V>::get_atoms_view(
    urd::index_view<::tyr::formalism::Predicate<F>, C> predicate) const
{
    return get_atoms_view<F>() | std::views::filter([predicate](auto atom) { return atom.get_predicate() == predicate; });
}

template<::tyr::TaskKind Kind, typename V>
template<::tyr::formalism::FactKind F>
auto ::tyr::planning::StateAccessors<Kind, V>::get_fterm_values_view() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
        return get_fterm_values<F>()
               | std::views::transform([repository = get_formalism_repository().get()](auto&& pair)
                                       { return std::make_pair(urd::resolve(pair.first, *repository), pair.second); });
    else
        return self().data().template get_fterm_values_view<F>(*get_formalism_repository());
}


/// A registered state (StateView: its data<State>, shared from its repository's pool): its accessors, and its identity,
/// its index in the repository that interned it.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::State<Kind>, V> : ::tyr::planning::StateAccessors<Kind, V>
{
    [[nodiscard]] const ::tyr::planning::StateRepository<Kind>& get_state_repository() const noexcept { return this->self().context(); }
    [[nodiscard]] urd::index<::tyr::planning::State<Kind>> index() const noexcept { return this->self().data().index; }
    [[nodiscard]] ::tyr::planning::PackedStateView<Kind> pack() const noexcept { return this->self().context().view(index()); }
};

/// A state being built (BuilderStateView): its accessors only. It has no index, so neither pack() nor a repository
/// that would name it.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<urd::builder<::tyr::planning::State<Kind>>, V> : ::tyr::planning::StateAccessors<Kind, V>
{
};

/// A registered state, packed (PackedStateView: its index; the view gives index()): its repository, and the state
/// unpacked.
template<::tyr::TaskKind Kind>
struct urd::view_mixin<::tyr::planning::State<Kind>, ::tyr::planning::PackedStateView<Kind>>
{
private:
    [[nodiscard]] const ::tyr::planning::PackedStateView<Kind>& self() const noexcept { return static_cast<const ::tyr::planning::PackedStateView<Kind>&>(*this); }

public:
    [[nodiscard]] const ::tyr::planning::StateRepository<Kind>& get_state_repository() const noexcept { return self().context(); }
    [[nodiscard]] ::tyr::planning::StateView<Kind> unpack() const { return self().context().get_registered_state(self().index()); }
};

#endif
