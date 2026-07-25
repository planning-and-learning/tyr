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

#include "tyr/datalog/policies/annotation_types.hpp"

#include "tyr/datalog/ground/policies/annotation_types.hpp"
#include "tyr/datalog/lifted/policies/annotation_types.hpp"

#include <algorithm>
#include <utility>

namespace tyr::datalog
{

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
WitnessAnnotation<Kind, R>::WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key_, Cost cost_) : rule_key(rule_key_), metric(), cost(cost_)
{
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
WitnessAnnotation<Kind, R>::WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key_, Metric metric_, Cost cost_) :
    rule_key(rule_key_),
    metric(metric_),
    cost(cost_)
{
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
WitnessAnnotation<Kind, R>::WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key_, Metric metric_, Cost cost_, NumericSupports numeric_supports_) :
    rule_key(rule_key_),
    metric(metric_),
    cost(cost_),
    numeric_supports(std::move(numeric_supports_))
{
    std::sort(numeric_supports.begin(), numeric_supports.end());
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
WitnessAnnotation<Kind, R>::WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key_,
                                              Metric metric_,
                                              Cost cost_,
                                              std::span<const NumericSupport<Kind>> numeric_supports_) :
    WitnessAnnotation(rule_key_, metric_, cost_, NumericSupports(numeric_supports_.begin(), numeric_supports_.end()))
{
}

template struct WitnessAnnotation<GroundTag, ::tyr::formalism::PredicateTag>;
template struct WitnessAnnotation<GroundTag, ::tyr::formalism::FunctionTag>;
template struct WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>;
template struct WitnessAnnotation<LiftedTag, ::tyr::formalism::FunctionTag>;

}
