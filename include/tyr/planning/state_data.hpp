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

#ifndef TYR_PLANNING_STATE_DATA_HPP_
#define TYR_PLANNING_STATE_DATA_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/ground/state_storage/vectors.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/state_storage/vectors.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"
#include "tyr/planning/state_storage/numeric.hpp"
#include "tyr/planning/state_storage/tree_bitset/atom.hpp"
#include "tyr/planning/state_storage/tree_bitset/fact.hpp"

#include <concepts>
#include <tuple>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace urd
{

/// A ground state's fluent facts, packed: a value held in its record, its handle in the storage of its policy.
template<>
struct data<::tyr::planning::FactStorage<::tyr::GroundTag>>
{
    ::tyr::planning::StateComponentRepository<::tyr::planning::FactStorage<::tyr::GroundTag>, ::tyr::planning::StateStoragePolicyTag>::Packed values;

    data() = default;

    data(::tyr::planning::StateComponentRepository<::tyr::planning::FactStorage<::tyr::GroundTag>, ::tyr::planning::StateStoragePolicyTag>::Packed values_) : values(values_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"values", &data::values>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::values> {}; }
};

/// A lifted state's fluent facts, packed: a value held in its record, its handle in the storage of its policy.
template<>
struct data<::tyr::planning::FactStorage<::tyr::LiftedTag>>
{
    ::tyr::planning::StateComponentRepository<::tyr::planning::FactStorage<::tyr::LiftedTag>, ::tyr::planning::StateStoragePolicyTag>::Packed indices;

    data() = default;

    data(::tyr::planning::StateComponentRepository<::tyr::planning::FactStorage<::tyr::LiftedTag>, ::tyr::planning::StateStoragePolicyTag>::Packed indices_) : indices(indices_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &data::indices>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::indices> {}; }
};

/// A state's derived atoms, packed: a value held in its record, its handle in the storage of its policy.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::AtomStorage<Kind>>
{
    typename ::tyr::planning::StateComponentRepository<::tyr::planning::AtomStorage<Kind>, ::tyr::planning::StateStoragePolicyTag>::Packed indices;

    data() = default;

    data(typename ::tyr::planning::StateComponentRepository<::tyr::planning::AtomStorage<Kind>, ::tyr::planning::StateStoragePolicyTag>::Packed indices_) : indices(indices_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &data::indices>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::indices> {}; }
};

/// A state's numeric values, packed: a value held in its record, its handle in the storage of its policy.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::NumericStorage<Kind>>
{
    typename ::tyr::planning::StateComponentRepository<::tyr::planning::NumericStorage<Kind>, ::tyr::planning::StateStoragePolicyTag>::Packed values;

    data() = default;

    data(typename ::tyr::planning::StateComponentRepository<::tyr::planning::NumericStorage<Kind>, ::tyr::planning::StateStoragePolicyTag>::Packed values_) : values(values_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"values", &data::values>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::values> {}; }
};

}  // namespace urd

namespace urd
{

/// A state as its repository stores it: its index and its storages, packed (each its storage record: a handle in the
/// storage of its policy). It is identified by its fluent facts and numeric values, by which the repository deduplicates
/// states; its derived atoms follow from them. Its builder holds the storages unpacked (state_builder.hpp).
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::State<Kind>>
{
    urd::index<::tyr::planning::State<Kind>> index;
    urd::data<::tyr::planning::FactStorage<Kind>> fact_storage;
    urd::data<::tyr::planning::AtomStorage<Kind>> atom_storage;
    urd::data<::tyr::planning::NumericStorage<Kind>> numeric_storage;

    data() = default;

    data(urd::data<::tyr::planning::FactStorage<Kind>> fact_storage_,
         urd::data<::tyr::planning::AtomStorage<Kind>> atom_storage_,
         urd::data<::tyr::planning::NumericStorage<Kind>> numeric_storage_) :
        index(),
        fact_storage(fact_storage_),
        atom_storage(atom_storage_),
        numeric_storage(numeric_storage_)
    {
    }

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"fact_storage", &data::fact_storage>,
                           urd::field<"atom_storage", &data::atom_storage>,
                           urd::field<"numeric_storage", &data::numeric_storage>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::fact_storage, &data::numeric_storage> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::fact_storage, &data::atom_storage, &data::numeric_storage> {}; }
};

}  // namespace urd

namespace tyr::planning
{
template<TaskKind Kind>
bool is_canonical(const urd::data<State<Kind>>&) noexcept
{ return true; }

template<TaskKind Kind>
void canonicalize(urd::data<State<Kind>>&) noexcept
{
}
}

#endif
