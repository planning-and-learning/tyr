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

#include "tyr/analysis/stratification.hpp"

#include "stratification_utils.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/expression_properties.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <boost/graph/detail/adjacency_list.hpp>
#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::analysis
{
namespace
{
/// The vertices of the dependency graph: the fluent predicates, then the derived predicates, then the fluent functions.
struct RelationVertexMap
{
    size_t num_fluent_predicates;
    size_t num_derived_predicates;
    size_t num_functions;

    [[nodiscard]] ygg::uint_t get_vertex(urd::index<formalism::Predicate<formalism::FluentTag>> predicate) const
    { return static_cast<ygg::uint_t>(predicate.value); }

    [[nodiscard]] ygg::uint_t get_vertex(urd::index<formalism::Predicate<formalism::DerivedTag>> predicate) const
    { return static_cast<ygg::uint_t>(num_fluent_predicates + static_cast<ygg::uint_t>(predicate.value)); }

    [[nodiscard]] ygg::uint_t get_vertex(urd::index<formalism::Function<formalism::FluentTag>> function) const
    { return static_cast<ygg::uint_t>(num_fluent_predicates + num_derived_predicates + static_cast<ygg::uint_t>(function.value)); }

    [[nodiscard]] size_t size() const { return num_fluent_predicates + num_derived_predicates + num_functions; }
};

/// Calls `function` with each head of the rule, of each kind.
template<TaskKind Kind, typename Function>
void for_each_head(formalism::RuleView<Kind> rule, Function function)
{
    for (const auto head : rule.template get_heads<formalism::FluentTag>())
        function(head);
    for (const auto head : rule.template get_heads<formalism::DerivedTag>())
        function(head);
}

template<TaskKind Kind>
void add_body_dependencies(formalism::RuleView<Kind> rule, ygg::uint_t head_vertex, const RelationVertexMap& vertices, stratification::DepGraph& graph)
{
    const auto add_literal_dependency = [&](const auto literal)
    {
        const auto body_vertex = vertices.get_vertex(literal.get_atom().get_predicate().index());

        stratification::EdgeProps ep;
        ep.kind = literal.get_polarity() ? stratification::EdgeKind::NonStrict : stratification::EdgeKind::Strict;
        boost::add_edge(body_vertex, head_vertex, ep, graph);
    };
    for (const auto literal : rule.get_body().template get_literals<formalism::FluentTag>())
        add_literal_dependency(literal);
    for (const auto literal : rule.get_body().template get_literals<formalism::DerivedTag>())
        add_literal_dependency(literal);

    auto reads = urd::Store<urd::set<formalism::FunctionTermView<Kind, formalism::FluentTag>>> {};
    for (const auto constraint : rule.get_body().get_numeric_constraints())
        formalism::collect_fterms(constraint, reads);
    for (const auto term : reads)
    {
        auto ep = stratification::EdgeProps {};
        ep.kind = stratification::EdgeKind::NonStrict;
        boost::add_edge(vertices.get_vertex(term.get_function().index()), head_vertex, ep, graph);
    }
}

template<TaskKind Kind, formalism::FactKind F>
ygg::uint_t get_head_vertex(formalism::AtomView<Kind, F> head, const RelationVertexMap& vertices)
{ return vertices.get_vertex(head.get_predicate().index()); }

template<TaskKind Kind>
void add_rule_dependencies(formalism::ProgramView<Kind> program, const RelationVertexMap& vertices, stratification::DepGraph& graph)
{
    for (const auto rule : program.get_rules())
        for_each_head(rule, [&](const auto head) { add_body_dependencies(rule, get_head_vertex(head, vertices), vertices, graph); });
}

template<TaskKind Kind>
void bucket_rules_by_stratum(formalism::ProgramView<Kind> program,
                             const RelationVertexMap& vertices,
                             const urd::Store<urd::sequence<ygg::uint_t>>& relation_stratum,
                             urd::Store<urd::sequence<RuleStratum<Kind>>>& buckets)
{
    for (const auto rule : program.get_rules())
    {
        auto stratum = std::optional<ygg::uint_t> {};
        for_each_head(rule,
                      [&](const auto head)
                      {
                          const auto head_stratum = relation_stratum[get_head_vertex(head, vertices)];
                          if (stratum && *stratum != head_stratum)
                              throw std::runtime_error("bucket_rules_by_stratum: the heads of a rule must share a stratum.");
                          stratum = head_stratum;
                      });
        assert(stratum);
        buckets[*stratum].push_back(rule.index());
    }
}

}  // namespace

// Build dependency graph: nodes = fluent and derived predicates and fluent functions.
template<TaskKind Kind>
static stratification::DepGraph build_dependency_graph(formalism::ProgramView<Kind> program, const RelationVertexMap& vertices)
{
    stratification::DepGraph graph(vertices.size());

    add_rule_dependencies(program, vertices, graph);

    return graph;
}

template<TaskKind Kind>
RuleStrata<Kind> compute_rule_stratification(formalism::ProgramView<Kind> program)
{
    const auto vertices = RelationVertexMap {
        program.template get_predicates<formalism::FluentTag>().size(),
        program.template get_predicates<formalism::DerivedTag>().size(),
        program.template get_functions<formalism::FluentTag>().size(),
    };

    // 1) dependency graph
    const auto dep = build_dependency_graph(program, vertices);

    // 2) SCC + check
    const auto [comp, num_comps] = stratification::compute_scc_and_check(dep);

    // 3) Condensed DAG
    const auto dag = stratification::build_condensation_dag(dep, comp, num_comps);

    // 4) Component strata
    const auto comp_stratum = stratification::compute_component_strata(dag);

    // 5) Assign each fluent predicate/function relation to a stratum.
    auto relation_stratum = urd::Store<urd::sequence<ygg::uint_t>>(vertices.size(), 0);
    for (ygg::uint_t i = 0; i < vertices.size(); ++i)
        relation_stratum[i] = comp_stratum[comp[i]];

    // 6) Bucket rules by head stratum.
    auto max_s = static_cast<ygg::uint_t>(0);
    for (auto s : comp_stratum)
        max_s = std::max(max_s, s);

    auto buckets = urd::Store<urd::sequence<RuleStratum<Kind>>>(max_s + 1);
    bucket_rules_by_stratum(program, vertices, relation_stratum, buckets);

    auto out = RuleStrata<Kind> {};
    out.data = std::move(buckets);

    return out;
}

template RuleStrata<GroundTag> compute_rule_stratification(formalism::ProgramView<GroundTag> program);
template RuleStrata<LiftedTag> compute_rule_stratification(formalism::ProgramView<LiftedTag> program);
}
