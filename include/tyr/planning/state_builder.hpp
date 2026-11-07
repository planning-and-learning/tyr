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

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_storage/iterators.hpp"
#include "tyr/planning/state_builder_concepts.hpp"

#include <concepts>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A registered state, unpacked: its index and its storage, read only. A view of an index reads it, and so does the
/// builder that filled it, whose storage it is (builder<State<Kind>> derives from it). The repository's packed record of
/// a state is tyr::planning::PackedState.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::State<Kind>>
{
    urd::index<::tyr::planning::State<Kind>> index;
    ::tyr::planning::FactUnpackedStorage<Kind> fact_storage;
    ::tyr::planning::AtomUnpackedStorage<Kind> atom_storage;
    ::tyr::planning::NumericUnpackedStorage<Kind> numeric_storage;

    [[nodiscard]] urd::index<::tyr::planning::State<Kind>> get_index() const noexcept { return index; }

    [[nodiscard]] ::tyr::formalism::planning::FDRValue get(urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> variable) const
    { return fact_storage.get(variable); }
    [[nodiscard]] ygg::float_t get(urd::index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> fterm) const
    { return numeric_storage.get(fterm); }
    [[nodiscard]] bool test(urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom) const
    { return atom_storage.test(atom); }

    [[nodiscard]] ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
    { return get(view.index()); }
    [[nodiscard]] ygg::float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
    { return get(view.index()); }
    [[nodiscard]] bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const { return test(view.index()); }

    [[nodiscard]] auto get_fluent_facts() const noexcept { return fact_storage.facts(); }
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_atoms(const ::tyr::formalism::planning::Repository& repository) const noexcept;
    template<::tyr::formalism::FactKind F>
        requires std::same_as<F, ::tyr::formalism::FluentTag>
    [[nodiscard]] auto get_fterm_values() const noexcept
    { return numeric_storage.fterm_values(); }

    [[nodiscard]] auto get_fluent_facts_view(const ::tyr::formalism::planning::Repository& repository) const noexcept;
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_atoms_view(const ::tyr::formalism::planning::Repository& repository) const noexcept;
    template<::tyr::formalism::FactKind F>
    [[nodiscard]] auto get_fterm_values_view(const ::tyr::formalism::planning::Repository& repository) const noexcept;

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_atom_storage() const noexcept;
    [[nodiscard]] const ::tyr::planning::NumericUnpackedStorage<Kind>& get_numeric_variables() const noexcept { return numeric_storage; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::fact_storage, &data::atom_storage, &data::numeric_storage> {}; }
    /// A registered state is its index.
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::index> {}; }
};

/// A state being created, before it is registered: the storage of a data<State<Kind>>, mutable. It compares by its
/// contents (the fluent facts and numeric values), as the repository deduplicates them; its index is none until it is
/// registered.
template<::tyr::TaskKind Kind>
struct builder<::tyr::planning::State<Kind>> : data<::tyr::planning::State<Kind>>
{
private:
    using base = data<::tyr::planning::State<Kind>>;

public:
    using base::get;
    using base::get_atom_storage;
    using base::get_numeric_variables;
    using base::test;

    void set(urd::index<::tyr::planning::State<Kind>> index) { this->index = index; }
    void set(urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact) { this->fact_storage.set(fact); }
    void set(urd::index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> fterm, ygg::float_t value)
    { this->numeric_storage.set(fterm, value); }
    void set(urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom) { this->atom_storage.set(atom); }
    void set(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> view) { set(view.data()); }
    void set(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view, ygg::float_t value)
    { set(view.index(), value); }
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
    void assign_unextended_part(const base& other)
    {
        this->fact_storage = other.fact_storage;
        this->numeric_storage = other.numeric_storage;
    }
    /// Takes the whole state: its index and its storage.
    void assign(const base& other) { static_cast<base&>(*this) = other; }
    void swap(builder& other) noexcept
    {
        using std::swap;
        swap(this->index, other.index);
        this->fact_storage.swap(other.fact_storage);
        this->atom_storage.swap(other.atom_storage);
        this->numeric_storage.swap(other.numeric_storage);
    }

    friend void swap(builder& lhs, builder& rhs) noexcept { lhs.swap(rhs); }

    void resize_fluent_facts(size_t size)
        requires std::same_as<Kind, ::tyr::GroundTag>
    { this->fact_storage.resize(size); }
    void resize_derived_atoms(size_t size)
        requires std::same_as<Kind, ::tyr::GroundTag>
    { this->atom_storage.resize(size); }

    template<::tyr::formalism::FactKind T>
    auto& get_atom_storage() noexcept
    {
        if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return this->fact_storage;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return this->atom_storage;
        else
            static_assert(urd::repr::dependent_false<T>::value);
    }
    ::tyr::planning::NumericUnpackedStorage<Kind>& get_numeric_variables() noexcept { return this->numeric_storage; }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<[](const builder& b) -> const auto& { return b.fact_storage; }, [](const builder& b) -> const auto& { return b.numeric_storage; }> {};
    }
};

static_assert(::tyr::planning::StateBuilderConcept<urd::builder<::tyr::planning::State<::tyr::GroundTag>>, ::tyr::GroundTag>);
static_assert(::tyr::planning::StateBuilderConcept<urd::builder<::tyr::planning::State<::tyr::LiftedTag>>, ::tyr::LiftedTag>);

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
[[nodiscard]] auto urd::data<::tyr::planning::State<Kind>>::get_atoms(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return get_fluent_facts()
               | std::views::transform([repository = &repository_](auto fact) { return *urd::resolve(fact, *repository).get_atom_index(); });
    else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
        return urd::transform(::tyr::planning::SetBits(atom_storage.indices),
                              [](std::size_t i) { return urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, F>> { static_cast<ygg::uint_t>(i) }; });
    else
        static_assert(urd::repr::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind>
auto urd::data<::tyr::planning::State<Kind>>::get_fluent_facts_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    // Computed on the fly: each view holds its fact, read through the context that owns its variable.
    return get_fluent_facts()
           | std::views::transform([repository = &repository_](auto fact) { return ::tyr::formalism::planning::computed_fact_view(fact, *repository); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto urd::data<::tyr::planning::State<Kind>>::get_atoms_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_atoms<F>(repository_) | std::views::transform([repository = &repository_](auto id) { return urd::resolve(id, *repository); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto urd::data<::tyr::planning::State<Kind>>::get_fterm_values_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_fterm_values<F>()
           | std::views::transform([repository = &repository_](auto&& pair) { return std::make_pair(urd::resolve(pair.first, *repository), pair.second); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
const auto& urd::data<::tyr::planning::State<Kind>>::get_atom_storage() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return fact_storage;
    else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
        return atom_storage;
    else
        static_assert(urd::repr::dependent_false<F>::value);
}

}  // namespace urd

#endif
