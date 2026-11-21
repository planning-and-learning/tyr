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
#include "tyr/formalism/conjunctive_condition_view.hpp"
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
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_storage_view.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

/// A state's builder, registered (StateView: shared from its repository's pool, its index set) or being built
/// (StateBuilderView: borrowed, no index): its storages, each a view through the formalism repository, what it reads from
/// them through its task, and its index and packed form. Its context, the state repository of its task, must outlive the
/// view and its ranges.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<urd::builder<::tyr::planning::State<Kind>, ::tyr::planning::StateRepository<Kind>>, V>
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

    [[nodiscard]] ::tyr::formalism::FDRValue value(urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>> index) const
    { return get_fact_storage().get(index); }

    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
    { return get_numeric_storage().get(index); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
    { return get_atom_storage().test(index); }

    [[nodiscard]] bool test(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const { return test(view.index()); }

    [[nodiscard]] bool test(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    {
        const auto fact = std::as_const(*self().context().get_task()->get_fdr_context()).get_fact(view);
        return fact && value(fact->get_variable().index()) == fact->get_value();
    }

    [[nodiscard]] ygg::float_t value(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const
    { return value(view.index()); }

    [[nodiscard]] ::tyr::formalism::FDRValue value(::tyr::formalism::FDRVariableView<::tyr::formalism::FluentTag> view) const { return value(view.index()); }

    [[nodiscard]] ygg::float_t value(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    { return value(view.index()); }

    [[nodiscard]] bool test(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const { return test(view.index()); }

    /// Its fluent facts, as its fact storage holds them.
    [[nodiscard]] auto get_fluent_fact_index_values() const noexcept { return get_fact_storage().facts(); }

    /// The indices of its atoms of kind F: static ones its task's, fluent ones those of its facts, derived ones its atom storage's.
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_atom_indices() const noexcept
    {
        if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
            return self().context().get_task()->get_static_state().get_atoms();
        else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
            return get_fact_storage().fluent_atoms();
        else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
            return get_atom_storage().atoms();
        else
            static_assert(false);
    }

    /// Its function term values of kind F: static ones its task's, fluent ones its numeric storage's.
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_fterm_index_values() const noexcept
    {
        if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
            return self().context().get_task()->get_static_state().get_fterm_index_values();
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

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_atoms(::tyr::formalism::PredicateView<F> predicate) const
    {
        return get_atoms<F>() | std::views::filter([predicate](auto atom) { return atom.get_predicate() == predicate; });
    }

    /// The facts, computed on the fly: each view owns its fact.
    [[nodiscard]] auto get_fluent_facts() const noexcept
    {
        return get_fluent_fact_index_values()
               | std::views::transform([repository = get_formalism_repository().get()](auto fact)
                                       { return ::tyr::formalism::computed_fact_view(std::move(fact), *repository); });
    }

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_fterm_values() const noexcept
    {
        return get_fterm_index_values<F>()
               | std::views::transform([repository = get_formalism_repository().get()](auto&& pair)
                                       { return std::make_pair(urd::resolve(pair.first, *repository), pair.second); });
    }

    /// Its packed form, a registered state's only (StateView): its index, resolved through its repository
    /// (StateRepository::view). A state being built has none.
    [[nodiscard]] ::tyr::planning::PackedStateView<Kind> pack() const noexcept
        requires std::same_as<V, ::tyr::planning::StateView<Kind>>
    { return self().template member<&urd::record_of<V>::index>(); }

    static constexpr auto functions() noexcept
    {
        using Self = const view_mixin&;
        constexpr auto read = urd::Functions<
            urd::function<"get_formalism_repository", &view_mixin::get_formalism_repository>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>) const>(
                              &view_mixin::test),
                          urd::arg<"index">>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>) const>(
                              &view_mixin::test),
                          urd::arg<"index">>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag>) const>(&view_mixin::test),
                          urd::arg<"view">>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag>) const>(&view_mixin::test),
                          urd::arg<"view">>,
            urd::function<"test",
                          static_cast<bool (view_mixin::*)(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag>) const>(
                              &view_mixin::test),
                          urd::arg<"view">>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>)
                                          const>(&view_mixin::value),
                          urd::arg<"index">>,
            urd::function<"get",
                          static_cast<::tyr::formalism::FDRValue (view_mixin::*)(urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>>) const>(
                              &view_mixin::value),
                          urd::arg<"index">>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>)
                                          const>(&view_mixin::value),
                          urd::arg<"index">>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag>) const>(
                              &view_mixin::value),
                          urd::arg<"view">>,
            urd::function<"get",
                          static_cast<::tyr::formalism::FDRValue (view_mixin::*)(::tyr::formalism::FDRVariableView<::tyr::formalism::FluentTag>) const>(
                              &view_mixin::value),
                          urd::arg<"view">>,
            urd::function<"get",
                          static_cast<ygg::float_t (view_mixin::*)(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag>) const>(
                              &view_mixin::value),
                          urd::arg<"view">>,
            urd::function<"get_fluent_fact_index_values", &view_mixin::get_fluent_fact_index_values>,
            urd::function<"fluent_facts", &view_mixin::get_fluent_facts>,
            urd::function<"static_atoms",
                          static_cast<decltype(std::declval<Self>().template get_atoms<::tyr::formalism::StaticTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template get_atoms<::tyr::formalism::StaticTag>)>,
            urd::function<"fluent_atoms",
                          static_cast<decltype(std::declval<Self>().template get_atoms<::tyr::formalism::FluentTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template get_atoms<::tyr::formalism::FluentTag>)>,
            urd::function<"derived_atoms",
                          static_cast<decltype(std::declval<Self>().template get_atoms<::tyr::formalism::DerivedTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template get_atoms<::tyr::formalism::DerivedTag>)>,
            urd::function<"static_fterm_values",
                          static_cast<decltype(std::declval<Self>().template get_fterm_values<::tyr::formalism::StaticTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template get_fterm_values<::tyr::formalism::StaticTag>)>,
            urd::function<"fluent_fterm_values",
                          static_cast<decltype(std::declval<Self>().template get_fterm_values<::tyr::formalism::FluentTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template get_fterm_values<::tyr::formalism::FluentTag>)>,
            urd::function<"get_atoms",
                          static_cast<decltype(std::declval<Self>().template get_atoms<::tyr::formalism::StaticTag>(
                              std::declval<::tyr::formalism::PredicateView<::tyr::formalism::StaticTag>>())) (view_mixin::*)(
                              ::tyr::formalism::PredicateView<::tyr::formalism::StaticTag>) const>(
                              &view_mixin::template get_atoms<::tyr::formalism::StaticTag>),
                          urd::arg<"predicate">>,
            urd::function<"get_atoms",
                          static_cast<decltype(std::declval<Self>().template get_atoms<::tyr::formalism::FluentTag>(
                              std::declval<::tyr::formalism::PredicateView<::tyr::formalism::FluentTag>>())) (view_mixin::*)(
                              ::tyr::formalism::PredicateView<::tyr::formalism::FluentTag>) const>(
                              &view_mixin::template get_atoms<::tyr::formalism::FluentTag>),
                          urd::arg<"predicate">>,
            urd::function<"get_atoms",
                          static_cast<decltype(std::declval<Self>().template get_atoms<::tyr::formalism::DerivedTag>(
                              std::declval<::tyr::formalism::PredicateView<::tyr::formalism::DerivedTag>>())) (view_mixin::*)(
                              ::tyr::formalism::PredicateView<::tyr::formalism::DerivedTag>) const>(
                              &view_mixin::template get_atoms<::tyr::formalism::DerivedTag>),
                          urd::arg<"predicate">>> {};
        // pack(), where it has it (a registered state).
        if constexpr (std::same_as<V, ::tyr::planning::StateView<Kind>>)
            return urd::Functions<urd::function<"pack", &view_mixin::pack>> {} + read;
        else
            return read;
    }
};

/// A state, packed: registered (PackedStateView: the view of its record's index) or a record held outside its repository
/// (not registered): its storages packed, and the state unpacked (a registered one's with its index).
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::State<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_fact_storage() const noexcept { return self().template member<&urd::record_of<V>::fact_storage>(); }

    [[nodiscard]] decltype(auto) get_atom_storage() const noexcept { return self().template member<&urd::record_of<V>::atom_storage>(); }

    [[nodiscard]] decltype(auto) get_numeric_storage() const noexcept { return self().template member<&urd::record_of<V>::numeric_storage>(); }

    [[nodiscard]] ::tyr::planning::StateView<Kind> unpack() const
    {
        if constexpr (std::same_as<V, ::tyr::planning::PackedStateView<Kind>>)
            return self().context().get_registered_state(self().index());
        else
            return self().context().unpack(urd::detail::view_access::record(self()));
    }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"unpack", &view_mixin::unpack>> {}; }
};

#endif
