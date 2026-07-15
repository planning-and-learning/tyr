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

#ifndef TYR_DATALOG_RULE_SCHEDULER_HPP_
#define TYR_DATALOG_RULE_SCHEDULER_HPP_

#include "tyr/analysis/listeners.hpp"              // for ListenerStratum
#include "tyr/analysis/stratification.hpp"         // for RuleStratum, Rule...
#include "tyr/formalism/datalog/declarations.hpp"  // for FluentTag (ptr only)
#include "tyr/formalism/datalog/rule_index.hpp"    // for ygg::Index

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset
#include <vector>                                   // for vector
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/vector.hpp>  // for ygg::View
#include <yggdrasil/core/types.hpp>         // for ygg::IndexList
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/semantics/equal_to.hpp>  // for EqualTo
#include <yggdrasil/semantics/hash.hpp>      // for Hash

namespace tyr::datalog
{

class RuleSchedulerStratum
{
public:
    RuleSchedulerStratum(const analysis::RuleStratum& rules,
                         const analysis::ListenerStratum& listeners,
                         const ::tyr::formalism::datalog::Repository& context,
                         size_t num_fluent_predicates,
                         size_t num_fluent_functions);

    void activate_all();

    void on_start_iteration() noexcept;

    void on_generate(ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> predicate);
    void on_generate(ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>> function);

    void on_finish_iteration();

    const ::tyr::formalism::datalog::Repository& get_context() const noexcept { return m_context; }
    const ygg::IndexList<::tyr::formalism::datalog::Rule>& get_rules() const noexcept { return m_rules; }

    /// Active rules in sorted index order: hash-set iteration order is platform-unspecified, but the
    /// rule processing order assigns program-repository rows (first-derivation order) that delta
    /// bitsets are indexed by, so it must be identical on every platform.
    const ygg::IndexList<::tyr::formalism::datalog::Rule>& get_active_rules() const noexcept { return m_sorted_active_rules; }

private:
    void rebuild_sorted_active_rules();

    const analysis::RuleStratum& m_rules;
    const analysis::ListenerStratum& m_listeners;
    const ::tyr::formalism::datalog::Repository& m_context;

    boost::dynamic_bitset<> m_active_predicates;
    boost::dynamic_bitset<> m_active_functions;
    ygg::UnorderedSet<ygg::Index<::tyr::formalism::datalog::Rule>> m_active_rules;
    ygg::IndexList<::tyr::formalism::datalog::Rule> m_sorted_active_rules;
};

struct RuleSchedulerStrata
{
    std::vector<RuleSchedulerStratum> data;
};

extern RuleSchedulerStrata create_schedulers(const analysis::RuleStrata& rules,
                                             const analysis::ListenerStrata& listeners,
                                             const ::tyr::formalism::datalog::Repository& context,
                                             size_t num_fluent_predicates,
                                             size_t num_fluent_functions);

}

#endif
