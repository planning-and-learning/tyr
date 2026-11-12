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

namespace tyr::datalog
{

template<>
class RuleInstance<GroundTag>
{
public:
    explicit RuleInstance(formalism::datalog::RuleView<GroundTag> rule) : m_rule(rule) {}

    [[nodiscard]] formalism::datalog::RuleView<GroundTag> get_rule() const noexcept { return m_rule; }
    [[nodiscard]] auto get_body() const noexcept { return m_rule.get_body(); }
    [[nodiscard]] auto get_heads() const noexcept { return m_rule.get_heads(); }

    [[nodiscard]] auto resolve(formalism::datalog::AtomView<GroundTag, formalism::FluentTag> atom) const noexcept { return atom.get_row(); }
    [[nodiscard]] auto resolve_row(formalism::datalog::AtomView<GroundTag, formalism::FluentTag> atom) const noexcept { return atom.get_row().data(); }

    [[nodiscard]] auto resolve(formalism::datalog::BooleanOperatorView<GroundTag> constraint) const noexcept { return constraint; }

    [[nodiscard]] auto witness_key() const noexcept { return m_rule.get_row(); }

private:
    formalism::datalog::RuleView<GroundTag> m_rule;
};

}

#endif
