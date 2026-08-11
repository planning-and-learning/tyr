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

#include "tyr/datalog/lifted/policies/annotation_types.hpp"
#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/datalog/grounder.hpp"

#include <optional>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
class RuleInstance<LiftedTag, R>
{
public:
    using Task = LiftedTag;
    using Relation = R;
    using SourceRule = ::tyr::formalism::datalog::RuleView<R>;

    RuleInstance(SourceRule rule, ::tyr::formalism::datalog::GrounderContext& grounder) : m_rule(rule), m_grounder(grounder) {}

    SourceRule get_rule() const noexcept { return m_rule; }
    auto get_body() const noexcept { return m_rule.get_body(); }
    auto get_head() const noexcept { return m_rule.get_head(); }
    auto get_metric_effects() const noexcept { return m_rule.get_metric_effects(); }

    auto resolve(::tyr::formalism::datalog::AtomView<::tyr::formalism::FluentTag> atom) const
    {
        return ::tyr::formalism::datalog::ground_binding(atom, m_grounder).first;
    }

    auto resolve(::tyr::formalism::datalog::LiftedBooleanOperatorView constraint) const { return ::tyr::formalism::datalog::ground(constraint, m_grounder); }

    ResolvedNumericEffect resolve(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::FluentTag> effect) const
    {
        return { effect.get_operator(),
                 ::tyr::formalism::datalog::ground_binding(effect.get_fterm(), m_grounder).first,
                 ::tyr::formalism::datalog::ground(effect.get_fexpr(), m_grounder) };
    }

    auto witness_key() const
    {
        if (!m_witness_key)
            m_witness_key = ::tyr::formalism::datalog::ground_binding(m_rule, m_grounder).first;
        return *m_witness_key;
    }

private:
    SourceRule m_rule;
    ::tyr::formalism::datalog::GrounderContext& m_grounder;
    mutable std::optional<WitnessRuleKeyT<Task, R>> m_witness_key;
};

}

#endif
