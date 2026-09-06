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

#ifndef TYR_DATALOG_LIFTED_RULE_INSTANCE_HPP_
#define TYR_DATALOG_LIFTED_RULE_INSTANCE_HPP_

#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/datalog/grounder.hpp"

#include <optional>
#include <yggdrasil/containers/variant.hpp>

namespace tyr::datalog
{

template<formalism::RelationKind R>
class RuleInstance<LiftedTag, R>
{
public:
    using Task = LiftedTag;
    using Relation = R;
    using SourceRule = formalism::datalog::RuleView<LiftedTag, R>;

    RuleInstance(const ConstRuleWorkspace<LiftedTag, R>& workspace, formalism::datalog::GrounderContext& grounder) :
        m_workspace(workspace),
        m_grounder(grounder)
    {
    }

    SourceRule get_rule() const noexcept { return m_workspace.get_rule(); }
    auto get_body() const noexcept { return get_rule().get_body(); }
    auto get_head() const noexcept { return get_rule().get_head(); }

    auto resolve(formalism::datalog::AtomView<LiftedTag, formalism::FluentTag> atom) const
    {
        return formalism::datalog::ground_binding(atom, m_grounder).first;
    }

    auto resolve(formalism::datalog::BooleanOperatorView<LiftedTag> constraint) const { return formalism::datalog::ground(constraint, m_grounder); }

    ResolvedNumericEffect resolve(formalism::datalog::NumericEffectView<LiftedTag, formalism::FluentTag> effect) const
    {
        return { effect.get_operator(),
                 formalism::datalog::ground_binding(effect.get_fterm(), m_grounder).first,
                 formalism::datalog::ground(effect.get_fexpr(), m_grounder) };
    }

    ResolvedNumericEffect resolve(formalism::datalog::NumericEffectView<GroundTag, formalism::FluentTag> effect) const noexcept
    {
        return { effect.get_operator(), effect.get_fterm().get_row(), effect.get_fexpr() };
    }

    template<typename Callback>
    bool for_each_resolved_metric_effect(Callback&& callback) const
    {
        for (const auto& metric_operator : m_workspace.get_nullary_effects())
            if (!ygg::visit([&](const auto metric_effect) { return callback(resolve(metric_effect)); }, metric_operator.get_variant()))
                return false;
        for (const auto& metric_operator : m_workspace.get_lifted_effects())
            if (!ygg::visit([&](const auto metric_effect) { return callback(resolve(metric_effect)); }, metric_operator.get_variant()))
                return false;
        return true;
    }

    auto witness_key() const
    {
        if (!m_witness_key)
            m_witness_key = formalism::datalog::ground_binding(get_rule(), m_grounder).first;
        return *m_witness_key;
    }

private:
    const ConstRuleWorkspace<LiftedTag, R>& m_workspace;
    formalism::datalog::GrounderContext& m_grounder;
    mutable std::optional<formalism::datalog::RuleBindingView<R>> m_witness_key;
};

}

#endif
