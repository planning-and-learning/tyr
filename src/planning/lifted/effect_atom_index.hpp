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
#include "tyr/planning/lifted/programs/translation_context.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"

#include <optional>
#include <ranges>
#include <span>
#include <urd/repr/maps/dense_multimap.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// The atoms of the effect predicates of an action or ground task program, grouped by the binding of their action, which an atom's
/// first objects are and the row of the action's applicability atom identifies. An atom's objects are a completed binding of its
/// effect: the action binding followed by objects of the effect's variables for which the effect fires.
class EffectAtomIndex
{
public:

    /// Indexes the atoms of `facts`, interned in `repository`, of the program with the given mappings.
    EffectAtomIndex(const AppPredicateToActionMapping& predicate_to_action,
                    const ConditionalEffectToPredicateMapping& effect_to_predicate,
                    const datalog::TaggedFactSets<formalism::FluentTag>& facts,
                    const formalism::datalog::Repository& repository);

    /// Forgets the grouping after the facts changed; the storage is kept.
    void clear() noexcept;

    /// Calls `apply` with each completed binding of the action's effect in `binding`, which holds the action binding before and after;
    /// an effect without predicate fires once with its action.
    template<typename Apply>
    void for_each_effect_binding(formalism::planning::ActionView<LiftedTag> action,
                                 formalism::planning::ConditionalEffectView<LiftedTag> effect,
                                 urd::flat_vector<urd::index<formalism::Object>>& binding,
                                 Apply apply)
    {
        const auto atoms = get_atoms(action, effect, binding);
        if (!atoms)
        {
            apply();
            return;
        }
        const auto action_arity = binding.size();
        for (const auto atom : std::ranges::subrange(atoms->first, atoms->second) | std::views::values)
        {
            binding.clear();
            for (const auto object : urd::resolve(atom, *m_repository).template get<urd::index<formalism::Object>>())
                binding.push_back(object);
            apply();
            binding.resize(action_arity);
        }
    }

private:
    using AtomIndex = formalism::datalog::PredicateRowIndex<formalism::FluentTag>;

    /// The atoms of one effect predicate, grouped by the applicability row of their action binding, refilled in the
    /// storage held: clear keeps each row's.
    using Atoms = urd::Store<urd::map<ygg::uint_t, AtomIndex>, urd::multi, urd::dense>;
    struct Groups
    {
        bool current = false;
        Atoms atoms;
    };

    /// The atoms of the effect's predicate whose action binding is `binding`, as the entries of its row (an
    /// equal_range), or none if the effect has no predicate.
    std::optional<std::pair<Atoms::const_iterator, Atoms::const_iterator>> get_atoms(formalism::planning::ActionView<LiftedTag> action,
                                                        formalism::planning::ConditionalEffectView<LiftedTag> effect,
                                                        std::span<const urd::index<formalism::Object>> binding);

    void rebuild(Groups& groups,
                 formalism::datalog::PredicateView<formalism::FluentTag> effect_predicate,
                 formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                 size_t action_arity);

    /// The row of the applicability atom of `binding`, if it holds.
    std::optional<ygg::uint_t> find_row(formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                                        std::span<const urd::index<formalism::Object>> binding);

    const ConditionalEffectToPredicateMapping* m_effect_to_predicate;
    const datalog::TaggedFactSets<formalism::FluentTag>* m_facts;
    const formalism::datalog::Repository* m_repository;
    urd::Store<urd::map<formalism::planning::ActionView<LiftedTag>, formalism::datalog::PredicateView<formalism::FluentTag>>> m_applicability_predicates;
    urd::Store<urd::map<formalism::datalog::PredicateView<formalism::FluentTag>, Groups>> m_groups;
    urd::flat_vector<urd::index<formalism::Object>> m_prefix;
    urd::data<ygg::Row<formalism::Predicate<formalism::FluentTag>>> m_lookup;
};

}

#endif
