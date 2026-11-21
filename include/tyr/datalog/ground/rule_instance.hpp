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
#include "tyr/formalism/repository.hpp"

namespace tyr::datalog
{

template<>
class RuleInstance<GroundTag>
{
public:
    explicit RuleInstance(formalism::RuleView<GroundTag> rule) : m_rule(rule) {}

    [[nodiscard]] formalism::RuleView<GroundTag> get_rule() const noexcept { return m_rule; }

    [[nodiscard]] auto get_body() const noexcept { return m_rule.get_body(); }

    template<formalism::FactKind F>
    [[nodiscard]] auto get_heads() const noexcept
    { return m_rule.template get_heads<F>(); }

    template<formalism::FactKind F>
    [[nodiscard]] auto resolve(formalism::AtomView<GroundTag, F> atom) const noexcept
    { return atom.get_row(); }

    [[nodiscard]] auto resolve(formalism::BooleanOperatorView<GroundTag> constraint) const noexcept { return constraint; }

    /// A ground rule is its own instance: its index keys witnesses and rule costs.
    [[nodiscard]] RuleKey<GroundTag> witness_key() const noexcept { return m_rule.index(); }

    [[nodiscard]] RuleKey<GroundTag> cost_key() const noexcept { return witness_key(); }

private:
    formalism::RuleView<GroundTag> m_rule;
};

}

#endif
