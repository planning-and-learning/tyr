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

#ifndef TYR_CONTAINERS_HPP_
#define TYR_CONTAINERS_HPP_

#include <boost/dynamic_bitset.hpp>
#include <cstddef>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr
{

/// Bit `pos` of `bitset`, false past its end.
inline bool test(std::size_t pos, const boost::dynamic_bitset<>& bitset) noexcept
{
    if (pos >= bitset.size())
        return false;
    return bitset.test(pos);
}

/// Sets bit `pos` of `bitset` to `value`, growing it with false bits if needed.
inline void set(std::size_t pos, bool value, boost::dynamic_bitset<>& bitset)
{
    if (pos >= bitset.size())
        bitset.resize(pos + 1, false);
    bitset[pos] = value;
}

/// Element `pos` of `vec`, `default_value` past its end.
template<typename T>
T get(std::size_t pos, const urd::Store<urd::sequence<T>>& vec, const T& default_value) noexcept
{
    if (pos >= vec.size())
        return default_value;
    return vec[pos];
}

/// Sets element `pos` of `vec` to `value`, growing it with `default_value` if needed.
template<class T>
void set(std::size_t pos, const T& value, urd::Store<urd::sequence<T>>& vec, const T& default_value)
{
    if (pos >= vec.size())
        vec.resize(pos + 1, default_value);
    vec[pos] = value;
}

}

#endif
