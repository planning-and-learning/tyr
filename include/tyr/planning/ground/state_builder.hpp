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
#include "tyr/formalism/planning/ground_atom_index.hpp"
#include "tyr/formalism/planning/ground_function_term_index.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_storage.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <limits>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{
template<::tyr::formalism::FactKind T>
struct GroundUnpackedAtomStorageType;

template<>
struct GroundUnpackedAtomStorageType<::tyr::formalism::FluentTag>
{
    using type = planning::FactUnpackedStorage<GroundTag>;
};

template<>
struct GroundUnpackedAtomStorageType<::tyr::formalism::DerivedTag>
{
    using type = planning::AtomUnpackedStorage<GroundTag>;
};

template<::tyr::formalism::FactKind T>
using GroundUnpackedAtomStorage = typename GroundUnpackedAtomStorageType<T>::type;

}

namespace tyr
{

template<>
struct Builder<planning::State<planning::GroundTag>>
{
public:
    using TaskType = planning::Task<planning::GroundTag>;

    Builder() = default;

    /**
     * UnpackedStateConcept
     */

    ygg::Index<planning::State<planning::GroundTag>> get_index() const;
    void set(ygg::Index<planning::State<planning::GroundTag>> index);

    ::tyr::formalism::planning::FDRValue get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const;
    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const;
    void set(ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact);

    ygg::float_t get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index) const;
    void set(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index, ygg::float_t value);

    bool test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index) const;
    void set(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index);
    bool test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) const;
    void set(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view);

    void clear();
    void clear_unextended_part();
    void clear_extended_part();
    void assign_unextended_part(const planning::UnpackedState<planning::GroundTag>& other);

    /**
     * For GroundTag
     */

    void resize_fluent_facts(size_t num_fluent_facts);
    void resize_derived_atoms(size_t num_derived_atoms);

    template<::tyr::formalism::FactKind T>
    planning::GroundUnpackedAtomStorage<T>& get_atoms() noexcept;
    template<::tyr::formalism::FactKind T>
    const planning::GroundUnpackedAtomStorage<T>& get_atoms() const noexcept;

    planning::NumericUnpackedStorage<planning::GroundTag>& get_numeric_variables() noexcept;
    const planning::NumericUnpackedStorage<planning::GroundTag>& get_numeric_variables() const noexcept;

private:
    ygg::Index<planning::State<planning::GroundTag>> m_index;

    planning::FactUnpackedStorage<planning::GroundTag> m_fact_storage;
    planning::AtomUnpackedStorage<planning::GroundTag> m_atom_storage;
    planning::NumericUnpackedStorage<planning::GroundTag> m_numeric_storage;
};

static_assert(planning::UnpackedStateConcept<planning::UnpackedState<planning::GroundTag>, planning::GroundTag>);

}

#endif
