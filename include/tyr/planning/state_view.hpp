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

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

/// A state, registered (StateView: its data<State>, shared from its repository's pool) or being built (BuilderStateView:
/// its builder; urd's default mixin of a builder is this one): its storages and what it reads from them through its
/// task. A registered state's view also has its index and packed form (view_mixin<data<State>> below). Its context, the
/// state repository of its task, must outlive the view and its ranges.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::State<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_fact_storage() const noexcept { return self().template member<&urd::record_of<V>::fact_storage>(); }

    [[nodiscard]] decltype(auto) get_atom_storage() const noexcept { return self().template member<&urd::record_of<V>::atom_storage>(); }

    [[nodiscard]] decltype(auto) get_numeric_storage() const noexcept { return self().template member<&urd::record_of<V>::numeric_storage>(); }

    [[nodiscard]] const auto& get_formalism_repository() const noexcept { return self().context().get_task()->get_repository(); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    { return self().context().get_task()->get_static_state().test(index); }

    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    { return self().context().get_task()->get_static_state().get(index); }

    [[nodiscard]] ::tyr::formalism::planning::FDRValue value(urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
    { return get_fact_storage().get(index); }

    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
    { return get_numeric_storage().get(index); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
    { return get_atom_storage().test(index); }

    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const { return test(view.index()); }

    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    {
        const auto fact = std::as_const(*self().context().get_task()->get_fdr_context()).get_fact(view);
        return fact && value(fact->get_variable().index()) == fact->get_value();
    }

    [[nodiscard]] ygg::float_t value(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const
    { return value(view.index()); }

    [[nodiscard]] ::tyr::formalism::planning::FDRValue value(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
    { return value(view.index()); }

    [[nodiscard]] ygg::float_t value(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    { return value(view.index()); }

    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const { return test(view.index()); }

    /// Its fluent facts, as its fact storage holds them.
    [[nodiscard]] auto get_fluent_facts() const noexcept { return get_fact_storage().facts(); }

    /// The indices of its atoms of kind F: static ones its task's, fluent ones those of its facts, derived ones its atom storage's.
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_atom_indices() const noexcept
    {
        if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
            return self().context().get_task()->get_static_state().get_atoms();
        else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
            return ::tyr::planning::fluent_atoms(get_fact_storage(), *get_formalism_repository());
        else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
            return get_atom_storage().atoms();
        else
            static_assert(false);
    }

    /// Its function term values of kind F: static ones its task's, fluent ones its numeric storage's.
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_fterm_values() const noexcept
    {
        if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
            return self().context().get_task()->get_static_state().get_fterm_values();
        else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
            return get_numeric_storage().fterm_values();
        else
            static_assert(false);
    }

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_atoms() const noexcept
    {
        return get_atom_indices<F>()
               | std::views::transform([repository = get_formalism_repository().get()](auto id) { return urd::resolve(id, *repository); });
    }

    template<::tyr::formalism::FactKind F, typename C>
    [[nodiscard]] auto get_atoms(urd::index_view<::tyr::formalism::Predicate<F>, C> predicate) const
    {
        return get_atoms<F>() | std::views::filter([predicate](auto atom) { return atom.get_predicate() == predicate; });
    }

    /// The facts, computed on the fly: each view owns its fact.
    [[nodiscard]] auto get_fluent_facts_view() const noexcept
    {
        return get_fluent_facts()
               | std::views::transform([repository = get_formalism_repository().get()](auto fact)
                                       { return ::tyr::formalism::planning::computed_fact_view(std::move(fact), *repository); });
    }

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_fterm_values_view() const noexcept
    {
        return get_fterm_values<F>()
               | std::views::transform([repository = get_formalism_repository().get()](auto&& pair)
                                       { return std::make_pair(urd::resolve(pair.first, *repository), pair.second); });
    }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<
            urd::function<"get_formalism_repository", &view_mixin::get_formalism_repository>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>) const>(
                              &view_mixin::test)>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>) const>(
                              &view_mixin::test)>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag>) const>(
                              &view_mixin::test)>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag>) const>(
                              &view_mixin::test)>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag>) const>(
                              &view_mixin::test)>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>)
                                          const>(&view_mixin::value)>,
            urd::function<"get",
                          static_cast<::tyr::formalism::planning::FDRValue (view_mixin::*)(
                              urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>) const>(&view_mixin::value)>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>)
                                          const>(&view_mixin::value)>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag>)
                                          const>(&view_mixin::value)>,
            urd::function<"get",
                          static_cast<::tyr::formalism::planning::FDRValue (view_mixin::*)(
                              ::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag>) const>(&view_mixin::value)>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag>)
                                          const>(&view_mixin::value)>,
            urd::function<"get_fluent_facts", &view_mixin::get_fluent_facts>,
            urd::function<"get_fluent_facts_view", &view_mixin::get_fluent_facts_view>> {};
    }
};

/// A registered state (StateView): what every state reads, and its index and packed form in the repository that
/// registered it. A state being built has neither.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<urd::data<::tyr::planning::State<Kind>>, V> : urd::view_mixin<::tyr::planning::State<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    /// Its packed form: its index, resolved through its repository (StateRepository::view).
    [[nodiscard]] ::tyr::planning::PackedStateView<Kind> pack() const noexcept { return self().template member<&urd::record_of<V>::index>(); }

    [[nodiscard]] urd::index<::tyr::planning::State<Kind>> index() const noexcept { return self().context().state_index(pack().index()); }
};

/// A registered state, packed (PackedStateView): its packed storages, its repository, and the state unpacked.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::PackedState<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_fact_storage() const noexcept { return self().template member<&urd::record_of<V>::fact_storage>(); }

    [[nodiscard]] decltype(auto) get_atom_storage() const noexcept { return self().template member<&urd::record_of<V>::atom_storage>(); }

    [[nodiscard]] decltype(auto) get_numeric_storage() const noexcept { return self().template member<&urd::record_of<V>::numeric_storage>(); }

    [[nodiscard]] ::tyr::planning::StateView<Kind> unpack() const
    { return self().context().get_registered_state(self().context().state_index(self().index())); }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"unpack", &view_mixin::unpack>> {}; }
};

#endif
