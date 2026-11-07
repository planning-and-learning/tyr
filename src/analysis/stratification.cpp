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
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "stratification_utils.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cassert>
#include <optional>
#include <stdexcept>

namespace tyr::analysis
{
namespace
{
struct RelationVertexMap
{
    size_t num_predicates;
    size_t num_functions;

    [[nodiscard]] ygg::uint_t get_vertex(urd::index<formalism::Predicate<formalism::FluentTag>> predicate) const { return static_cast<ygg::uint_t>(predicate.value); }
    [[nodiscard]] ygg::uint_t get_vertex(urd::index<formalism::Function<formalism::FluentTag>> function) const
    { return static_cast<ygg::uint_t>(num_predicates + static_cast<ygg::uint_t>(function.value)); }
    [[nodiscard]] size_t size() const { return num_predicates + num_functions; }
};

template<formalism::RelationKind R>
void add_body_dependencies(formalism::datalog::RuleView<LiftedTag, R> rule,
                           ygg::uint_t head_vertex,
                           const RelationVertexMap& vertices,
                           stratification::DepGraph& graph)
{
    for (const auto literal : rule.get_body().template get_literals<formalism::FluentTag>())
    {
        const auto body_vertex = vertices.get_vertex(literal.get_atom().get_predicate().index());

        stratification::EdgeProps ep;
        ep.kind = literal.get_polarity() ? stratification::EdgeKind::NonStrict : stratification::EdgeKind::Strict;
        boost::add_edge(body_vertex, head_vertex, ep, graph);
    }

    for (const auto term : formalism::datalog::collect_fluent_reads(rule))
    {
        auto ep = stratification::EdgeProps {};
        ep.kind = stratification::EdgeKind::NonStrict;
        boost::add_edge(vertices.get_vertex(term.get_function().index()), head_vertex, ep, graph);
    }
}

ygg::uint_t get_head_vertex(formalism::datalog::AtomView<LiftedTag, formalism::FluentTag> head, const RelationVertexMap& vertices)
{ return vertices.get_vertex(head.get_predicate().index()); }

ygg::uint_t get_head_vertex(formalism::datalog::NumericEffectOperatorView<LiftedTag, formalism::FluentTag> head, const RelationVertexMap& vertices)
{
    return vertices.get_vertex(head.get_fterm().get_function().index());
}

template<formalism::RelationKind R>
void add_rule_dependencies(formalism::datalog::ProgramView<LiftedTag> program, const RelationVertexMap& vertices, stratification::DepGraph& graph)
{
    for (const auto rule : program.get_rules<R>())
        formalism::datalog::for_each_head(rule, [&](const auto& head) { add_body_dependencies(rule, get_head_vertex(head, vertices), vertices, graph); });
}

template<formalism::RelationKind R>
void bucket_rules_by_stratum(formalism::datalog::ProgramView<LiftedTag> program,
                             const RelationVertexMap& vertices,
                             const urd::Store<urd::sequence<ygg::uint_t>>& relation_stratum,
                             urd::Store<urd::sequence<RuleStratum>>& buckets)
{
    for (const auto rule : program.get_rules<R>())
    {
        auto stratum = std::optional<ygg::uint_t> {};
        formalism::datalog::for_each_head(rule,
                                          [&](const auto& head)
                                          {
                                              const auto head_stratum = relation_stratum[get_head_vertex(head, vertices)];
                                              if (stratum && *stratum != head_stratum)
                                                  throw std::runtime_error("bucket_rules_by_stratum: the heads of a rule must share a stratum.");
                                              stratum = head_stratum;
                                          });
        assert(stratum);
        buckets[*stratum].template get<R>().push_back(rule.index());
    }
}

}  // namespace

// Build dependency graph: nodes = fluent predicates and fluent functions.
static stratification::DepGraph build_dependency_graph(formalism::datalog::ProgramView<LiftedTag> program, const RelationVertexMap& vertices)
{
    stratification::DepGraph graph(vertices.size());

    add_rule_dependencies<formalism::PredicateTag>(program, vertices, graph);
    add_rule_dependencies<formalism::FunctionTag>(program, vertices, graph);

    return graph;
}

RuleStrata compute_rule_stratification(formalism::datalog::ProgramView<LiftedTag> program)
{
    const auto vertices = RelationVertexMap {
        program.get_predicates<formalism::FluentTag>().size(),
        program.get_functions<formalism::FluentTag>().size(),
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

    auto buckets = urd::Store<urd::sequence<RuleStratum>>(max_s + 1);
    bucket_rules_by_stratum<formalism::PredicateTag>(program, vertices, relation_stratum, buckets);
    bucket_rules_by_stratum<formalism::FunctionTag>(program, vertices, relation_stratum, buckets);

    auto out = RuleStrata {};
    out.data = std::move(buckets);

    return out;
}
}
