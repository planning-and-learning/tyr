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

#ifndef TYR_PLANNING_STATE_BUILDER_HPP_
#define TYR_PLANNING_STATE_BUILDER_HPP_

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
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/lifted/state_storage.hpp"

#include <concepts>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace urd
{

/// A state being created, before it is registered: the storage of a data<State<Kind>>, mutable. It compares by its
/// contents (the fluent facts and numeric values), as the repository deduplicates them; its index is none until it is
/// registered.
template<::tyr::TaskKind Kind>
struct builder<::tyr::planning::State<Kind>> : data<::tyr::planning::State<Kind>>
{
private:
    using base = data<::tyr::planning::State<Kind>>;

public:
    builder() = default;
    /// A copy of a builder: its index and its storages (its record is not copied: records are move-only).
    builder(const builder& other) : base() { copy_from(other); }
    builder& operator=(const builder& other)
    {
        if (this != &other)
            copy_from(other);
        return *this;
    }
    builder(builder&& other) noexcept = default;
    builder& operator=(builder&& other) noexcept = default;
    ~builder() = default;

    /// What it holds, as a state's view reads it (the builder is the mutable record: it reads its own storages).
    [[nodiscard]] ::tyr::formalism::planning::FDRValue get(urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> variable) const
    {
        return this->fact_storage.get(variable);
    }
    [[nodiscard]] ygg::float_t get(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> fterm) const
    {
        return this->numeric_storage.get(fterm);
    }
    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom) const
    {
        return this->atom_storage.test(atom);
    }
    [[nodiscard]] auto get_fluent_facts() const noexcept { return this->fact_storage.facts(); }

    void set(urd::index<::tyr::planning::State<Kind>> state) { this->index = state; }
    void set(urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact) { this->fact_storage.set(fact); }
    void set(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> fterm, ygg::float_t value)
    {
        this->numeric_storage.set(fterm, value);
    }
    void set(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom) { this->atom_storage.set(atom); }
    /// A fact's view (borrowed or computed): its fact set.
    template<typename V>
        requires std::same_as<typename V::value_type, ::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>
    void set(const V& fact)
    {
        set(urd::make<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>(fact.get_variable(), fact.get_value()));
    }
    void set(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view, ygg::float_t value) { set(view.index(), value); }
    void set(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) { set(view.index()); }

    void clear()
    {
        this->index = {};
        clear_unextended_part();
        clear_extended_part();
    }
    void clear_unextended_part()
    {
        this->fact_storage.clear();
        this->numeric_storage.clear();
    }
    void clear_extended_part() { this->atom_storage.clear(); }
    /// Takes a state's facts and numeric values (a view of it: registered, or another builder's).
    template<typename V>
    void assign_unextended_part(const V& state)
    {
        this->fact_storage = state.get_fact_storage();
        this->numeric_storage = state.get_numeric_storage();
    }
    /// Takes another builder's facts and numeric values.
    void assign_unextended_part(const builder& other)
    {
        this->fact_storage = other.fact_storage;
        this->numeric_storage = other.numeric_storage;
    }
    /// Takes a registered state whole: its storages and its index.
    void assign(const ::tyr::planning::StateView<Kind>& state)
    {
        assign_storages(state);
        this->index = state.index();
    }
    /// Takes a state being built whole: its storages (it has no index).
    void assign(const ::tyr::planning::BuilderStateView<Kind>& state)
    {
        assign_storages(state);
        this->index = {};
    }
    /// Takes another builder whole.
    void assign(const builder& other) { copy_from(other); }
    void swap(builder& other) noexcept
    {
        using std::swap;
        swap(this->index, other.index);
        this->fact_storage.swap(other.fact_storage);
        this->atom_storage.swap(other.atom_storage);
        this->numeric_storage.swap(other.numeric_storage);
    }

    friend void swap(builder& lhs, builder& rhs) noexcept { lhs.swap(rhs); }

private:
    /// The storages of a state's view.
    template<typename V>
    void assign_storages(const V& state)
    {
        assign_unextended_part(state);
        this->atom_storage = state.get_atom_storage();
    }
    void copy_from(const builder& other)
    {
        this->index = other.index;
        this->fact_storage = other.fact_storage;
        this->atom_storage = other.atom_storage;
        this->numeric_storage = other.numeric_storage;
    }

public:
    void resize_fluent_facts(size_t size)
        requires std::same_as<Kind, ::tyr::GroundTag>
    {
        this->fact_storage.resize(size);
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<[](const builder& b) -> const auto& { return b.fact_storage; },
                            [](const builder& b) -> const auto& { return b.numeric_storage; }> {};
    }
};

}  // namespace urd

#endif
