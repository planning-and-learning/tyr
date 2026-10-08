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

#ifndef TYR_PLANNING_GROUND_STATE_BUILDER_HPP_
#define TYR_PLANNING_GROUND_STATE_BUILDER_HPP_

#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"
#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/formalism/planning/function_term_index.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/ground/state_storage/iterators.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_storage.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <limits>
#include <ranges>
#include <tuple>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{
template<formalism::FactKind T>
struct GroundUnpackedAtomStorageType;

template<>
struct GroundUnpackedAtomStorageType<formalism::FluentTag>
{
    using type = planning::FactUnpackedStorage<GroundTag>;
};

template<>
struct GroundUnpackedAtomStorageType<formalism::DerivedTag>
{
    using type = planning::AtomUnpackedStorage<GroundTag>;
};

template<formalism::FactKind T>
using GroundUnpackedAtomStorage = typename GroundUnpackedAtomStorageType<T>::type;

}

namespace ygg
{

template<>
struct Builder<::tyr::planning::State<::tyr::GroundTag>>
{
public:
    using StateType = ::tyr::planning::State<::tyr::GroundTag>;
    using TaskType = ::tyr::planning::Task<::tyr::GroundTag>;

    Builder() = default;

    /**
     * StateBuilderConcept
     */

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

    ::tyr::planning::FDRFactRange<::tyr::GroundTag, ::tyr::formalism::FluentTag> get_fluent_facts() const noexcept;
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

    /**
     * For GroundTag
     */

    void resize_fluent_facts(size_t num_fluent_facts);
    void resize_derived_atoms(size_t num_derived_atoms);

    template<::tyr::formalism::FactKind T>
    ::tyr::planning::GroundUnpackedAtomStorage<T>& get_atom_storage() noexcept;
    template<::tyr::formalism::FactKind T>
    const ::tyr::planning::GroundUnpackedAtomStorage<T>& get_atom_storage() const noexcept;

    ::tyr::planning::NumericUnpackedStorage<::tyr::GroundTag>& get_numeric_variables() noexcept;
    const ::tyr::planning::NumericUnpackedStorage<::tyr::GroundTag>& get_numeric_variables() const noexcept;

    auto identifying_members() const noexcept { return std::tie(m_fact_storage, m_numeric_storage); }

private:
    ygg::Index<StateType> m_index;

    ::tyr::planning::FactUnpackedStorage<::tyr::GroundTag> m_fact_storage;
    ::tyr::planning::AtomUnpackedStorage<::tyr::GroundTag> m_atom_storage;
    ::tyr::planning::NumericUnpackedStorage<::tyr::GroundTag> m_numeric_storage;
};

static_assert(::tyr::planning::StateBuilderConcept<Builder<::tyr::planning::State<::tyr::GroundTag>>, ::tyr::GroundTag>);

template<::tyr::formalism::FactKind F>
auto Builder<::tyr::planning::State<::tyr::GroundTag>>::get_atoms(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return get_fluent_facts()
               | std::views::transform([repository = &repository_](auto fact) { return *ygg::make_view(fact, *repository).get_atom_index(); });
    else if constexpr (std::same_as<F, ::tyr::formalism::DerivedTag>)
        return ::tyr::planning::AtomRange<F>(m_atom_storage);
    else
        static_assert(ygg::dependent_false<F>::value);
}

template<::tyr::formalism::FactKind F>
::tyr::planning::FunctionTermValueRange<F> Builder<::tyr::planning::State<::tyr::GroundTag>>::get_fterm_values() const noexcept
{
    if constexpr (std::same_as<F, ::tyr::formalism::FluentTag>)
        return ::tyr::planning::FunctionTermValueRange<F>(m_numeric_storage);
    else
        static_assert(ygg::dependent_false<F>::value);
}

inline auto Builder<::tyr::planning::State<::tyr::GroundTag>>::get_fluent_facts_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_fluent_facts() | std::views::transform([repository = &repository_](auto id) { return ygg::make_view(id, *repository); });
}

template<::tyr::formalism::FactKind F>
auto Builder<::tyr::planning::State<::tyr::GroundTag>>::get_atoms_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_atoms<F>(repository_) | std::views::transform([repository = &repository_](auto id) { return ygg::make_view(id, *repository); });
}

template<::tyr::formalism::FactKind F>
auto Builder<::tyr::planning::State<::tyr::GroundTag>>::get_fterm_values_view(const ::tyr::formalism::planning::Repository& repository_) const noexcept
{
    return get_fterm_values<F>()
           | std::views::transform([repository = &repository_](auto&& pair) { return std::make_pair(ygg::make_view(pair.first, *repository), pair.second); });
}

}

#endif
