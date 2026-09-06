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

#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <yggdrasil/containers/variant.hpp>

namespace tyr::datalog
{

template<formalism::RelationKind R>
class RuleInstance<GroundTag, R>
{
public:
    using Task = GroundTag;
    using Relation = R;
    using SourceRule = formalism::datalog::RuleView<GroundTag, R>;

    explicit RuleInstance(SourceRule rule) : m_rule(rule) {}

    SourceRule get_rule() const noexcept { return m_rule; }
    auto get_body() const noexcept { return m_rule.get_body(); }
    auto get_head() const noexcept { return m_rule.get_head(); }

    auto resolve(formalism::datalog::AtomView<GroundTag, formalism::FluentTag> atom) const noexcept { return atom.get_row(); }

    auto resolve(formalism::datalog::BooleanOperatorView<GroundTag> constraint) const noexcept { return constraint; }

    ResolvedNumericEffect resolve(formalism::datalog::NumericEffectView<GroundTag, formalism::FluentTag> effect) const noexcept
    {
        return { effect.get_operator(), effect.get_fterm().get_row(), effect.get_fexpr() };
    }

    template<typename Callback>
    bool for_each_resolved_metric_effect(Callback&& callback) const
    {
        for (const auto& metric_operator : m_rule.get_metric_effects())
            if (!ygg::visit([&](const auto metric_effect) { return callback(resolve(metric_effect)); }, metric_operator.get_variant()))
                return false;
        return true;
    }

    auto witness_key() const noexcept { return m_rule.get_row(); }

private:
    SourceRule m_rule;
};

}

#endif
