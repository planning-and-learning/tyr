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

#ifndef TYR_PLANNING_STATES_STATE_BUILDER_HPP_
#define TYR_PLANNING_STATES_STATE_BUILDER_HPP_

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
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/states/storages/atom_storage_builder.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/ground/fact_storage_builder.hpp"
#include "tyr/planning/states/storages/lifted/fact_storage_builder.hpp"
#include "tyr/planning/states/storages/numeric_storage_builder.hpp"
#include "tyr/planning/states/storages/temporal_storage_builder.hpp"
#include "tyr/planning/states/state_data.hpp"

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/handle.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace urd
{

/// A state's builder: its record's members (state_data.hpp), each storage held only built (urd::built_child: its
/// builder's handle, never null: made with the builder, cleared in place with it, its buffers kept), of the state's
/// context C (its state repository, or a repository nesting it), through which they read the task's facts. A
/// repository's state builders are pooled (StateRepository::get_state_builder). A state being created has no index
/// until it is registered; a registered state's pooled builder (StateBuilderView) holds its index. It compares by its
/// contents (the fluent facts and numeric values), as the repository deduplicates states.
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::State<Kind>, C>
{
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes): its record's members, read and written by
    // name as the record's are.
    urd::key<::tyr::planning::State<Kind>> index;
    urd::built_child<::tyr::planning::FactStorage<Kind>, C> fact_storage = urd::make_owned<urd::builder<::tyr::planning::FactStorage<Kind>, C>>();
    urd::built_child<::tyr::planning::AtomStorage<Kind>, C> atom_storage = urd::make_owned<urd::builder<::tyr::planning::AtomStorage<Kind>, C>>();
    urd::built_child<::tyr::planning::NumericStorage<Kind>, C> numeric_storage = urd::make_owned<urd::builder<::tyr::planning::NumericStorage<Kind>, C>>();
    urd::built_child<::tyr::planning::TemporalStorage<Kind>, C> temporal_storage = urd::make_owned<urd::builder<::tyr::planning::TemporalStorage<Kind>, C>>();

    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    void set(urd::index<::tyr::planning::State<Kind>> state) { index = state; }

    void set(urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>> fact) { fact_storage->set(fact); }

    void set(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> fterm, ygg::float_t value)
    { numeric_storage->set(fterm, value); }

    /// The value of `fterm` its numeric storage holds built (NaN where it has none).
    [[nodiscard]] ygg::float_t get(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> fterm) const
    { return numeric_storage->get(fterm); }

    void set(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom) { atom_storage->set(atom); }

    /// A fact's view (borrowed or computed): its fact set.
    template<typename V>
        requires std::same_as<typename V::value_type, ::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>
    void set(const V& fact)
    { set(urd::make<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>(fact.get_variable(), fact.get_value())); }

    void set(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view, ygg::float_t value) { set(view.index(), value); }

    void set(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) { set(view.index()); }

    void clear()
    {
        index = {};
        clear_unextended_part();
        clear_extended_part();
    }

    void clear_unextended_part()
    {
        fact_storage->clear();
        numeric_storage->clear();
        temporal_storage->clear();
    }

    void clear_extended_part() { atom_storage->clear(); }

    /// Takes a state's facts, numeric values and agenda (a view of it: registered, or another builder's).
    template<typename V>
    void assign_unextended_part(const V& state)
    {
        fact_storage->assign(state.get_fact_storage());
        numeric_storage->assign(state.get_numeric_storage());
        temporal_storage->assign(state.get_temporal_storage());
    }

    /// Takes a state whole: its storages and its index (none for a state being built).
    void assign(const ::tyr::planning::StateBuilderView<Kind>& state)
    {
        assign_storages(state);
        index = state.pack().index();
    }

    void swap(builder& other) noexcept
    {
        using std::swap;
        swap(index, other.index);
        swap(fact_storage, other.fact_storage);
        swap(atom_storage, other.atom_storage);
        swap(numeric_storage, other.numeric_storage);
        swap(temporal_storage, other.temporal_storage);
    }

    void resize_fluent_facts(size_t size)
        requires std::same_as<Kind, ::tyr::GroundTag>
    { fact_storage->resize(size); }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"fact_storage", &builder::fact_storage>,
                           urd::field<"atom_storage", &builder::atom_storage>,
                           urd::field<"numeric_storage", &builder::numeric_storage>,
                           urd::field<"temporal_storage", &builder::temporal_storage>> {};
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&builder::fact_storage, &builder::numeric_storage, &builder::temporal_storage> {}; }

    static constexpr auto functions() noexcept
    {
        using FluentTerm = ::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>;
        using DerivedAtom = ::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>;
        constexpr auto mutators = urd::Functions<
            urd::function<"set", static_cast<void (builder::*)(urd::index<::tyr::planning::State<Kind>>)>(&builder::set), urd::arg<"state">>,
            urd::function<"set",
                          static_cast<void (builder::*)(urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>)>(&builder::set),
                          urd::arg<"fact">>,
            urd::function<"set", static_cast<void (builder::*)(urd::index<FluentTerm>, ygg::float_t)>(&builder::set), urd::arg<"fterm">, urd::arg<"value">>,
            urd::function<"set", static_cast<void (builder::*)(urd::index<DerivedAtom>)>(&builder::set), urd::arg<"atom">>,
            urd::function<"set",
                          static_cast<void (builder::*)(::tyr::formalism::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag>, ygg::float_t)>(
                              &builder::set),
                          urd::arg<"fterm">,
                          urd::arg<"value">>,
            urd::function<"set",
                          static_cast<void (builder::*)(::tyr::formalism::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag>)>(&builder::set),
                          urd::arg<"atom">>,
            urd::function<"clear", &builder::clear>,
            urd::function<"clear_unextended_part", &builder::clear_unextended_part>,
            urd::function<"clear_extended_part", &builder::clear_extended_part>,
            urd::function<"assign_unextended_part", &builder::template assign_unextended_part<::tyr::planning::StateBuilderView<Kind>>, urd::arg<"state">>,
            urd::function<"assign", &builder::assign, urd::arg<"state">>,
            urd::function<"swap", &builder::swap, urd::arg<"other">>> {};
        // resize_fluent_facts, where it has it (a ground state: a value per FDR variable).
        if constexpr (std::same_as<Kind, ::tyr::GroundTag>)
            return mutators + urd::Functions<urd::function<"resize_fluent_facts", &builder::resize_fluent_facts, urd::arg<"size">>> {};
        else
            return mutators;
    }

private:
    /// The storages of a state's view.
    template<typename V>
    void assign_storages(const V& state)
    {
        assign_unextended_part(state);
        atom_storage->assign(state.get_atom_storage());
    }
};

}  // namespace urd

namespace tyr::planning
{
}

#endif
