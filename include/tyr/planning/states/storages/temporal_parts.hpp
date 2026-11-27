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

#ifndef TYR_PLANNING_STATES_STORAGES_TEMPORAL_PARTS_HPP_
#define TYR_PLANNING_STATES_STORAGES_TEMPORAL_PARTS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/states/storages/temporal_network.hpp"

#include <bit>
#include <compare>
#include <cstdint>
#include <fmt/format.h>
#include <span>
#include <string_view>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/format.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace tyr::planning
{

/// The key of an action binding (a row of its action).
using ActionRowKey = urd::key<formalism::Action<LiftedTag>, ygg::Row<formalism::Action<LiftedTag>>>;

/// A durative action started and not ended: its binding, the timepoint of its start, and the bounds of its duration
/// (evaluated at its start: ?duration's).
struct OpenAction
{
    ActionRowKey action;
    std::uint32_t start = 0;
    ygg::float_t min_duration = 0;
    ygg::float_t max_duration = TemporalNetwork::infinity;

    friend bool operator==(const OpenAction&, const OpenAction&) = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"action", &OpenAction::action>,
                           urd::field<"start", &OpenAction::start>,
                           urd::field<"min_duration", &OpenAction::min_duration>,
                           urd::field<"max_duration", &OpenAction::max_duration>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// What a frontier timepoint is to a state variable (an FDR variable, or a fluent function term): its last writer, a reader
/// since (a condition that read it), or the over-all condition of an open action that reads it until the action ends. A
/// new happening is ordered after them as it interferes with them.
enum class TemporalRoleKind : std::uint8_t
{
    WRITER,
    READER,
    OVER_ALL,
    NUMERIC_WRITER,
    NUMERIC_READER,
};

/// A role's name (its serialization, as the formalism's enums: to_string).
constexpr std::string_view to_string(TemporalRoleKind kind)
{
    switch (kind)
    {
        case TemporalRoleKind::WRITER:
            return "writer";
        case TemporalRoleKind::READER:
            return "reader";
        case TemporalRoleKind::OVER_ALL:
            return "over_all";
        case TemporalRoleKind::NUMERIC_WRITER:
            return "numeric_writer";
        case TemporalRoleKind::NUMERIC_READER:
            return "numeric_reader";
    }
    return "";
}

struct TemporalRole
{
    TemporalRoleKind kind = TemporalRoleKind::WRITER;
    std::uint32_t variable = 0;
    std::uint32_t timepoint = 0;

    friend bool operator==(const TemporalRole&, const TemporalRole&) = default;
    friend auto operator<=>(const TemporalRole&, const TemporalRole&) = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"kind", &TemporalRole::kind>, urd::field<"variable", &TemporalRole::variable>, urd::field<"timepoint", &TemporalRole::timepoint>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// A state's temporal part, unpacked (its builder's form): its open durative actions, the roles of its frontier
/// timepoints, how many timed initial literals have happened, and the minimal temporal network over the frontier (the
/// origin first). Canonical once canonicalize() ordered it: two states with equal facts and numeric values are the same
/// state iff these are equal.
struct TemporalParts
{
    urd::Store<urd::sequence<OpenAction>> open;
    urd::Store<urd::sequence<TemporalRole>> roles;
    std::uint32_t til_progress = 0;
    TemporalNetwork network;

    void clear()
    {
        open.clear();
        roles.clear();
        til_progress = 0;
        network.clear();
    }

    friend bool operator==(const TemporalParts& lhs, const TemporalParts& rhs)
    {
        return lhs.til_progress == rhs.til_progress && lhs.open == rhs.open && lhs.roles == rhs.roles && lhs.network.size() == rhs.network.size()
               && std::ranges::equal(lhs.network.distances(), rhs.network.distances());
    }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"open", &TemporalParts::open>,
                           urd::field<"roles", &TemporalParts::roles>,
                           urd::field<"til_progress", &TemporalParts::til_progress>,
                           urd::field<"network", &TemporalParts::network>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// A state's temporal part as its record holds it: one canonical sequence of words (the counts, the open actions, the
/// roles, the distances), field by field, so that equal parts pack to equal words.
struct TemporalWords
{
    urd::flat_vector<std::uint64_t> words;

    /// Packs built parts (a builder's, written into its record: urd's AssignsBuilt).
    TemporalWords& operator=(const TemporalParts& parts);

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"words", &TemporalWords::words>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&TemporalWords::words> {}; }
};

/// Writes the packed words of `parts` into `words` (appended).
template<typename Words>
void pack_temporal(const TemporalParts& parts, Words& words)
{
    const auto bits = [](ygg::float_t value) { return std::bit_cast<std::uint64_t>(ygg::FloatTolerance<ygg::float_t>::canonicalize(value)); };
    words.push_back(parts.open.size());
    words.push_back(parts.roles.size());
    words.push_back(parts.til_progress);
    words.push_back(parts.network.size());
    for (const auto& open : parts.open)
    {
        words.push_back(open.action.get<formalism::Action<LiftedTag>>().value);
        words.push_back(open.action.get<ygg::Row<formalism::Action<LiftedTag>>>().value);
        words.push_back(open.start);
        words.push_back(bits(open.min_duration));
        words.push_back(bits(open.max_duration));
    }
    for (const auto& role : parts.roles)
    {
        words.push_back(static_cast<std::uint64_t>(role.kind));
        words.push_back(role.variable);
        words.push_back(role.timepoint);
    }
    for (const auto distance : parts.network.distances())
        words.push_back(bits(distance));
}

/// Reads packed words back into `parts`.
void unpack_temporal(std::span<const std::uint64_t> words, TemporalParts& parts);

inline TemporalWords& TemporalWords::operator=(const TemporalParts& parts)
{
    words.clear();
    pack_temporal(parts, words);
    return *this;
}

}

/// A state's temporal part: its open actions (action@start [min, max]), its roles (kind variable@timepoint), the timed
/// initial literals that happened, and each frontier timepoint's window [earliest, latest].
template<>
struct fmt::formatter<tyr::planning::TemporalParts, char>
{
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    fmt::format_context::iterator format(const tyr::planning::TemporalParts& value, fmt::format_context& ctx) const
    {
        auto out = fmt::format_to(ctx.out(), "(temporal :open (");
        for (const auto& open : value.open)
            out = fmt::format_to(out, " {}@{} [{}, {}]", open.action, open.start, open.min_duration, open.max_duration);
        out = fmt::format_to(out, ") :roles (");
        for (const auto& role : value.roles)
            out = fmt::format_to(out, " {}:{}@{}", static_cast<int>(role.kind), role.variable, role.timepoint);
        out = fmt::format_to(out, ") :til {} :windows (", value.til_progress);
        for (std::size_t t = 0; t < value.network.size(); ++t)
            out = fmt::format_to(out, " [{}, {}]", value.network.earliest(t), value.network.latest(t));
        return fmt::format_to(out, "))");
    }
};

#endif
