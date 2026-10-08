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
#include "tyr/planning/ground/state_storage/iterators.hpp"
#include "tyr/planning/lifted/state_storage/iterators.hpp"
#include "tyr/planning/state_index.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<typename T, typename Kind>
concept StateBuilderConcept = requires(std::remove_reference_t<T>& s,
                                       const std::remove_reference_t<T>& cs,
                                       ygg::Index<State<Kind>> index,
                                       ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> variable,
                                       ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>> fact,
                                       ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> fterm,
                                       ygg::float_t value,
                                       ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> atom) {
    requires TaskKind<Kind>;
    { s.clear() };
    { s.clear_unextended_part() };
    { s.clear_extended_part() };
    { s.assign_unextended_part(cs) };
    { s.swap(s) } noexcept;
    { cs.get_index() } -> std::same_as<ygg::Index<State<Kind>>>;
    { s.set(index) };
    { cs.get(variable) } -> std::same_as<formalism::planning::FDRValue>;
    { s.set(fact) };
    { cs.get(fterm) } -> std::same_as<ygg::float_t>;
    { s.set(fterm, value) };
    { cs.test(atom) } -> std::same_as<bool>;
    { s.set(atom) };
};

}

namespace ygg
{

template<::tyr::TaskKind Kind>
struct Builder<::tyr::planning::State<Kind>>
{
public:
    using StateType = ::tyr::planning::State<Kind>;
    using TaskType = ::tyr::planning::Task<Kind>;

    Builder() = default;

    ygg::Index<StateType> get_index() const;
    void set(ygg::Index<StateType> index);

    ::tyr::formalism::planning::FDRValue get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const;
    void set(ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact);
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const;
    void set(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index, ygg::float_t value);
    bool test(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const;
    void set(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index);

    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const;
    void set(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> view);
    ygg::float_t get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const;
    void set(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view, ygg::float_t value);
    bool test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const;
    void set(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view);

    ::tyr::planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag> get_fluent_facts() const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_atoms(const ::tyr::formalism::planning::Repository& repository) const noexcept;
    template<::tyr::formalism::FactKind F>
    ::tyr::planning::FunctionTermValueRange<F> get_fterm_values() const noexcept;

    auto get_fluent_facts_view(const ::tyr::formalism::planning::Repository& repository) const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_atoms_view(const ::tyr::formalism::planning::Repository& repository) const noexcept;
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values_view(const ::tyr::formalism::planning::Repository& repository) const noexcept;

    void clear();
    void clear_unextended_part();
    void clear_extended_part();
    void assign_unextended_part(const Builder& other);
    void swap(Builder& other) noexcept;

    friend void swap(Builder& lhs, Builder& rhs) noexcept { lhs.swap(rhs); }

    void resize_fluent_facts(size_t num_fluent_facts)
        requires std::same_as<Kind, ::tyr::GroundTag>;
    void resize_derived_atoms(size_t num_derived_atoms)
        requires std::same_as<Kind, ::tyr::GroundTag>;

    template<::tyr::formalism::FactKind T>
    auto& get_atom_storage() noexcept;
    template<::tyr::formalism::FactKind T>
    const auto& get_atom_storage() const noexcept;

    ::tyr::planning::NumericUnpackedStorage<Kind>& get_numeric_variables() noexcept;
    const ::tyr::planning::NumericUnpackedStorage<Kind>& get_numeric_variables() const noexcept;

    auto identifying_members() const noexcept { return std::tie(m_fact_storage, m_numeric_storage); }

private:
    ygg::Index<StateType> m_index;

    ::tyr::planning::FactUnpackedStorage<Kind> m_fact_storage;
    ::tyr::planning::AtomUnpackedStorage<Kind> m_atom_storage;
    ::tyr::planning::NumericUnpackedStorage<Kind> m_numeric_storage;
};

static_assert(::tyr::planning::StateBuilderConcept<Builder<::tyr::planning::State<::tyr::GroundTag>>, ::tyr::GroundTag>);
static_assert(::tyr::planning::StateBuilderConcept<Builder<::tyr::planning::State<::tyr::LiftedTag>>, ::tyr::LiftedTag>);

template<::tyr::TaskKind Kind>
ygg::Index<::tyr::planning::State<Kind>> Builder<::tyr::planning::State<Kind>>::get_index() const
{
    return m_index;
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(ygg::Index<::tyr::planning::State<Kind>> index)
{
    m_index = index;
}

template<::tyr::TaskKind Kind>
::tyr::formalism::planning::FDRValue
Builder<::tyr::planning::State<Kind>>::get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
{
    return m_fact_storage.get(index);
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact)
{
    m_fact_storage.set(fact);
}

template<::tyr::TaskKind Kind>
ygg::float_t
Builder<::tyr::planning::State<Kind>>::get(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
{
    return m_numeric_storage.get(index);
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index,
                                                ygg::float_t value)
{
    m_numeric_storage.set(index, value);
}

template<::tyr::TaskKind Kind>
bool Builder<::tyr::planning::State<Kind>>::test(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
{
    return m_atom_storage.test(index);
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index)
{
    m_atom_storage.set(index);
}

template<::tyr::TaskKind Kind>
::tyr::formalism::planning::FDRValue
Builder<::tyr::planning::State<Kind>>::get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> view)
{
    set(view.get_data());
}

template<::tyr::TaskKind Kind>
ygg::float_t Builder<::tyr::planning::State<Kind>>::get(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> view,
                                                ygg::float_t value)
{
    set(view.get_index(), value);
}

template<::tyr::TaskKind Kind>
bool Builder<::tyr::planning::State<Kind>>::test(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view) const
{
    return test(view.get_index());
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::set(::tyr::formalism::planning::AtomView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> view)
{
    set(view.get_index());
}

template<::tyr::TaskKind Kind>
::tyr::planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag> Builder<::tyr::planning::State<Kind>>::get_fluent_facts() const noexcept
{
    return ::tyr::planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag>(m_fact_storage);
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto Builder<::tyr::planning::State<Kind>>::get_atoms(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return get_fluent_facts()
               | std::views::transform([repository = &repository_](auto fact) { return *ygg::make_view(fact, *repository).get_atom_index(); });
    else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
        return ::tyr::planning::AtomRange<F>(m_atom_storage);
    else
        static_assert(ygg::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
::tyr::planning::FunctionTermValueRange<F> Builder<::tyr::planning::State<Kind>>::get_fterm_values() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return ::tyr::planning::FunctionTermValueRange<F>(m_numeric_storage);
    else
        static_assert(ygg::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind>
auto Builder<::tyr::planning::State<Kind>>::get_fluent_facts_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_fluent_facts() | std::views::transform([repository = &repository_](auto id) { return ygg::make_view(id, *repository); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto Builder<::tyr::planning::State<Kind>>::get_atoms_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_atoms<F>(repository_) | std::views::transform([repository = &repository_](auto id) { return ygg::make_view(id, *repository); });
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto Builder<::tyr::planning::State<Kind>>::get_fterm_values_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_fterm_values<F>()
           | std::views::transform([repository = &repository_](auto&& pair) { return std::make_pair(ygg::make_view(pair.first, *repository), pair.second); });
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::clear()
{
    ygg::clear(m_index);
    clear_unextended_part();
    clear_extended_part();
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::clear_unextended_part()
{
    m_fact_storage.clear();
    m_numeric_storage.clear();
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::clear_extended_part()
{
    m_atom_storage.clear();
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::assign_unextended_part(const Builder& other)
{
    m_fact_storage = other.m_fact_storage;
    m_numeric_storage = other.m_numeric_storage;
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::swap(Builder& other) noexcept
{
    using std::swap;
    swap(m_index, other.m_index);
    m_fact_storage.swap(other.m_fact_storage);
    m_atom_storage.swap(other.m_atom_storage);
    m_numeric_storage.swap(other.m_numeric_storage);
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::resize_fluent_facts(size_t size)
    requires std::same_as<Kind, ::tyr::GroundTag>
{
    m_fact_storage.resize(size);
}

template<::tyr::TaskKind Kind>
void Builder<::tyr::planning::State<Kind>>::resize_derived_atoms(size_t size)
    requires std::same_as<Kind, ::tyr::GroundTag>
{
    m_atom_storage.resize(size);
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
auto& Builder<::tyr::planning::State<Kind>>::get_atom_storage() noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return m_fact_storage;
    else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
        return m_atom_storage;
    else
        static_assert(ygg::dependent_false<F>::value);
}

template<::tyr::TaskKind Kind>
template<::tyr::formalism::FactKind F>
const auto& Builder<::tyr::planning::State<Kind>>::get_atom_storage() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return m_fact_storage;
    else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
        return m_atom_storage;
    else
        static_assert(ygg::dependent_false<F>::value);
}
template<::tyr::TaskKind Kind>
::tyr::planning::NumericUnpackedStorage<Kind>& Builder<::tyr::planning::State<Kind>>::get_numeric_variables() noexcept
{
    return m_numeric_storage;
}

template<::tyr::TaskKind Kind>
const ::tyr::planning::NumericUnpackedStorage<Kind>& Builder<::tyr::planning::State<Kind>>::get_numeric_variables() const noexcept
{
    return m_numeric_storage;
}

}

#endif
