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

#ifndef TYR_SRC_PLANNING_LIFTED_EFFECT_ATOM_INDEX_HPP_
#define TYR_SRC_PLANNING_LIFTED_EFFECT_ATOM_INDEX_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <optional>
#include <span>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

/// The atoms of the effect predicates of an action or ground task program, grouped by the binding of their action, which an atom's
/// first objects are and the row of the action's applicability atom identifies. An atom's objects are a completed binding of its
/// effect: the action binding followed by objects of the effect's variables for which the effect fires.
class EffectAtomIndex
{
public:
    using AppPredicateToActionMapping =
        ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, formalism::planning::ActionView<LiftedTag>>;
    using ConditionalEffectToPredicateMapping =
        ygg::UnorderedMap<std::pair<formalism::planning::ActionView<LiftedTag>, formalism::planning::ConditionalEffectView<LiftedTag>>,
                          formalism::datalog::PredicateView<formalism::FluentTag>>;

    /// Indexes the atoms of `facts`, interned in `repository`, of the program with the given mappings.
    EffectAtomIndex(const AppPredicateToActionMapping& predicate_to_action,
                    const ConditionalEffectToPredicateMapping& effect_to_predicate,
                    const datalog::PredicateFactSets<formalism::FluentTag>& facts,
                    const formalism::datalog::Repository& repository);

    /// Forgets the grouping after the facts changed; the storage is kept.
    void clear() noexcept;

    /// Calls `apply` with each completed binding of the action's effect in `binding`, which holds the action binding before and after;
    /// an effect without predicate fires once with its action.
    template<typename Apply>
    void for_each_effect_binding(formalism::planning::ActionView<LiftedTag> action,
                                 formalism::planning::ConditionalEffectView<LiftedTag> effect,
                                 ygg::IndexList<formalism::Object>& binding,
                                 Apply&& apply)
    {
        const auto atoms = get_atoms(action, effect, binding);
        if (!atoms)
        {
            apply();
            return;
        }
        const auto action_arity = binding.size();
        for (const auto atom : *atoms)
        {
            binding.clear();
            ygg::extend(ygg::make_view(atom, m_repository).get_objects(), binding);
            apply();
            binding.resize(action_arity);
        }
    }

private:
    using AtomIndex = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;

    /// The atoms of one effect predicate, grouped by the applicability row of their action binding: those of row `r` are
    /// `atoms[offsets[r], offsets[r + 1])`.
    struct Groups
    {
        bool current = false;
        std::vector<AtomIndex> atoms;
        std::vector<ygg::uint_t> offsets;
        std::vector<ygg::uint_t> atom_rows;
    };

    /// The atoms of the effect's predicate whose action binding is `binding`, or none if the effect has no predicate.
    std::optional<std::span<const AtomIndex>> get_atoms(formalism::planning::ActionView<LiftedTag> action,
                                                        formalism::planning::ConditionalEffectView<LiftedTag> effect,
                                                        std::span<const ygg::Index<formalism::Object>> binding);

    void rebuild(Groups& groups,
                 formalism::datalog::PredicateView<formalism::FluentTag> effect_predicate,
                 formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                 size_t action_arity);

    /// The row of the applicability atom of `binding`, if it holds.
    std::optional<ygg::uint_t> find_row(formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                                        std::span<const ygg::Index<formalism::Object>> binding);

    const ConditionalEffectToPredicateMapping& m_effect_to_predicate;
    const datalog::PredicateFactSets<formalism::FluentTag>& m_facts;
    const formalism::datalog::Repository& m_repository;
    ygg::UnorderedMap<formalism::planning::ActionView<LiftedTag>, formalism::datalog::PredicateView<formalism::FluentTag>> m_applicability_predicates;
    ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, Groups> m_groups;
    ygg::IndexList<formalism::Object> m_prefix;
    ygg::Data<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>> m_lookup;
};

}

#endif
