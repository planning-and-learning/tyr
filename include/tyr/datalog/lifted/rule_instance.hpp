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

#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <optional>
#include <utility>

namespace tyr::datalog
{

template<>
class RuleInstance<LiftedTag>
{
public:
    RuleInstance(formalism::RuleView<LiftedTag> rule, formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder) :
        m_rule(rule),
        m_grounder(&grounder)
    {
    }

    formalism::RuleView<LiftedTag> get_rule() const noexcept { return m_rule; }

    auto get_body() const noexcept { return get_rule().get_body(); }

    template<formalism::FactKind F>
    auto resolve(formalism::AtomView<LiftedTag, F> atom) const
    { return formalism::ground_binding(atom, *m_grounder).first; }

    auto resolve(formalism::BooleanOperatorView<LiftedTag> constraint) const { return formalism::ground(constraint, *m_grounder).first; }

    /// The instance's (rule, binding) row keys its witnesses; grounded once, on first use.
    RuleKey<LiftedTag> witness_key() const { return cost_key().index(); }

    /// The row as a view: the credit of its actions reads the row's objects.
    formalism::RuleRowView cost_key() const
    {
        if (!m_witness_key)
            m_witness_key = formalism::ground_binding(get_rule(), *m_grounder).first;
        return *m_witness_key;
    }

private:
    formalism::RuleView<LiftedTag> m_rule;
    formalism::GroundingContext<formalism::Builder, formalism::Repository>* m_grounder;
    mutable std::optional<formalism::RuleRowView> m_witness_key;
};

}

#endif
