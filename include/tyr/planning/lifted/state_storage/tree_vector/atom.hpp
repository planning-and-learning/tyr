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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_VECTOR_ATOM_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_VECTOR_ATOM_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/state_storage/tree_vector/context.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <urd/repr/pools/tree_vector.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<>
struct AtomPackedStorage<LiftedTag, TreeVector> : urd::compared_by_identity<AtomPackedStorage<LiftedTag, TreeVector>>
{
    urd::repr::TreeHandle index;

    AtomPackedStorage() = default;
    explicit AtomPackedStorage(urd::repr::TreeHandle index_) : index(index_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&AtomPackedStorage::index> {}; }
};

template<bool ThreadSafe>
class AtomStorageBackend<LiftedTag, TreeVector, ThreadSafe>
{
public:
    using Context = StateStorageContext<LiftedTag, TreeVector, ThreadSafe>;
    using Unpacked = AtomUnpackedStorage<LiftedTag>;
    using Packed = AtomPackedStorage<LiftedTag, TreeVector>;

    explicit AtomStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    UintVectorSetT<Context>* m_uint_vectors;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
};

}

#endif
