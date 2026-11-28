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

#ifndef TYR_PLANNING_STATES_STATE_VIEW_HPP_
#define TYR_PLANNING_STATES_STATE_VIEW_HPP_

#include "tyr/declarations.hpp"
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
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_view_concepts.hpp"
#include "tyr/planning/states/storages/atom_storage_view.hpp"
#include "tyr/planning/states/storages/fact_storage_view.hpp"
#include "tyr/planning/states/storages/numeric_storage_view.hpp"
#include "tyr/planning/states/storages/temporal_storage_view.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <concepts>
#include <fmt/base.h>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <urd/elements/builder.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

/// A state's builder, registered (StateBuilderView: shared from its repository's pool, its index set) or being built
/// (StateBuilderView: borrowed, no index): its storages, each a view through the formalism repository, what it reads from
/// them through its task, and its index and packed form. Its context, the state repository of its task, must outlive the
/// view and its ranges.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<urd::builder<::tyr::planning::State<Kind>, ::tyr::planning::StateRepository<Kind>>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto fact_storage() const noexcept { return self().template member<&urd::record_of<V>::fact_storage>(); }

    [[nodiscard]] auto atom_storage() const noexcept { return self().template member<&urd::record_of<V>::atom_storage>(); }

    [[nodiscard]] auto numeric_storage() const noexcept { return self().template member<&urd::record_of<V>::numeric_storage>(); }

    [[nodiscard]] auto temporal_storage() const noexcept { return self().template member<&urd::record_of<V>::temporal_storage>(); }

    [[nodiscard]] const auto& formalism_repository() const noexcept { return self().context().get_task()->get_repository(); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    { return self().context().get_task()->get_static_state().test(index); }

    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>> index) const
    { return self().context().get_task()->get_static_state().get(index); }

    [[nodiscard]] ::tyr::formalism::FDRValue value(urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>> index) const
    { return fact_storage().get(index); }

    [[nodiscard]] ygg::float_t value(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
    { return numeric_storage().get(index); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
    { return atom_storage().test(index); }

    [[nodiscard]] bool test(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const { return test(view.index()); }

    [[nodiscard]] bool test(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    {
        const auto fact = std::as_const(*self().context().get_task()->get_fdr_context()).get_fact(view);
        return fact && value(fact->variable().index()) == fact->value();
    }

    [[nodiscard]] ygg::float_t value(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> view) const
    { return value(view.index()); }

    [[nodiscard]] ::tyr::formalism::FDRValue value(::tyr::formalism::FDRVariableView<::tyr::formalism::FluentTag> view) const { return value(view.index()); }

    [[nodiscard]] ygg::float_t value(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    { return value(view.index()); }

    [[nodiscard]] bool test(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const { return test(view.index()); }

    /// Its fluent facts, as its fact storage holds them.
    [[nodiscard]] auto fluent_fact_index_values() const noexcept { return fact_storage().facts(); }

    /// The indices of its atoms of kind F: static ones its task's, fluent ones those of its facts, derived ones its atom storage's.
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto atom_indices() const noexcept
    {
        if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
            return self().context().get_task()->get_static_state().get_atoms();
        else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
            return fact_storage().fluent_atoms();
        else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
            return atom_storage().atoms();
        else
            static_assert(false);
    }

    /// Its function term values of kind F: static ones its task's, fluent ones its numeric storage's.
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto fterm_index_values() const noexcept
    {
        if constexpr (std::same_as<F, ::tyr::formalism::StaticTag>)
            return self().context().get_task()->get_static_state().get_fterm_index_values();
        else if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
            return numeric_storage().fterm_values();
        else
            static_assert(false);
    }

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto atoms() const noexcept
    {
        return atom_indices<F>() | std::views::transform([repository = formalism_repository().get()](auto id) { return urd::resolve(id, *repository); });
    }

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto atoms(::tyr::formalism::PredicateView<F> predicate) const
    {
        return atoms<F>() | std::views::filter([predicate](auto atom) { return atom.predicate() == predicate; });
    }

    /// The facts, computed on the fly: each view owns its fact.
    [[nodiscard]] auto fluent_facts() const noexcept
    {
        return fluent_fact_index_values()
               | std::views::transform([repository = formalism_repository().get()](auto fact) { return urd::resolve(std::move(fact), *repository); });
    }

    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto fterm_values() const noexcept
    {
        return fterm_index_values<F>()
               | std::views::transform([repository = formalism_repository().get()](auto&& pair)
                                       { return std::make_pair(urd::resolve(pair.first, *repository), pair.second); });
    }

    /// Its packed form, the view of its builder's index, resolved through its repository (StateRepository::view): a
    /// registered state's; a state being built has none (its index is none).
    [[nodiscard]] ::tyr::planning::StateView<Kind> pack() const noexcept
        requires urd::Builder<urd::record_of<V>>
    { return self().template member<&urd::record_of<V>::index>(); }

    static constexpr auto functions() noexcept
    {
        using Self = const view_mixin&;
        constexpr auto read = urd::Functions<
            urd::function<"formalism_repository", &view_mixin::formalism_repository>,
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
            urd::function<"fluent_fact_index_values", &view_mixin::fluent_fact_index_values>,
            urd::function<"fluent_facts", &view_mixin::fluent_facts>,
            urd::function<"static_atoms",
                          static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::StaticTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template atoms<::tyr::formalism::StaticTag>)>,
            urd::function<"fluent_atoms",
                          static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::FluentTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template atoms<::tyr::formalism::FluentTag>)>,
            urd::function<"derived_atoms",
                          static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::DerivedTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template atoms<::tyr::formalism::DerivedTag>)>,
            urd::function<"static_fterm_values",
                          static_cast<decltype(std::declval<Self>().template fterm_values<::tyr::formalism::StaticTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template fterm_values<::tyr::formalism::StaticTag>)>,
            urd::function<"fluent_fterm_values",
                          static_cast<decltype(std::declval<Self>().template fterm_values<::tyr::formalism::FluentTag>()) (view_mixin::*)() const noexcept>(
                              &view_mixin::template fterm_values<::tyr::formalism::FluentTag>)>,
            urd::function<"atoms",
                          static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::StaticTag>(
                              std::declval<::tyr::formalism::PredicateView<::tyr::formalism::StaticTag>>())) (view_mixin::*)(
                              ::tyr::formalism::PredicateView<::tyr::formalism::StaticTag>) const>(&view_mixin::template atoms<::tyr::formalism::StaticTag>),
                          urd::arg<"predicate">>,
            urd::function<"atoms",
                          static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::FluentTag>(
                              std::declval<::tyr::formalism::PredicateView<::tyr::formalism::FluentTag>>())) (view_mixin::*)(
                              ::tyr::formalism::PredicateView<::tyr::formalism::FluentTag>) const>(&view_mixin::template atoms<::tyr::formalism::FluentTag>),
                          urd::arg<"predicate">>,
            urd::function<"atoms",
                          static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::DerivedTag>(
                              std::declval<::tyr::formalism::PredicateView<::tyr::formalism::DerivedTag>>())) (view_mixin::*)(
                              ::tyr::formalism::PredicateView<::tyr::formalism::DerivedTag>) const>(&view_mixin::template atoms<::tyr::formalism::DerivedTag>),
                          urd::arg<"predicate">>> {};
        // pack(), where it has it (a builder's view).
        if constexpr (urd::Builder<urd::record_of<V>>)
            return urd::Functions<urd::function<"pack", &view_mixin::pack>> {} + read;
        else
            return read;
    }
};

/// A state, packed: registered (StateView: the view of its record's index) or a record held outside its repository
/// (not registered): its storages packed, and the state unpacked (a registered one's with its index).
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::State<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto fact_storage() const noexcept { return self().template member<&urd::record_of<V>::fact_storage>(); }

    [[nodiscard]] auto atom_storage() const noexcept { return self().template member<&urd::record_of<V>::atom_storage>(); }

    [[nodiscard]] auto numeric_storage() const noexcept { return self().template member<&urd::record_of<V>::numeric_storage>(); }

    [[nodiscard]] auto temporal_storage() const noexcept { return self().template member<&urd::record_of<V>::temporal_storage>(); }

    [[nodiscard]] ::tyr::planning::SharedStateBuilderView<Kind> unpack() const
    {
        // A registered state by its key (its index kept); a record held outside its repository as it is.
        if constexpr (urd::Indexed<V>)
            return ::tyr::planning::share(self().context(), ::tyr::planning::get_registered_state(self().context(), self().index()));
        else
            return ::tyr::planning::share(self().context(), ::tyr::planning::unpack(self().context(), urd::record(self())));
    }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"unpack", &view_mixin::unpack>> {}; }
};

namespace tyr::planning
{

/// A state read through its repository by its storages: a view of its builder or of its record.
template<typename V>
concept StateReadable = requires(const V& state) {
    state.fact_storage();
    state.numeric_storage();
    state.temporal_storage();
};

/// The hash of a state's unextended content (its facts, numeric values and agenda, not its derived atoms): its builder's
/// (urd::Hash: by its identifying storages, each by what it holds, not by its view, which hashes with its context), so
/// that the same state hashes alike whichever repository it is read through (the workers').
template<urd::BuilderView V>
ygg::hash_t hash_unextended(const V& state)
{ return urd::Hash<> {}(urd::record(state)); }

/// Whether two states have equal unextended content (hash_unextended), whichever repositories they are read through.
template<urd::BuilderView V, urd::BuilderView W>
bool equal_unextended(const V& lhs, const W& rhs)
{ return urd::EqualTo<> {}(urd::record(lhs), urd::record(rhs)); }

}

/// A state, registered or being built: its atoms, facts and numeric values.
template<::tyr::TaskKind Kind>
struct fmt::formatter<::tyr::planning::SharedStateBuilderView<Kind>, char> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateBuilderView<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct fmt::formatter<::tyr::planning::StateBuilderView<Kind>, char> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateBuilderView<Kind>& value, fmt::format_context& ctx) const;
};

#endif
