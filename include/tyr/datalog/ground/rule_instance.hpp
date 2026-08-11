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

#ifndef TYR_DATALOG_GROUND_RULE_INSTANCE_HPP_
#define TYR_DATALOG_GROUND_RULE_INSTANCE_HPP_

#include "tyr/datalog/ground/policies/annotation_types.hpp"
#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
class RuleInstance<GroundTag, R>
{
public:
    using Task = GroundTag;
    using Relation = R;
    using SourceRule = ::tyr::formalism::datalog::GroundRuleView<R>;

    explicit RuleInstance(SourceRule rule) : m_rule(rule) {}

    SourceRule get_rule() const noexcept { return m_rule; }
    auto get_body() const noexcept { return m_rule.get_body(); }
    auto get_head() const noexcept { return m_rule.get_head(); }
    auto get_metric_effects() const noexcept { return m_rule.get_metric_effects(); }

    auto resolve(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom) const noexcept { return atom.get_row(); }

    auto resolve(::tyr::formalism::datalog::GroundBooleanOperatorView constraint) const noexcept { return constraint; }

    ResolvedNumericEffect resolve(::tyr::formalism::datalog::GroundNumericEffectView<::tyr::formalism::FluentTag> effect) const noexcept
    {
        return { effect.get_operator(), effect.get_fterm().get_row(), effect.get_fexpr() };
    }

    auto witness_key() const noexcept { return m_rule; }

private:
    SourceRule m_rule;
};

}

#endif
