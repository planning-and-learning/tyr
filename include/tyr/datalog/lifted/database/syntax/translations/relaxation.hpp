/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SYNTAX_TRANSLATIONS_RELAXATION_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_TRANSLATIONS_RELAXATION_HPP_

#include "tyr/datalog/lifted/database/syntax/operator_builder.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database
{

/// Relaxations of queries over inputs of which only some are known.
struct Relaxation
{
    urd::Store<urd::sequence<std::optional<QueryView<>>>> queries;  ///< Per root; nullopt if nothing is constrained.
    urd::Store<urd::sequence<size_t>> slots;                        ///< The original input slot of each relaxed input, shared by all roots.
};

namespace detail
{

/// Which side of R a relaxation bounds: the over-approximation R̂ contains every possible answer, the
/// under-approximation Ř only certain ones (see relax() below). A subtrahend is bounded on the other side.
enum class Polarity : std::uint8_t
{
    over,
    under
};

constexpr Polarity flip(Polarity polarity) noexcept { return polarity == Polarity::over ? Polarity::under : Polarity::over; }

template<std::predicate<size_t> Known>
class Relaxer
{
    using Labels = urd::Store<urd::sequence<urd::index<ygg::Column>>>;

public:
    Relaxer(QueryRepository& repository, QueryBuilder& builder, Known known) : m_build(repository, builder), m_known(std::move(known)) {}

    /// The relaxation of `query` on the side `polarity`, restricted to the needed labels `needed` (see relax() below);
    /// nullopt means nothing is known: ⊤ when over, ∅ when under.
    std::optional<QueryView<>> relax(QueryView<> query, const Labels& needed, Polarity polarity)
    {
        auto result = std::visit([&]<typename Tag>(QueryView<Tag> concrete) { return relax(concrete, needed, polarity); }, query.get_variant());
        // Labels nobody needs are eliminated here, in the order of `needed`; with none needed, only the emptiness remains.
        if (result)
            result = m_build.project(*result, intersection(needed, labels(*result)));
        return result;
    }

    urd::Store<urd::sequence<size_t>> slots() && { return std::move(m_slots); }

    static Labels labels(QueryView<> query)
    {
        auto result = Labels();
        for (const auto& column : columns_of(query))
            result.push_back(column.label);
        return result;
    }

private:
    OperatorBuilder m_build;
    Known m_known;
    urd::Store<urd::sequence<size_t>> m_slots;  ///< The original slot of each relaxed input.

    static bool contains(const Labels& labels, urd::index<ygg::Column> label) { return std::ranges::find(labels, label) != labels.end(); }

    /// The labels of `left_operand` also in `right_operand`, in the order of `left_operand`.
    static Labels intersection(const Labels& left_operand, const Labels& right_operand)
    {
        auto result = Labels();
        for (const auto label : left_operand)
            if (contains(right_operand, label))
                result.push_back(label);
        return result;
    }

    /// `left_operand` followed by the labels of `right_operand` not in it.
    static Labels united(Labels left_operand, const Labels& right_operand)
    {
        for (const auto label : right_operand)
            if (!contains(left_operand, label))
                left_operand.push_back(label);
        return left_operand;
    }

    static bool subset(const Labels& left_operand, const Labels& right_operand)
    {
        return std::ranges::all_of(left_operand, [&](urd::index<ygg::Column> label) { return contains(right_operand, label); });
    }

    /// The relaxed input of a known slot.
    size_t renumbered(size_t slot)
    {
        // ponytail: linear in the number of inputs, which edge queries keep small.
        const auto found = std::ranges::find(m_slots, slot);
        if (found != m_slots.end())
            return static_cast<size_t>(found - m_slots.begin());
        m_slots.push_back(slot);
        return m_slots.size() - 1;
    }

    std::optional<QueryView<>> input(QueryView<QueryInputTag> concrete)
    {
        const auto slot = concrete.get_input_slot();
        if (!m_known(slot))
            return std::nullopt;
        const auto columns = concrete.columns();
        return m_build.template make<QueryInputTag>(
            [&](auto& data)
            {
                data.input_slot = renumbered(slot);
                data.columns.assign(columns.begin(), columns.end());
            });
    }

    /// A conjunction (⋈, ⋉) of relaxed operands: an unknown side is its identity ⊤ when over and absorbs it as ∅ when
    /// under.
    template<typename Tag>
    std::optional<QueryView<>> conjoin(std::optional<QueryView<>> left_operand, std::optional<QueryView<>> right_operand, Polarity polarity)
    {
        if (!left_operand || !right_operand)
            return polarity == Polarity::over ? (left_operand ? left_operand : right_operand) : std::nullopt;
        return m_build.template binary<Tag>(*left_operand, *right_operand);
    }

    /// A union of relaxed operands: an unknown side absorbs it as ⊤ when over and is its identity ∅ when under. Only
    /// labels both sides constrain stay constrained.
    std::optional<QueryView<>> disjoin(std::optional<QueryView<>> left_operand, std::optional<QueryView<>> right_operand, Polarity polarity)
    {
        if (!left_operand || !right_operand)
            return polarity == Polarity::over ? std::nullopt : (left_operand ? left_operand : right_operand);
        const auto common = intersection(labels(*left_operand), labels(*right_operand));
        return m_build.template binary<QueryUnionTag>(m_build.project(*left_operand, common), m_build.project(*right_operand, common));
    }

    template<typename Tag>
    std::optional<QueryView<>> relax(QueryView<Tag> concrete, const Labels& needed, Polarity polarity)
    {
        if constexpr (std::same_as<Tag, QueryInputTag>)
            return input(concrete);
        else if constexpr (std::same_as<Tag, QueryEmptyTag>)
            return m_build.empty(concrete.columns());
        else if constexpr (std::same_as<Tag, QueryJoinTag> || std::same_as<Tag, QueryGenericJoinTag>)
        {
            auto conjuncts = urd::Store<urd::sequence<QueryView<>>>();
            if constexpr (std::same_as<Tag, QueryJoinTag>)
                conjuncts = { concrete.get_left_operand(), concrete.get_right_operand() };
            else
                for (const auto input : concrete.get_inputs())
                    conjuncts.push_back(input);
            // A conjunct needs its labels that are needed above or shared with another conjunct (early elimination).
            auto result = std::optional<QueryView<>>();
            for (size_t k = 0; k < conjuncts.size(); ++k)
            {
                auto others = needed;
                for (size_t j = 0; j < conjuncts.size(); ++j)
                    if (j != k)
                        others = united(std::move(others), labels(conjuncts[j]));
                const auto relaxed = relax(conjuncts[k], intersection(labels(conjuncts[k]), others), polarity);
                result = k == 0 ? relaxed : conjoin<QueryJoinTag>(result, relaxed, polarity);
            }
            return result;
        }
        else if constexpr (std::same_as<Tag, QueryProjectTag>)
            return relax(concrete.get_operand(), needed, polarity);
        else if constexpr (std::same_as<Tag, QueryRenameTag>)
        {
            const auto before = columns_of(concrete.get_operand());
            const auto after = concrete.columns();
            auto inner = Labels();
            for (const auto label : needed)
                inner.push_back(before[static_cast<size_t>(std::ranges::find(after, label, &ygg::ColumnLayout::label) - after.begin())].label);
            const auto arg = relax(concrete.get_operand(), inner, polarity);
            if (!arg)
                return std::nullopt;
            // The relaxed columns keep their labels' positions in the argument's schema.
            auto renamed = Labels();
            for (const auto& column : columns_of(*arg))
                renamed.push_back(after[static_cast<size_t>(std::ranges::find(before, column.label, &ygg::ColumnLayout::label) - before.begin())].label);
            return m_build.template relabel<QueryRenameTag>(*arg, renamed);
        }
        else if constexpr (std::same_as<Tag, QuerySelectEqualTag> || std::same_as<Tag, QuerySelectValueTag>)
        {
            const auto selected = [&]
            {
                if constexpr (std::same_as<Tag, QuerySelectEqualTag>)
                    return Labels { concrete.get_left_column(), concrete.get_right_column() };
                else
                    return Labels { concrete.get_column() };
            }();
            const auto arg = relax(concrete.get_operand(), united(needed, selected), polarity);
            if (!arg)
                return std::nullopt;
            // A selection on an unconstrained column is dropped.
            if (!subset(selected, labels(*arg)))
                return arg;
            return m_build.template make<Tag>(
                [&](auto& filtered)
                {
                    // The same selection over the relaxed operand.
                    urd::build_from(concrete, filtered, [&](QueryView<>) { return *arg; });
                });
        }
        else if constexpr (std::same_as<Tag, QueryUnionTag>)
            return disjoin(relax(concrete.get_left_operand(), needed, polarity), relax(concrete.get_right_operand(), needed, polarity), polarity);
        else if constexpr (std::same_as<Tag, QuerySemijoinTag>)
        {
            const auto shared = intersection(labels(concrete.get_left_operand()), labels(concrete.get_right_operand()));
            return conjoin<QuerySemijoinTag>(relax(concrete.get_left_operand(), united(needed, shared), polarity),
                                             relax(concrete.get_right_operand(), shared, polarity),
                                             polarity);
        }
        else if constexpr (std::same_as<Tag, QueryDifferenceTag> || std::same_as<Tag, QueryAntijoinTag>)
        {
            // A \ B is A ▷ B on all of A's labels. The subtrahend is bounded on the other side: B̂ ⊇ B when under,
            // B̌ ⊆ B when over.
            const auto shared = intersection(labels(concrete.get_left_operand()), labels(concrete.get_right_operand()));
            const auto left_operand = relax(concrete.get_left_operand(), polarity == Polarity::under ? united(needed, shared) : needed, polarity);
            const auto right_operand = relax(concrete.get_right_operand(), shared, flip(polarity));
            if (polarity == Polarity::under)
            {
                // B ⊆ B̂ × U: a row matching no row of B̂ on its labels matches none of B. Subtracting ⊤ leaves ∅.
                if (!left_operand || !right_operand)
                    return std::nullopt;
                return m_build.template binary<QueryAntijoinTag>(*left_operand, *right_operand);
            }
            // Kept only as a filter on needed labels the relaxed left operand constrains; filtering on labels eliminated above
            // would require all their values (relational division). Subtracting ∅ leaves the left operand.
            if (!left_operand || !right_operand || !subset(shared, needed) || !subset(shared, labels(*left_operand)))
                return left_operand;
            if constexpr (std::same_as<Tag, QueryDifferenceTag>)
                return m_build.template binary<Tag>(*left_operand, m_build.project(*right_operand, labels(*left_operand)));
            else
                return m_build.template binary<Tag>(*left_operand, *right_operand);
        }
        else
        {
            static_assert(std::same_as<Tag, QueryDistanceTag>, "unhandled query operator");
            throw std::invalid_argument("Relaxation: distances are not first-order queries.");
        }
    }
};

}  // namespace detail

/// The relaxations Q̂ of queries Q whose inputs not marked by `known(slot)` may hold any rows.
///
/// Contract: Q̂ reads only the known inputs, its schema S' is a subset of Q's schema, and for all inputs I
///     Q(I) ⊆ Q̂(I) × U^(schema(Q) \ S'),
/// where U is the universe of values. Labels outside S' are unconstrained; nullopt (written ⊤ below) means no label
/// is constrained. All roots share one numbering of the known inputs, so they can be compiled together.
///
/// Subtrahends need the opposite bound, so the translation has two polarities: the over-approximation R̂ above and an
/// under-approximation Ř, which reads only the known inputs, has R's schema, and satisfies Ř(I) ⊆ R(I) for all inputs
/// I; nullopt (written ∅ below) means nothing is certain. Negation flips the polarity.
///
/// Translation, by induction on R, restricted to the needed labels N (initially all of the root's labels; pass π_L(Q)
/// to need only L). Every result is finally projected onto its labels in N, so a subquery with nothing needed becomes
/// nullary, a test of its non-emptiness; an under-approximation keeps all of N. S_R is R's schema, S'_R that of R̂, and
/// shared = S_A ∩ S_B:
///
///     R                    R̂ under N                                     Ř under N
///     -------------------  --------------------------------------------  -------------------------------------
///     input s              s if known(s), otherwise ⊤                    s if known(s), otherwise ∅
///     empty                empty                                         empty
///     A ⋈ B, generic join  ⋈ of the conjuncts, each under its labels     the same ⋈; ∅ if a conjunct is ∅
///                          needed in N or by another conjunct; a ⊤
///                          conjunct drops out (the identity of ⋈)
///     π_L(A)               Â_N                                           Ǎ_N
///     ρ(A)                 Â_ρ⁻¹(N), renamed as in ρ; ⊤ if Â = ⊤          the same with Ǎ; ∅ if Ǎ = ∅
///     σ(A)                 σ(Â_(N ∪ selected)) if Â constrains the       σ(Ǎ_(N ∪ selected)); ∅ if Ǎ = ∅
///                          selected columns, otherwise Â_(N ∪ selected)
///     A ∪ B                π_C(Â_N) ∪ π_C(B̂_N) with C = S'_A ∩ S'_B,      Ǎ_N ∪ B̌_N; a ∅ side drops out
///                          possibly nullary; ⊤ if a side is ⊤
///     A ⋉ B                Â_(N ∪ shared) ⋉ B̂_shared; a ⊤ side drops    Ǎ_(N ∪ shared) ⋉ B̌_shared; ∅ if a side is ∅
///                          out (the identity of ⋉)
///     A \ B                Â_N \ B̌_shared if N ⊇ S_A = S'_A and B̌ ≠ ∅,  Ǎ_(N ∪ shared) ▷ B̂_shared; ∅ if B̂ = ⊤
///                          otherwise Â_N
///     A ▷ B                Â_N ▷ B̌_shared if shared ⊆ N ∩ S'_A and       Ǎ_(N ∪ shared) ▷ B̂_shared; ∅ if B̂ = ⊤
///                          B̌ ≠ ∅, otherwise Â_N
///     distance             not first-order; rejected
///
/// Each rule keeps the contract: ⊤ is the identity of ⋈ and ⋉ and absorbs ∪, ∅ the reverse; eliminating a label no
/// one needs is exact (π_L(A ⋈ B) = π_L(A) ⋈ π_∅(B) when B shares nothing with A or L); a union may only stay
/// constrained where both sides are; a selection is checkable only on constrained columns. Subtracting B̌ ⊆ B removes
/// no row of A \ B, if done on needed labels the relaxed left operand constrains; and as B ⊆ B̂ × U^(S_B \ S'_B), a row
/// matching no row of B̂ on S'_B matches none of B, so Ǎ ▷ B̂ ⊆ A \ B. With every input known and no difference or
/// antijoin below a projection that eliminates labels it shares, R̂ = Ř = R.
///
/// Literature: R̂ over-approximates the possible answers of R and Ř under-approximates its certain answers in the
/// possible-world semantics of incomplete databases, the unknown inputs being arbitrary relations (Imieliński &
/// Lipski, "Incomplete Information in Relational Databases", JACM 1984; Abiteboul, Hull & Vianu, "Foundations of
/// Databases", 1995, ch. 19). Both are computed by query rewriting, with negation exchanging them, (A − B)? = A? − B⁺,
/// as in Guagliardo & Libkin's translations Q? and Q⁺ ("Making SQL Queries Correct on Incomplete Databases: A
/// Feasibility Study", PODS 2016). In heuristic search, the same construction relaxes a problem by dropping the
/// constraints it cannot evaluate in advance (Pearl, "Heuristics", 1984, ch. 4); relaxing π_L(Q) is the projection of
/// pattern databases, which drops conditions on variables outside the pattern (Edelkamp, "Planning with Pattern
/// Databases", ECP 2001; Haslum, Botea, Helmert, Bonet & Koenig, "Domain-Independent Construction of Pattern Database
/// Heuristics for Cost-Optimal Planning", AAAI 2007), while antijoins within the pattern stay as in constrained
/// pattern databases (Haslum, Bonet & Geffner, "New Admissible Heuristics for Domain-Independent Planning", AAAI
/// 2005). Labels are eliminated as early as possible, as in early quantification (Burch, Clarke & Long, "Symbolic
/// Model Checking with Partitioned Transition Relations", VLSI 1991) and variable elimination (Dechter, "Bucket
/// Elimination", AIJ 1999; Abo Khamis, Ngo & Rudra, "FAQ: Questions Asked Frequently", PODS 2016); an exact antijoin
/// on eliminated labels would need relational division (Codd, "Relational Completeness of Data Base Sublanguages",
/// 1972). The treatment of eliminated labels in the rules above is our construction for yggdrasil's operators.
///
/// The relaxed queries are built in `repository`, whose interning keeps shared subqueries shared. A subquery shared
/// by several paths is relaxed once per path: edge queries are small, and this runs once per feature.
template<std::predicate<size_t> Known>
Relaxation relax(std::span<const QueryView<>> roots, Known&& known, QueryRepository& repository, QueryBuilder& builder)
{
    auto relaxer = detail::Relaxer<std::remove_cvref_t<Known>>(repository, builder, std::forward<Known>(known));
    auto queries = urd::Store<urd::sequence<std::optional<QueryView<>>>>();
    for (const auto root : roots)
        queries.push_back(relaxer.relax(root, relaxer.labels(root), detail::Polarity::over));
    return Relaxation { std::move(queries), std::move(relaxer).slots() };
}

/// The relaxation of one query; see the overload for several roots.
template<std::predicate<size_t> Known>
Relaxation relax(QueryView<> query, Known&& known, QueryRepository& repository, QueryBuilder& builder)
{ return relax(std::span<const QueryView<>>(&query, 1), std::forward<Known>(known), repository, builder); }

}  // namespace tyr::datalog::database

#endif
