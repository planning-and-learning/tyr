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
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <type_traits>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::analysis
{
namespace
{
struct RelationVertexMap
{
    size_t num_predicates;
    size_t num_functions;

    ygg::uint_t get_vertex(ygg::Index<f::Predicate<f::FluentTag>> predicate) const { return ygg::uint_t(predicate); }
    ygg::uint_t get_vertex(ygg::Index<f::Function<f::FluentTag>> function) const { return static_cast<ygg::uint_t>(num_predicates + ygg::uint_t(function)); }
    size_t size() const { return num_predicates + num_functions; }
};

void add_function_dependencies(fd::FunctionExpressionView expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph);

void add_function_dependencies(ygg::float_t, ygg::uint_t, const RelationVertexMap&, stratification::DepGraph&) {}

void add_function_dependencies(fd::LiftedUnaryOperatorView expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    add_function_dependencies(expression.get_arg(), head_vertex, vertices, graph);
}

template<f::BinaryOperatorKind O>
void add_function_dependencies(fd::LiftedBinaryOperatorView<O> expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    add_function_dependencies(expression.get_lhs(), head_vertex, vertices, graph);
    add_function_dependencies(expression.get_rhs(), head_vertex, vertices, graph);
}

void add_function_dependencies(fd::LiftedMultiOperatorView expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    for (const auto arg : expression.get_args())
        add_function_dependencies(arg, head_vertex, vertices, graph);
}

template<f::FactKind T>
void add_function_dependencies(fd::FunctionTermView<T>, ygg::uint_t, const RelationVertexMap&, stratification::DepGraph&)
{
}

void add_function_dependencies(fd::FunctionTermView<f::FluentTag> term,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    stratification::EdgeProps ep;
    ep.kind = stratification::EdgeKind::NonStrict;
    boost::add_edge(vertices.get_vertex(term.get_function().get_index()), head_vertex, ep, graph);
}

void add_function_dependencies(fd::LiftedArithmeticOperatorView expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    visit([&](auto&& arg) { add_function_dependencies(arg, head_vertex, vertices, graph); }, expression.get_variant());
}

void add_function_dependencies(fd::FunctionExpressionView expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    visit([&](auto&& arg) { add_function_dependencies(arg, head_vertex, vertices, graph); }, expression.get_variant());
}

void add_function_dependencies(fd::LiftedBooleanOperatorView expression,
                               ygg::uint_t head_vertex,
                               const RelationVertexMap& vertices,
                               stratification::DepGraph& graph)
{
    visit(
        [&](auto&& arg)
        {
            add_function_dependencies(arg.get_lhs(), head_vertex, vertices, graph);
            add_function_dependencies(arg.get_rhs(), head_vertex, vertices, graph);
        },
        expression.get_variant());
}

void add_numeric_effect_head_dependencies(fd::NumericEffectView<f::FluentTag> effect,
                                          ygg::uint_t head_vertex,
                                          const RelationVertexMap& vertices,
                                          stratification::DepGraph& graph)
{
    if (effect.get_operator() != f::NumericEffectOperatorKind::Assign)
        add_function_dependencies(effect.get_fterm(), head_vertex, vertices, graph);

    add_function_dependencies(effect.get_fexpr(), head_vertex, vertices, graph);
}

template<f::RelationKind R>
void add_body_dependencies(fd::RuleView<R> rule, ygg::uint_t head_vertex, const RelationVertexMap& vertices, stratification::DepGraph& graph)
{
    for (const auto literal : rule.get_body().template get_literals<f::FluentTag>())
    {
        const auto body_vertex = vertices.get_vertex(literal.get_atom().get_predicate().get_index());

        stratification::EdgeProps ep;
        ep.kind = literal.get_polarity() ? stratification::EdgeKind::NonStrict : stratification::EdgeKind::Strict;
        boost::add_edge(body_vertex, head_vertex, ep, graph);
    }

    for (const auto constraint : rule.get_body().get_numeric_constraints())
        add_function_dependencies(constraint, head_vertex, vertices, graph);
}

ygg::uint_t get_head_vertex(fd::AtomView<f::FluentTag> head, const RelationVertexMap& vertices)
{
    return vertices.get_vertex(head.get_predicate().get_index());
}

ygg::uint_t get_head_vertex(fd::NumericEffectOperatorView<f::FluentTag> head, const RelationVertexMap& vertices)
{
    return visit([&](auto&& effect) { return vertices.get_vertex(effect.get_fterm().get_function().get_index()); }, head.get_variant());
}

void add_head_dependencies(fd::AtomView<f::FluentTag>, ygg::uint_t, const RelationVertexMap&, stratification::DepGraph&);
void add_head_dependencies(fd::NumericEffectOperatorView<f::FluentTag>, ygg::uint_t, const RelationVertexMap&, stratification::DepGraph&);

template<f::RelationKind R>
void add_rule_dependencies(fd::ProgramView<LiftedTag> program, const RelationVertexMap& vertices, stratification::DepGraph& graph)
{
    for (const auto rule : program.get_rules<R>())
    {
        const auto head = rule.get_head();
        const auto head_vertex = get_head_vertex(head, vertices);

        add_body_dependencies(rule, head_vertex, vertices, graph);
        add_head_dependencies(head, head_vertex, vertices, graph);
    }
}

void add_head_dependencies(fd::AtomView<f::FluentTag>, ygg::uint_t, const RelationVertexMap&, stratification::DepGraph&) {}

void add_head_dependencies(fd::NumericEffectOperatorView<f::FluentTag> head,
                           ygg::uint_t head_vertex,
                           const RelationVertexMap& vertices,
                           stratification::DepGraph& graph)
{
    visit([&](auto&& effect) { add_numeric_effect_head_dependencies(effect, head_vertex, vertices, graph); }, head.get_variant());
}

template<f::RelationKind R>
void bucket_rules_by_stratum(fd::ProgramView<LiftedTag> program,
                             const RelationVertexMap& vertices,
                             const std::vector<ygg::uint_t>& relation_stratum,
                             std::vector<RuleStratum>& buckets)
{
    for (const auto rule : program.get_rules<R>())
        buckets[relation_stratum[get_head_vertex(rule.get_head(), vertices)]].template get<R>().push_back(rule.get_index());
}

}  // namespace

// Build dependency graph: nodes = fluent predicates and fluent functions.
static stratification::DepGraph build_dependency_graph(fd::ProgramView<LiftedTag> program, const RelationVertexMap& vertices)
{
    stratification::DepGraph graph(vertices.size());

    add_rule_dependencies<f::PredicateTag>(program, vertices, graph);
    add_rule_dependencies<f::FunctionTag>(program, vertices, graph);

    return graph;
}

RuleStrata compute_rule_stratification(fd::ProgramView<LiftedTag> program)
{
    const auto vertices = RelationVertexMap {
        program.get_predicates<f::FluentTag>().size(),
        program.get_functions<f::FluentTag>().size(),
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
    auto relation_stratum = std::vector<ygg::uint_t>(vertices.size(), 0);
    for (ygg::uint_t i = 0; i < vertices.size(); ++i)
        relation_stratum[i] = comp_stratum[comp[i]];

    // 6) Bucket rules by head stratum.
    auto max_s = ygg::uint_t(0);
    for (auto s : comp_stratum)
        max_s = std::max(max_s, s);

    auto buckets = std::vector<RuleStratum>(max_s + 1);
    bucket_rules_by_stratum<f::PredicateTag>(program, vertices, relation_stratum, buckets);
    bucket_rules_by_stratum<f::FunctionTag>(program, vertices, relation_stratum, buckets);

    auto out = RuleStrata {};
    out.data = std::move(buckets);

    return out;
}
}
