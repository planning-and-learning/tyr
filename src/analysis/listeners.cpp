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

#include "tyr/analysis/listeners.hpp"

#include "tyr/analysis/stratification.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <cista/containers/vector.h>
#include <gtl/phmap.hpp>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::analysis
{
namespace
{
template<formalism::RelationKind R>
void add_stratum_listeners(const TypedRuleStratum<R>& stratum, const formalism::datalog::Repository& context, TypedListenerStratum<R>& listeners)
{
    for (const auto rule : stratum)
    {
        const auto rule_view = ygg::make_view(rule, context);

        for (const auto literal : rule_view.get_body().template get_literals<formalism::FluentTag>())
            if (literal.get_polarity())
                listeners.predicates[literal.get_atom().get_predicate().get_index()].insert(rule);

        for (const auto term : formalism::datalog::collect_fluent_reads(rule_view))
            listeners.functions[term.get_function().get_index()].insert(rule);
    }
}
}

ListenerStrata compute_listeners(const RuleStrata& strata, const formalism::datalog::Repository& context)
{
    auto listeners = ListenerStrata();

    for (const auto& stratum : strata.data)
    {
        auto listeners_in_stratum = ListenerStratum {};

        add_stratum_listeners(stratum.template get<formalism::PredicateTag>(), context, listeners_in_stratum.template get<formalism::PredicateTag>());
        add_stratum_listeners(stratum.template get<formalism::FunctionTag>(), context, listeners_in_stratum.template get<formalism::FunctionTag>());

        listeners.data.push_back(std::move(listeners_in_stratum));
    }

    // std::cout << listeners.data << std::endl;

    return listeners;
}
}
