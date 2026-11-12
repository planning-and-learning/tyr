/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/construction.hpp"
#include "tyr/datalog/lifted/database/datalog/evaluator.hpp"

#include <algorithm>
#include <functional>
#include <gtest/gtest.h>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace
{
namespace dl = tyr::datalog::database::datalog;
using ygg::uint_t;
using Tuple = std::vector<uint_t>;
using Tuples = std::set<Tuple>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using Change = std::pair<ygg::FreeBuilderRelationView, ygg::FreeBuilderRelationView>;
using Function = dl::AggregateFunction;

dl::Term var(uint_t variable) { return dl::Term::variable(variable); }
dl::Term obj(uint_t object) { return dl::Term::constant(object); }

/// Interns programs with short calls.
struct Programs
{
    dl::DatalogRepository repository;
    dl::DatalogBuilder builder;

    dl::RelationView relation(size_t id, size_t arity, size_t slot = urd::data<dl::Relation>::derived)
    {
        auto data = dl::checkout<dl::Relation>(builder);
        data->id = id;
        data->arity = arity;
        data->input = slot != urd::data<dl::Relation>::derived;
        data->input_slot = data->input ? slot : 0;
        return dl::insert(repository, *data).first;
    }
    /// A derived relation whose last column is a weight, least per row of the other columns.
    dl::RelationView weighted(size_t id, size_t arity)
    {
        auto data = dl::checkout<dl::Relation>(builder);
        data->id = id;
        data->arity = arity;
        data->weighted = true;
        return dl::insert(repository, *data).first;
    }
    dl::AtomView atom(dl::RelationView relation, std::initializer_list<dl::Term> terms)
    {
        auto data = dl::checkout<dl::Atom>(builder);
        data->relation = relation.index();
        for (const auto term : terms)
            data->terms.push_back(term);
        return dl::insert(repository, *data).first;
    }
    dl::AggregateView aggregate(Function function,
                                uint_t result,
                                uint_t value,
                                std::initializer_list<dl::AtomView> atoms,
                                std::initializer_list<dl::AtomView> negated = {},
                                std::initializer_list<dl::Arithmetic> arithmetic = {})
    {
        auto data = dl::checkout<dl::Aggregate>(builder);
        data->function = function;
        data->result = result;
        data->value = value;
        for (const auto atom : atoms)
            data->atoms.push_back(atom.index());
        for (const auto atom : negated)
            data->negated.push_back(atom.index());
        for (const auto& operation : arithmetic)
            data->arithmetic.push_back(operation);
        return dl::insert(repository, *data).first;
    }
    dl::RuleView rule(dl::AtomView head,
                      std::initializer_list<dl::AtomView> atoms,
                      std::initializer_list<dl::AtomView> negated = {},
                      std::initializer_list<dl::Comparison> comparisons = {},
                      std::initializer_list<dl::AggregateView> aggregates = {},
                      std::initializer_list<dl::Arithmetic> arithmetic = {})
    {
        auto data = dl::checkout<dl::Rule>(builder);
        data->head = head.index();
        for (const auto atom : atoms)
            data->atoms.push_back(atom.index());
        for (const auto atom : negated)
            data->negated.push_back(atom.index());
        for (const auto& comparison : comparisons)
            data->comparisons.push_back(comparison);
        for (const auto aggregate : aggregates)
            data->aggregates.push_back(aggregate.index());
        for (const auto& operation : arithmetic)
            data->arithmetic.push_back(operation);
        return dl::insert(repository, *data).first;
    }
    dl::ProgramView program(std::initializer_list<dl::RuleView> rules)
    {
        auto data = dl::checkout<dl::Program>(builder);
        for (const auto rule : rules)
            data->rules.push_back(rule.index());
        return dl::insert(repository, *data).first;
    }
};

/// Naive bottom-up evaluation, stratum by stratum, independent of the evaluator.
class Interpreter
{
    dl::ProgramView m_program;
    std::map<size_t, Tuples> m_relations;
    const std::vector<Tuples>* m_inputs;

    [[nodiscard]] const Tuples& rows(dl::RelationView relation) const
    {
        static const Tuples none;
        if (relation.is_input())
            return (*m_inputs)[relation.get_input_slot()];
        const auto it = m_relations.find(relation.get_id());
        return it == m_relations.end() ? none : it->second;
    }
    static std::optional<uint_t> value(dl::Term term, const std::map<uint_t, uint_t>& assignment)
    {
        if (!term.is_variable())
            return term.value;
        const auto it = assignment.find(term.value);
        return it == assignment.end() ? std::nullopt : std::optional(it->second);
    }
    bool matches(dl::AtomView atom, const Tuple& tuple, std::map<uint_t, uint_t>& assignment) const
    {
        const auto terms = atom.get_terms();
        for (size_t i = 0; i < terms.size(); ++i)
        {
            if (const auto bound = value(terms[i], assignment))
            {
                if (*bound != tuple[i])
                    return false;
            }
            else
                assignment[terms[i].value] = tuple[i];
        }
        return true;
    }
    template<typename Atoms>
    void
    enumerate(const Atoms& atoms, size_t i, const std::map<uint_t, uint_t>& assignment, const std::function<void(const std::map<uint_t, uint_t>&)>& f) const
    {
        if (i == atoms.size())
            return f(assignment);
        for (const auto& tuple : rows(atoms[i].get_relation()))
        {
            auto extended = assignment;
            if (matches(atoms[i], tuple, extended))
                enumerate(atoms, i + 1, extended, f);
        }
    }
    template<typename Negated>
    [[nodiscard]] bool passes(const Negated& negated, std::span<const dl::Comparison> comparisons, const std::map<uint_t, uint_t>& assignment) const
    {
        for (const auto& comparison : comparisons)
            if (!comparison.holds(value(comparison.lhs, assignment).value(), value(comparison.rhs, assignment).value()))
                return false;
        for (const auto atom : negated)
        {
            auto tuple = Tuple();
            for (const auto term : atom.get_terms())
                tuple.push_back(value(term, assignment).value());
            if (rows(atom.get_relation()).contains(tuple))
                return false;
        }
        return true;
    }
    static void bind(std::span<const dl::Arithmetic> arithmetic, std::map<uint_t, uint_t>& assignment)
    {
        for (const auto& operation : arithmetic)
            assignment[operation.result] = operation.apply(value(operation.lhs, assignment).value(), value(operation.rhs, assignment).value());
    }
    /// The aggregate over the assignments of its variables that extend `outer` on the variables both bind.
    [[nodiscard]] std::optional<uint_t> aggregate(dl::AggregateView aggregate, const std::map<uint_t, uint_t>& outer) const
    {
        const auto atoms = std::vector<dl::AtomView>(aggregate.get_atoms().begin(), aggregate.get_atoms().end());
        const auto negated = std::vector<dl::AtomView>(aggregate.get_negated().begin(), aggregate.get_negated().end());
        std::set<std::map<uint_t, uint_t>> assignments;
        enumerate(atoms,
                  0,
                  {},
                  [&](const auto& matched)
                  {
                      auto inner = matched;
                      bind(aggregate.get_arithmetic(), inner);
                      for (const auto& [variable, object] : inner)
                          if (const auto it = outer.find(variable); it != outer.end() && it->second != object)
                              return;
                      if (passes(negated, aggregate.get_comparisons(), inner))
                          assignments.insert(inner);
                  });
        if (aggregate.get_function() == Function::COUNT)
            return static_cast<uint_t>(assignments.size());
        std::vector<uint_t> values;
        values.reserve(assignments.size());
        for (const auto& assignment : assignments)
            values.push_back(assignment.at(aggregate.get_value()));
        if (aggregate.get_function() == Function::SUM)
        {
            uint_t sum = 0;
            for (const auto v : values)
                sum += v;
            return sum;
        }
        if (values.empty())
            return std::nullopt;
        return aggregate.get_function() == Function::MIN ? *std::ranges::min_element(values) : *std::ranges::max_element(values);
    }
    bool apply(dl::RuleView rule)
    {
        const auto atoms = std::vector<dl::AtomView>(rule.get_atoms().begin(), rule.get_atoms().end());
        const auto negated = std::vector<dl::AtomView>(rule.get_negated().begin(), rule.get_negated().end());
        const auto head = rule.get_head();
        auto derived = Tuples();
        enumerate(atoms,
                  0,
                  {},
                  [&](const auto& base)
                  {
                      auto assignment = base;
                      for (const auto aggregate : rule.get_aggregates())
                      {
                          const auto result = this->aggregate(aggregate, base);
                          if (!result)
                              return;
                          assignment[aggregate.get_result()] = *result;
                      }
                      bind(rule.get_arithmetic(), assignment);
                      if (!passes(negated, rule.get_comparisons(), assignment))
                          return;
                      auto tuple = Tuple();
                      for (const auto term : head.get_terms())
                          tuple.push_back(value(term, assignment).value());
                      derived.insert(tuple);
                  });
        auto& target = m_relations[head.get_relation().get_id()];
        const auto before = target;
        target.insert(derived.begin(), derived.end());
        if (head.get_relation().is_weighted())
        {
            // The least weight per row of the other columns (Bellman-Ford over the rounds).
            auto least = std::map<Tuple, uint_t>();
            for (const auto& tuple : target)
            {
                const auto key = Tuple(tuple.begin(), tuple.end() - 1);
                if (const auto it = least.find(key); it == least.end() || tuple.back() < it->second)
                    least[key] = tuple.back();
            }
            target.clear();
            for (const auto& [key, weight] : least)
            {
                auto tuple = key;
                tuple.push_back(weight);
                target.insert(tuple);
            }
        }
        return target != before;
    }

public:
    Interpreter(dl::ProgramView program, const std::vector<Tuples>& inputs) : m_program(program), m_inputs(&inputs)
    {
        // A relation's stratum is at least its positive dependencies' and above its negative and aggregated ones.
        std::map<size_t, size_t> stratum;
        for (bool changed = true; changed;)
        {
            changed = false;
            for (const auto rule : program.get_rules())
            {
                auto& level = stratum[rule.get_head().get_relation().get_id()];
                const auto require = [&](dl::AtomView atom, size_t above)
                {
                    if (atom.get_relation().is_input())
                        return;
                    const auto needed = stratum[atom.get_relation().get_id()] + above;
                    if (needed > level)
                    {
                        level = needed;
                        changed = true;
                    }
                };
                for (const auto atom : rule.get_atoms())
                    require(atom, 0);
                for (const auto atom : rule.get_negated())
                    require(atom, 1);
                for (const auto aggregate : rule.get_aggregates())
                    for (const auto atom : aggregate.get_atoms())
                        require(atom, 1);
            }
        }
        size_t top = 0;
        for (const auto& [relation, level] : stratum)
            top = std::max(top, level);
        for (size_t level = 0; level <= top; ++level)
            for (bool changed = true; changed;)
            {
                changed = false;
                for (const auto rule : program.get_rules())
                    if (stratum[rule.get_head().get_relation().get_id()] == level)
                        changed = apply(rule) || changed;
            }
    }
    [[nodiscard]] const Tuples& get(size_t relation) const
    {
        static const Tuples none;
        const auto it = m_relations.find(relation);
        return it == m_relations.end() ? none : it->second;
    }
};

const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const Relation& relation) { return layer().view(relation); }

Relation relation_of(const Tuples& tuples, size_t arity)
{
    auto columns = std::vector<ygg::ColumnLayout>();
    for (size_t i = 0; i < arity; ++i)
        columns.push_back(ygg::ColumnLayout { urd::index<ygg::Column>(ygg::to_uint_t(i)), ygg::type_id<uint_t> });
    Relation result { std::span<const ygg::ColumnLayout>(columns) };
    auto values = std::vector<ygg::Value>(arity);
    for (const auto& tuple : tuples)
    {
        for (size_t i = 0; i < arity; ++i)
            values[i] = ygg::encode_value(tuple[i]);
        result.insert(std::span<const ygg::Value>(values));
    }
    return result;
}

Tuples tuples_of(ygg::FreeBuilderRelationView relation)
{
    Tuples result;
    for (size_t i = 0; i < relation.size(); ++i)
    {
        const auto values = relation.row(i).get_values();
        Tuple tuple;
        for (size_t c = 0; c < relation.columns().size(); ++c)
            tuple.push_back(ygg::decode_value<uint_t>(values.get(c)));
        result.insert(tuple);
    }
    return result;
}

Tuples tuples_of(const Relation& relation) { return tuples_of(borrow(relation)); }

/// The test programs over the inputs e (binary, slot 0) and v (unary, slot 1).
struct Fixture
{
    Programs p;
    dl::RelationView e = p.relation(100, 2, 0), v = p.relation(101, 1, 1);
    dl::RelationView tc = p.relation(0, 2), tc2 = p.relation(1, 2), odd = p.relation(2, 2), even = p.relation(3, 2), unreachable = p.relation(4, 2),
                     loops = p.relation(5, 1), from3 = p.relation(6, 1), distinct = p.relation(7, 2), into2 = p.relation(8, 1), degree = p.relation(9, 2),
                     edges = p.relation(10, 1), largest = p.relation(11, 2), smallest = p.relation(12, 2), weight = p.relation(13, 2),
                     reach = p.relation(14, 2), isolated = p.relation(15, 1), has_out = p.relation(16, 1), triangle = p.relation(17, 3),
                     cycle = p.relation(18, 2), sums = p.relation(19, 3), ascending = p.relation(20, 2), sparse = p.relation(21, 2), gaps = p.relation(22, 2),
                     doubled = p.relation(23, 2), dist = p.weighted(24, 2), costly = p.weighted(25, 2), wide = p.weighted(26, 2), far = p.relation(27, 1);
    std::vector<dl::RelationView> outputs { tc,     tc2,       odd,     even,     unreachable, loops, from3,    distinct, into2,
                                            degree, edges,     largest, smallest, weight,      reach, isolated, triangle, cycle,
                                            sums,   ascending, sparse,  gaps,     doubled,     dist,  costly,   wide,     far };
    dl::ProgramView program = make();

    dl::ProgramView make()
    {
        constexpr uint_t x = 0, y = 1, z = 2, n = 3, m = 4;
        using enum dl::Comparison::Kind;
        using Operation = dl::Arithmetic::Operation;
        return p.program({
            // Transitive closure, linear and nonlinear.
            p.rule(p.atom(tc, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }),
            p.rule(p.atom(tc, { var(x), var(z) }), { p.atom(tc, { var(x), var(y) }), p.atom(e, { var(y), var(z) }) }),
            p.rule(p.atom(tc2, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }),
            p.rule(p.atom(tc2, { var(x), var(z) }), { p.atom(tc2, { var(x), var(y) }), p.atom(tc2, { var(y), var(z) }) }),
            // Mutual recursion.
            p.rule(p.atom(odd, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }),
            p.rule(p.atom(odd, { var(x), var(z) }), { p.atom(even, { var(x), var(y) }), p.atom(e, { var(y), var(z) }) }),
            p.rule(p.atom(even, { var(x), var(z) }), { p.atom(odd, { var(x), var(y) }), p.atom(e, { var(y), var(z) }) }),
            // Stratified negation.
            p.rule(p.atom(unreachable, { var(x), var(y) }), { p.atom(v, { var(x) }), p.atom(v, { var(y) }) }, { p.atom(tc, { var(x), var(y) }) }),
            p.rule(p.atom(has_out, { var(x) }), { p.atom(e, { var(x), var(y) }) }),
            p.rule(p.atom(isolated, { var(x) }), { p.atom(v, { var(x) }) }, { p.atom(has_out, { var(x) }) }),
            // Repeated variables, constants and comparisons.
            p.rule(p.atom(loops, { var(x) }), { p.atom(e, { var(x), var(x) }) }),
            p.rule(p.atom(from3, { var(y) }), { p.atom(e, { obj(3), var(y) }) }),
            p.rule(p.atom(distinct, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }, {}, { dl::Comparison { NOT_EQUAL, var(x), var(y) } }),
            p.rule(p.atom(into2, { var(x) }), { p.atom(e, { var(x), var(y) }) }, {}, { dl::Comparison { EQUAL, var(y), obj(2) } }),
            // Cyclic bodies (Generic Join), also recursive.
            p.rule(p.atom(triangle, { var(x), var(y), var(z) }),
                   { p.atom(e, { var(x), var(y) }), p.atom(e, { var(y), var(z) }), p.atom(e, { var(z), var(x) }) }),
            p.rule(p.atom(cycle, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }), p.atom(e, { var(y), var(x) }) }),
            p.rule(p.atom(cycle, { var(x), var(z) }), { p.atom(cycle, { var(x), var(y) }), p.atom(e, { var(y), var(z) }), p.atom(tc, { var(z), var(x) }) }),
            // Aggregates, grouped by the rule's other variables, also over a lower stratum's recursive relation.
            p.rule(p.atom(degree, { var(x), var(n) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   {},
                   { p.aggregate(Function::COUNT, n, 0, { p.atom(e, { var(x), var(y) }) }) }),
            p.rule(p.atom(edges, { var(n) }), {}, {}, {}, { p.aggregate(Function::COUNT, n, 0, { p.atom(e, { var(x), var(y) }) }) }),
            p.rule(p.atom(largest, { var(x), var(n) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   {},
                   { p.aggregate(Function::MAX, n, y, { p.atom(e, { var(x), var(y) }) }) }),
            p.rule(p.atom(smallest, { var(x), var(n) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   {},
                   { p.aggregate(Function::MIN, n, y, { p.atom(tc, { var(x), var(y) }) }) }),
            p.rule(p.atom(weight, { var(x), var(n) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   {},
                   { p.aggregate(Function::SUM, n, y, { p.atom(e, { var(x), var(y) }) }) }),
            p.rule(p.atom(reach, { var(x), var(n) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   {},
                   { p.aggregate(Function::COUNT, n, 0, { p.atom(tc, { var(x), var(y) }) }) }),
            // Arithmetic and order: results in heads, read by comparisons, by later arithmetic and as an aggregate's value;
            // a recursive relation over a computed value.
            p.rule(p.atom(sums, { var(x), var(y), var(n) }),
                   { p.atom(e, { var(x), var(y) }) },
                   {},
                   {},
                   {},
                   { dl::Arithmetic { Operation::ADD, var(x), var(y), n } }),
            p.rule(p.atom(ascending, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }, {}, { dl::Comparison { LESS, var(x), var(y) } }),
            p.rule(p.atom(sparse, { var(x), var(n) }), { p.atom(degree, { var(x), var(n) }) }, {}, { dl::Comparison { LESS_EQUAL, var(n), obj(1) } }),
            p.rule(p.atom(gaps, { var(x), var(m) }),
                   { p.atom(e, { var(x), var(y) }) },
                   {},
                   { dl::Comparison { NOT_EQUAL, var(m), obj(0) } },
                   {},
                   { dl::Arithmetic { Operation::SUBTRACT, var(y), var(x), n }, dl::Arithmetic { Operation::MAX, var(n), obj(1), m } }),
            p.rule(p.atom(gaps, { var(x), var(m) }),
                   { p.atom(gaps, { var(x), var(n) }) },
                   {},
                   { dl::Comparison { LESS, var(n), obj(4) } },
                   {},
                   { dl::Arithmetic { Operation::MULTIPLY, var(n), obj(2), m } }),
            p.rule(p.atom(doubled, { var(x), var(n) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   {},
                   { p.aggregate(Function::SUM, n, m, { p.atom(e, { var(x), var(y) }) }, {}, { dl::Arithmetic { Operation::MULTIPLY, var(y), obj(2), m } }) }),
            // Weighted relations: unit distances from 0, distances with computed edge costs from 1, the widest bottleneck
            // (max) from 2, and a higher stratum reading weights as values.
            p.rule(p.atom(dist, { var(x), obj(0) }), { p.atom(v, { var(x) }) }, {}, { dl::Comparison { EQUAL, var(x), obj(0) } }),
            p.rule(p.atom(dist, { var(z), var(m) }),
                   { p.atom(dist, { var(y), var(n) }), p.atom(e, { var(y), var(z) }) },
                   {},
                   {},
                   {},
                   { dl::Arithmetic { Operation::ADD, var(n), obj(1), m } }),
            p.rule(p.atom(costly, { var(x), obj(0) }), { p.atom(v, { var(x) }) }, {}, { dl::Comparison { EQUAL, var(x), obj(1) } }),
            p.rule(p.atom(costly, { var(z), var(m) }),
                   { p.atom(costly, { var(y), var(n) }), p.atom(e, { var(y), var(z) }) },
                   {},
                   {},
                   {},
                   { dl::Arithmetic { Operation::ADD, var(y), var(z), x }, dl::Arithmetic { Operation::ADD, var(n), var(x), m } }),
            p.rule(p.atom(wide, { var(x), var(m) }),
                   { p.atom(v, { var(x) }) },
                   {},
                   { dl::Comparison { EQUAL, var(x), obj(2) } },
                   {},
                   { dl::Arithmetic { Operation::ADD, var(x), obj(0), m } }),
            p.rule(p.atom(wide, { var(z), var(m) }),
                   { p.atom(wide, { var(y), var(n) }), p.atom(e, { var(y), var(z) }) },
                   {},
                   {},
                   {},
                   { dl::Arithmetic { Operation::MAX, var(n), var(z), m } }),
            p.rule(p.atom(far, { var(x) }), { p.atom(dist, { var(x), var(n) }) }, {}, { dl::Comparison { LESS, obj(1), var(n) } }),
        });
    }
};
}  // namespace

TEST(DatabaseDatalog, ArithmeticSaturatesAndInfinityIsTheLargestValue)
{
    using Operation = dl::Arithmetic::Operation;
    constexpr auto inf = dl::Arithmetic::infinity;
    const auto apply = [](Operation operation, uint_t a, uint_t b) { return dl::Arithmetic { operation, {}, {}, 0 }.apply(a, b); };
    EXPECT_EQ(apply(Operation::ADD, 2, 3), 5);
    EXPECT_EQ(apply(Operation::ADD, inf - 1, 1), inf);
    EXPECT_EQ(apply(Operation::ADD, inf, 0), inf);
    EXPECT_EQ(apply(Operation::SUBTRACT, 2, 3), 0);
    EXPECT_EQ(apply(Operation::SUBTRACT, 3, inf), 0);
    EXPECT_EQ(apply(Operation::SUBTRACT, inf, inf), inf);
    EXPECT_EQ(apply(Operation::MULTIPLY, 0, inf), 0);
    EXPECT_EQ(apply(Operation::MULTIPLY, 2, inf), inf);
    EXPECT_EQ(apply(Operation::MULTIPLY, inf / 2 + 1, 2), inf);
    EXPECT_EQ(apply(Operation::DIVIDE, 7, 2), 3);
    EXPECT_EQ(apply(Operation::DIVIDE, 7, 0), inf);
    EXPECT_EQ(apply(Operation::DIVIDE, 7, inf), 0);
    EXPECT_EQ(apply(Operation::DIVIDE, inf, inf), inf);
    EXPECT_EQ(apply(Operation::MIN, 4, inf), 4);
    EXPECT_EQ(apply(Operation::MAX, 4, inf), inf);
    EXPECT_TRUE((dl::Comparison { dl::Comparison::Kind::LESS, {}, {} }.holds(4, inf)));
    EXPECT_FALSE((dl::Comparison { dl::Comparison::Kind::LESS, {}, {} }.holds(4, 4)));
    EXPECT_TRUE((dl::Comparison { dl::Comparison::Kind::LESS_EQUAL, {}, {} }.holds(4, 4)));
}

TEST(DatabaseDatalog, EvaluatorEqualsNaiveEvaluation)
{
    Fixture fixture;
    dl::Evaluator evaluator(layer(), fixture.program, fixture.outputs);
    std::mt19937 random(5);
    for (size_t trial = 0; trial < 60; ++trial)
    {
        std::vector<Tuples> inputs(2);
        for (size_t i = 0, count = random() % 14; i < count; ++i)
            inputs[0].insert({ static_cast<uint_t>(random() % 6), static_cast<uint_t>(random() % 6) });
        for (uint_t vertex = 0; vertex < 6; ++vertex)
            if (random() % 4 != 0)
                inputs[1].insert({ vertex });
        const auto e = relation_of(inputs[0], 2), v = relation_of(inputs[1], 1);
        evaluator.evaluate(std::array { borrow(e), borrow(v) });
        const Interpreter expected(fixture.program, inputs);
        for (size_t i = 0; i < fixture.outputs.size(); ++i)
            EXPECT_EQ(tuples_of(evaluator.get_result(i)), expected.get(fixture.outputs[i].get_id()))
                << "trial " << trial << ", relation " << fixture.outputs[i].get_id();
    }
}

TEST(DatabaseDatalog, EvaluatorUpdatesEqualNaiveEvaluationAfterEveryBatch)
{
    Fixture fixture;
    dl::Evaluator evaluator(layer(), fixture.program, fixture.outputs);
    std::vector<Tuples> current(2);
    evaluator.initialize(std::array { borrow(relation_of(current[0], 2)), borrow(relation_of(current[1], 1)) });
    std::mt19937 random(9);
    for (size_t batch = 0; batch < 400; ++batch)
    {
        std::vector<Tuples> added(2), removed(2);
        const size_t removing = random() % 4;
        for (size_t i = 0; i < 3; ++i)
        {
            const size_t slot = random() % 2;
            const size_t arity = slot == 0 ? 2 : 1;
            if (removing == slot && !current[slot].empty())
            {
                auto it = current[slot].begin();
                std::advance(it, random() % current[slot].size());
                if (!added[slot].contains(*it))
                    removed[slot].insert(*it);
                continue;
            }
            Tuple tuple;
            for (size_t c = 0; c < arity; ++c)
                tuple.push_back(static_cast<uint_t>(random() % 6));
            if (!current[slot].contains(tuple))
                added[slot].insert(tuple);
        }
        std::vector<Tuples> before;
        before.reserve(fixture.outputs.size());
        for (size_t i = 0; i < fixture.outputs.size(); ++i)
            before.push_back(tuples_of(evaluator.get_result(i)));
        const auto ea = relation_of(added[0], 2), er = relation_of(removed[0], 2), va = relation_of(added[1], 1), vr = relation_of(removed[1], 1);
        evaluator.update(std::array { Change { borrow(ea), borrow(er) }, Change { borrow(va), borrow(vr) } });
        for (size_t slot = 0; slot < 2; ++slot)
        {
            for (const auto& tuple : removed[slot])
                current[slot].erase(tuple);
            current[slot].insert(added[slot].begin(), added[slot].end());
        }
        const Interpreter expected(fixture.program, current);
        for (size_t i = 0; i < fixture.outputs.size(); ++i)
        {
            const auto relation = fixture.outputs[i].get_id();
            const auto& after = expected.get(relation);
            ASSERT_EQ(tuples_of(evaluator.get_result(i)), after) << "batch " << batch << ", relation " << relation;
            Tuples gained, lost;
            std::ranges::set_difference(after, before[i], std::inserter(gained, gained.end()));
            std::ranges::set_difference(before[i], after, std::inserter(lost, lost.end()));
            EXPECT_EQ(tuples_of(evaluator.get_delta(i).added), gained) << "batch " << batch << ", relation " << relation;
            EXPECT_EQ(tuples_of(evaluator.get_delta(i).removed), lost) << "batch " << batch << ", relation " << relation;
        }
    }
}
TEST(DatabaseDatalog, EvaluatorReadsOnlyWhatItsOutputsDependOn)
{
    // tc reads e alone: v's slot is not read, so a relation of the wrong arity there is fine.
    Fixture fixture;
    dl::Evaluator evaluator(layer(), fixture.program, std::array { fixture.tc });
    const std::vector<Tuples> inputs { { { 0, 1 }, { 1, 2 }, { 2, 0 } }, {} };
    const auto e = relation_of(inputs[0], 2), wrong = relation_of({ { 0, 0, 0 } }, 3);
    evaluator.evaluate(std::array { borrow(e), borrow(wrong) });
    EXPECT_EQ(tuples_of(evaluator.get_result()), Interpreter(fixture.program, inputs).get(fixture.tc.get_id()));
}

TEST(DatabaseDatalog, DeleteAndRederiveDropsRowsThatOnlySupportEachOther)
{
    // reach(x) <- start(x);  reach(y) <- reach(x), e(x, y), over a <-> cycle b -> c -> b entered from a.
    Programs p;
    const auto e = p.relation(100, 2, 0), start = p.relation(101, 1, 1), reach = p.relation(0, 1);
    constexpr uint_t x = 0, y = 1, a = 0, b = 1, c = 2, d = 3;
    const auto program = p.program({ p.rule(p.atom(reach, { var(x) }), { p.atom(start, { var(x) }) }),
                                     p.rule(p.atom(reach, { var(y) }), { p.atom(reach, { var(x) }), p.atom(e, { var(x), var(y) }) }) });
    dl::Evaluator evaluator(layer(), program, std::array { reach });
    const auto edges = relation_of({ { a, b }, { b, c }, { c, b }, { d, c } }, 2), starts = relation_of({ { a } }, 1);
    evaluator.initialize(std::array { borrow(edges), borrow(starts) });
    EXPECT_EQ(tuples_of(evaluator.get_result()), (Tuples { { a }, { b }, { c } }));
    const auto none2 = relation_of({}, 2), none1 = relation_of({}, 1);
    // Removing a -> b leaves b and c supporting each other only: both go.
    const auto cut = relation_of({ { a, b } }, 2);
    evaluator.update(std::array { Change { borrow(none2), borrow(cut) }, Change { borrow(none1), borrow(none1) } });
    EXPECT_EQ(tuples_of(evaluator.get_result()), (Tuples { { a } }));
    EXPECT_EQ(tuples_of(evaluator.get_delta().removed), (Tuples { { b }, { c } }));
    // Starting from d as well, then cutting a's other edge: c and b are rederived through d.
    const auto from_d = relation_of({ { d } }, 1);
    evaluator.update(std::array { Change { borrow(cut), borrow(none2) }, Change { borrow(from_d), borrow(none1) } });
    EXPECT_EQ(tuples_of(evaluator.get_result()), (Tuples { { a }, { b }, { c }, { d } }));
    evaluator.update(std::array { Change { borrow(none2), borrow(cut) }, Change { borrow(none1), borrow(none1) } });
    EXPECT_EQ(tuples_of(evaluator.get_result()), (Tuples { { a }, { b }, { c }, { d } }));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(DatabaseDatalog, WeightedGoalStopsAtTheLeastTarget)
{
    // dist: costs y + 1 per edge from the sources; best: the least dist over the targets, the only output, so the
    // stratum of dist stops once a target settles. Equal to the naive interpreter, in full evaluation and in updates,
    // also of the targets alone.
    Programs p;
    const auto e = p.relation(100, 2, 0), source = p.relation(101, 1, 1), target = p.relation(102, 1, 2);
    const auto dist = p.weighted(0, 2), best = p.weighted(1, 1);
    constexpr uint_t x = 0, y = 1, w = 2, c = 3, v = 4;
    using Operation = dl::Arithmetic::Operation;
    const auto program = p.program({
        p.rule(p.atom(dist, { var(x), obj(0) }), { p.atom(source, { var(x) }) }),
        p.rule(p.atom(dist, { var(y), var(v) }),
               { p.atom(dist, { var(x), var(w) }), p.atom(e, { var(x), var(y) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(y), obj(1), c }, dl::Arithmetic { Operation::ADD, var(w), var(c), v } }),
        p.rule(p.atom(best, { var(w) }), { p.atom(dist, { var(x), var(w) }), p.atom(target, { var(x) }) }),
    });
    dl::Evaluator evaluator(layer(), program, std::array { best });
    std::mt19937 random(17);
    const auto draw = [&](size_t count, size_t arity)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % 8));
            tuples.insert(tuple);
        }
        return tuples;
    };
    auto current = std::vector<Tuples>(3);
    evaluator.initialize(std::array { borrow(relation_of(current[0], 2)), borrow(relation_of(current[1], 1)), borrow(relation_of(current[2], 1)) });
    for (size_t trial = 0; trial < 200; ++trial)
    {
        auto inputs = std::vector<Tuples> { draw(random() % 16, 2), draw(1 + random() % 2, 1), draw(random() % 3, 1) };
        if (trial % 4 == 3)  // Only the targets change: where the stratum stopped depends on them.
        {
            inputs[0] = current[0];
            inputs[1] = current[1];
        }
        auto relations = std::vector<Relation>();
        for (size_t slot = 0; slot < 3; ++slot)
            relations.push_back(relation_of(inputs[slot], slot == 0 ? 2 : 1));
        const Interpreter expected(program, inputs);
        if (trial % 2 == 0)
            evaluator.evaluate(std::array { borrow(relations[0]), borrow(relations[1]), borrow(relations[2]) });
        else
        {
            auto added = std::vector<Relation>(), removed = std::vector<Relation>();
            for (size_t slot = 0; slot < 3; ++slot)
            {
                Tuples a, r;
                std::ranges::set_difference(inputs[slot], current[slot], std::inserter(a, a.end()));
                std::ranges::set_difference(current[slot], inputs[slot], std::inserter(r, r.end()));
                added.push_back(relation_of(a, slot == 0 ? 2 : 1));
                removed.push_back(relation_of(r, slot == 0 ? 2 : 1));
            }
            evaluator.update(std::array { Change { borrow(added[0]), borrow(removed[0]) },
                                          Change { borrow(added[1]), borrow(removed[1]) },
                                          Change { borrow(added[2]), borrow(removed[2]) } });
        }
        EXPECT_EQ(tuples_of(evaluator.get_result()), expected.get(1)) << "trial " << trial;
        current = inputs;
        if (trial % 2 == 0)
        {
            // evaluate() does not keep what update() needs; initialize again for the next update.
            evaluator.initialize(std::array { borrow(relations[0]), borrow(relations[1]), borrow(relations[2]) });
        }
    }
}

TEST(DatabaseDatalog, WeightedGoalSearchesWithAHeuristicFromStaticRules)
{
    // The edges are static, so A* searches with the abstract distances over them; sources and targets change per state.
    // dist also reads a fluent relation `open`, which the abstraction drops: its derivations include the concrete ones.
    Programs p;
    const auto e = p.relation(100, 2, 0), source = p.relation(101, 1, 1), target = p.relation(102, 1, 2), open = p.relation(103, 1, 3);
    const auto dist = p.weighted(0, 2), best = p.weighted(1, 1);
    constexpr uint_t x = 0, y = 1, w = 2, c = 3, v = 4;
    using Operation = dl::Arithmetic::Operation;
    const auto program = p.program({
        p.rule(p.atom(dist, { var(x), obj(0) }), { p.atom(source, { var(x) }) }),
        p.rule(p.atom(dist, { var(y), var(v) }),
               { p.atom(dist, { var(x), var(w) }), p.atom(e, { var(x), var(y) }), p.atom(open, { var(y) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(y), obj(1), c }, dl::Arithmetic { Operation::ADD, var(w), var(c), v } }),
        p.rule(p.atom(best, { var(w) }), { p.atom(dist, { var(x), var(w) }), p.atom(target, { var(x) }) }),
    });
    std::mt19937 random(23);
    const auto draw = [&](size_t count, size_t arity)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % 12));
            tuples.insert(tuple);
        }
        return tuples;
    };
    for (size_t graph = 0; graph < 20; ++graph)
    {
        const auto edges = draw(30, 2);
        const auto edge_relation = relation_of(edges, 2);
        constexpr auto static_slots = std::array<size_t, 1> { 0 };
        dl::Evaluator evaluator(layer(), program, std::array { best }, static_slots);
        for (size_t trial = 0; trial < 20; ++trial)
        {
            const auto inputs = std::vector<Tuples> { edges, draw(1 + random() % 2, 1), draw(1 + random() % 3, 1), draw(10, 1) };
            const auto s = relation_of(inputs[1], 1), t = relation_of(inputs[2], 1), o = relation_of(inputs[3], 1);
            evaluator.evaluate(std::array { borrow(edge_relation), borrow(s), borrow(t), borrow(o) });
            const Interpreter expected(program, inputs);
            EXPECT_EQ(tuples_of(evaluator.get_result()), expected.get(1)) << "graph " << graph << ", trial " << trial;
        }
    }
}

TEST(DatabaseDatalog, WeightedGoalStopsEachKeyAtItsLeastTarget)
{
    // dist per key k: from k's sources over the shared static edges and the fluent `open`; best: per key the least dist
    // over k's targets, so the stratum stops each key once one of its targets settles, searching with A*. Equal to the
    // naive interpreter, in full evaluation and in updates, also of the targets alone.
    Programs p;
    const auto e = p.relation(100, 2, 0), source = p.relation(101, 2, 1), target = p.relation(102, 2, 2), open = p.relation(103, 1, 3);
    const auto dist = p.weighted(0, 3), best = p.weighted(1, 2);
    constexpr uint_t k = 0, x = 1, y = 2, w = 3, c = 4, v = 5;
    using Operation = dl::Arithmetic::Operation;
    const auto program = p.program({
        p.rule(p.atom(dist, { var(k), var(x), obj(0) }), { p.atom(source, { var(k), var(x) }) }),
        p.rule(p.atom(dist, { var(k), var(y), var(v) }),
               { p.atom(dist, { var(k), var(x), var(w) }), p.atom(e, { var(x), var(y) }), p.atom(open, { var(y) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(y), obj(1), c }, dl::Arithmetic { Operation::ADD, var(w), var(c), v } }),
        p.rule(p.atom(best, { var(k), var(w) }), { p.atom(dist, { var(k), var(x), var(w) }), p.atom(target, { var(k), var(x) }) }),
    });
    std::mt19937 random(29);
    const auto draw = [&](size_t count, size_t arity, uint_t range)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % (a == 0 && arity == 2 && range == 4 ? 4 : 12)));
            tuples.insert(tuple);
        }
        return tuples;
    };
    for (size_t graph = 0; graph < 10; ++graph)
    {
        const auto edges = draw(30, 2, 12);
        constexpr auto static_slots = std::array<size_t, 1> { 0 };
        dl::Evaluator evaluator(layer(), program, std::array { best }, static_slots);
        auto current = std::vector<Tuples> { edges, {}, {}, {} };
        evaluator.initialize(std::array { borrow(relation_of(current[0], 2)),
                                          borrow(relation_of(current[1], 2)),
                                          borrow(relation_of(current[2], 2)),
                                          borrow(relation_of(current[3], 1)) });
        for (size_t trial = 0; trial < 40; ++trial)
        {
            auto inputs = std::vector<Tuples> { edges, draw(2 + random() % 4, 2, 4), draw(1 + random() % 6, 2, 4), draw(10, 1, 12) };
            if (trial % 4 == 3)  // Only the targets change.
            {
                inputs[1] = current[1];
                inputs[3] = current[3];
            }
            const Interpreter expected(program, inputs);
            if (trial % 2 == 0)
            {
                auto relations = std::vector<Relation>();
                for (size_t slot = 0; slot < 4; ++slot)
                    relations.push_back(relation_of(inputs[slot], slot == 3 ? 1 : 2));
                evaluator.initialize(std::array { borrow(relations[0]), borrow(relations[1]), borrow(relations[2]), borrow(relations[3]) });
            }
            else
            {
                auto added = std::vector<Relation>(), removed = std::vector<Relation>();
                for (size_t slot = 0; slot < 4; ++slot)
                {
                    Tuples a, r;
                    std::ranges::set_difference(inputs[slot], current[slot], std::inserter(a, a.end()));
                    std::ranges::set_difference(current[slot], inputs[slot], std::inserter(r, r.end()));
                    added.push_back(relation_of(a, slot == 3 ? 1 : 2));
                    removed.push_back(relation_of(r, slot == 3 ? 1 : 2));
                }
                evaluator.update(std::array { Change { borrow(added[0]), borrow(removed[0]) },
                                              Change { borrow(added[1]), borrow(removed[1]) },
                                              Change { borrow(added[2]), borrow(removed[2]) },
                                              Change { borrow(added[3]), borrow(removed[3]) } });
            }
            EXPECT_EQ(tuples_of(evaluator.get_result()), expected.get(1)) << "graph " << graph << ", trial " << trial;
            current = inputs;
        }
    }
}

TEST(DatabaseDatalog, WeightedGoalSearchesWithProjectionsOfTheTargets)
{
    // dist over pairs (x, y): x moves over the static `e`, y over the static `f` where the fluent `open` holds; best:
    // the least dist with y a target and x anything, so the heuristic projects the key onto y. Equal to the naive
    // interpreter.
    Programs p;
    const auto e = p.relation(100, 2, 0), f = p.relation(101, 2, 1), source = p.relation(102, 2, 2), target = p.relation(103, 1, 3),
               open = p.relation(104, 1, 4);
    const auto dist = p.weighted(0, 3), best = p.weighted(1, 1);
    constexpr uint_t x = 0, y = 1, z = 2, w = 3, v = 4;
    using Operation = dl::Arithmetic::Operation;
    const auto program = p.program({
        p.rule(p.atom(dist, { var(x), var(y), obj(0) }), { p.atom(source, { var(x), var(y) }) }),
        p.rule(p.atom(dist, { var(z), var(y), var(v) }),
               { p.atom(dist, { var(x), var(y), var(w) }), p.atom(e, { var(x), var(z) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(w), obj(1), v } }),
        p.rule(p.atom(dist, { var(x), var(z), var(v) }),
               { p.atom(dist, { var(x), var(y), var(w) }), p.atom(f, { var(y), var(z) }), p.atom(open, { var(z) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(w), obj(2), v } }),
        p.rule(p.atom(best, { var(w) }), { p.atom(dist, { var(x), var(y), var(w) }), p.atom(target, { var(y) }) }),
    });
    std::mt19937 random(31);
    const auto draw = [&](size_t count, size_t arity)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % 8));
            tuples.insert(tuple);
        }
        return tuples;
    };
    for (size_t graph = 0; graph < 10; ++graph)
    {
        const auto edges = draw(14, 2), fills = draw(14, 2);
        const auto e_relation = relation_of(edges, 2), f_relation = relation_of(fills, 2);
        constexpr auto static_slots = std::array<size_t, 2> { 0, 1 };
        dl::Evaluator evaluator(layer(), program, std::array { best }, static_slots);
        for (size_t trial = 0; trial < 20; ++trial)
        {
            const auto inputs = std::vector<Tuples> { edges, fills, draw(1 + random() % 2, 2), draw(1 + random() % 2, 1), draw(6, 1) };
            const auto s = relation_of(inputs[2], 2), t = relation_of(inputs[3], 1), o = relation_of(inputs[4], 1);
            evaluator.evaluate(std::array { borrow(e_relation), borrow(f_relation), borrow(s), borrow(t), borrow(o) });
            const Interpreter expected(program, inputs);
            EXPECT_EQ(tuples_of(evaluator.get_result()), expected.get(1)) << "graph " << graph << ", trial " << trial;
        }
    }
}

TEST(DatabaseDatalog, UnitWeightedGoalReusesItsSearchesAcrossUpdates)
{
    // dist per key k with unit costs from k's sources over the static edges where the fluent `open` holds: unit-cost
    // searches, kept across updates while k's targets stay the same. Most updates change `open` alone, which adds
    // and removes edges; some change sources or targets. Equal to the naive interpreter after every update.
    Programs p;
    const auto e = p.relation(100, 2, 0), source = p.relation(101, 2, 1), target = p.relation(102, 2, 2), open = p.relation(103, 1, 3);
    const auto dist = p.weighted(0, 3), best = p.weighted(1, 2);
    constexpr uint_t k = 0, x = 1, y = 2, w = 3, v = 4;
    using Operation = dl::Arithmetic::Operation;
    const auto program = p.program({
        p.rule(p.atom(dist, { var(k), var(x), obj(0) }), { p.atom(source, { var(k), var(x) }) }),
        p.rule(p.atom(dist, { var(k), var(y), var(v) }),
               { p.atom(dist, { var(k), var(x), var(w) }), p.atom(e, { var(x), var(y) }), p.atom(open, { var(y) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(w), obj(1), v } }),
        p.rule(p.atom(best, { var(k), var(w) }), { p.atom(dist, { var(k), var(x), var(w) }), p.atom(target, { var(k), var(x) }) }),
    });
    std::mt19937 random(37);
    const auto draw = [&](size_t count, size_t arity, uint_t key_range)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % (a == 0 && arity == 2 ? key_range : 30)));
            tuples.insert(tuple);
        }
        return tuples;
    };
    for (size_t graph = 0; graph < 20; ++graph)
    {
        const auto edges = draw(70, 2, 30);
        constexpr auto static_slots = std::array<size_t, 1> { 0 };
        dl::Evaluator evaluator(layer(), program, std::array { best }, static_slots);
        auto current = std::vector<Tuples> { edges, draw(4, 2, 3), draw(4, 2, 3), draw(24, 1, 30) };
        evaluator.initialize(std::array { borrow(relation_of(current[0], 2)),
                                          borrow(relation_of(current[1], 2)),
                                          borrow(relation_of(current[2], 2)),
                                          borrow(relation_of(current[3], 1)) });
        EXPECT_EQ(tuples_of(evaluator.get_result()), Interpreter(program, current).get(1)) << "graph " << graph;
        for (size_t trial = 0; trial < 60; ++trial)
        {
            auto inputs = current;
            // Toggle a few cells of `open`; every fifth update also redraws the sources, every seventh the targets.
            for (size_t i = 0; i < 3; ++i)
            {
                const auto cell = Tuple { static_cast<uint_t>(random() % 30) };
                if (!inputs[3].erase(cell))
                    inputs[3].insert(cell);
            }
            if (trial % 5 == 4)
                inputs[1] = draw(4, 2, 3);
            if (trial % 7 == 6)
                inputs[2] = draw(4, 2, 3);
            auto added = std::vector<Relation>(), removed = std::vector<Relation>();
            for (size_t slot = 0; slot < 4; ++slot)
            {
                Tuples a, r;
                std::ranges::set_difference(inputs[slot], current[slot], std::inserter(a, a.end()));
                std::ranges::set_difference(current[slot], inputs[slot], std::inserter(r, r.end()));
                added.push_back(relation_of(a, slot == 3 ? 1 : 2));
                removed.push_back(relation_of(r, slot == 3 ? 1 : 2));
            }
            evaluator.update(std::array { Change { borrow(added[0]), borrow(removed[0]) },
                                          Change { borrow(added[1]), borrow(removed[1]) },
                                          Change { borrow(added[2]), borrow(removed[2]) },
                                          Change { borrow(added[3]), borrow(removed[3]) } });
            EXPECT_EQ(tuples_of(evaluator.get_result()), Interpreter(program, inputs).get(1)) << "graph " << graph << ", trial " << trial;
            current = inputs;
        }
    }
}

// As UnitWeightedGoalReusesItsSearchesAcrossUpdates, but only whether each key reaches a target is read: the searches
// end at the first path (any_path) and restart their backward frontiers, whose found vertices removed edges would
// leave stale. Toggling `open` adds and removes edges, so keys switch between reaching a target and not.
TEST(DatabaseDatalog, UnitWeightedGoalWhoseWeightIsNotReadUnderUpdates)
{
    // dist per key k with unit costs from k's sources over the static edges where the fluent `open` holds: unit-cost
    // searches, kept across updates while k's targets stay the same. Most updates change `open` alone, which adds
    // and removes edges; some change sources or targets. Equal to the naive interpreter after every update.
    Programs p;
    const auto e = p.relation(100, 2, 0), source = p.relation(101, 2, 1), target = p.relation(102, 2, 2), open = p.relation(103, 1, 3);
    const auto dist = p.weighted(0, 3), best = p.weighted(1, 2), reached = p.relation(2, 1);
    constexpr uint_t k = 0, x = 1, y = 2, w = 3, v = 4;
    using Operation = dl::Arithmetic::Operation;
    const auto program = p.program({
        p.rule(p.atom(dist, { var(k), var(x), obj(0) }), { p.atom(source, { var(k), var(x) }) }),
        p.rule(p.atom(dist, { var(k), var(y), var(v) }),
               { p.atom(dist, { var(k), var(x), var(w) }), p.atom(e, { var(x), var(y) }), p.atom(open, { var(y) }) },
               {},
               {},
               {},
               { dl::Arithmetic { Operation::ADD, var(w), obj(1), v } }),
        p.rule(p.atom(best, { var(k), var(w) }), { p.atom(dist, { var(k), var(x), var(w) }), p.atom(target, { var(k), var(x) }) }),
        p.rule(p.atom(reached, { var(k) }), { p.atom(best, { var(k), var(w) }) }),
    });
    std::mt19937 random(37);
    const auto draw = [&](size_t count, size_t arity, uint_t key_range)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % (a == 0 && arity == 2 ? key_range : 30)));
            tuples.insert(tuple);
        }
        return tuples;
    };
    for (size_t graph = 0; graph < 20; ++graph)
    {
        const auto edges = draw(70, 2, 30);
        constexpr auto static_slots = std::array<size_t, 1> { 0 };
        dl::Evaluator evaluator(layer(), program, std::array { reached }, static_slots);
        auto current = std::vector<Tuples> { edges, draw(4, 2, 3), draw(4, 2, 3), draw(24, 1, 30) };
        evaluator.initialize(std::array { borrow(relation_of(current[0], 2)),
                                          borrow(relation_of(current[1], 2)),
                                          borrow(relation_of(current[2], 2)),
                                          borrow(relation_of(current[3], 1)) });
        EXPECT_EQ(tuples_of(evaluator.get_result()), Interpreter(program, current).get(2)) << "graph " << graph;
        for (size_t trial = 0; trial < 60; ++trial)
        {
            auto inputs = current;
            // Toggle a few cells of `open`; every fifth update also redraws the sources, every seventh the targets.
            for (size_t i = 0; i < 3; ++i)
            {
                const auto cell = Tuple { static_cast<uint_t>(random() % 30) };
                if (!inputs[3].erase(cell))
                    inputs[3].insert(cell);
            }
            if (trial % 5 == 4)
                inputs[1] = draw(4, 2, 3);
            if (trial % 7 == 6)
                inputs[2] = draw(4, 2, 3);
            auto added = std::vector<Relation>(), removed = std::vector<Relation>();
            for (size_t slot = 0; slot < 4; ++slot)
            {
                Tuples a, r;
                std::ranges::set_difference(inputs[slot], current[slot], std::inserter(a, a.end()));
                std::ranges::set_difference(current[slot], inputs[slot], std::inserter(r, r.end()));
                added.push_back(relation_of(a, slot == 3 ? 1 : 2));
                removed.push_back(relation_of(r, slot == 3 ? 1 : 2));
            }
            evaluator.update(std::array { Change { borrow(added[0]), borrow(removed[0]) },
                                          Change { borrow(added[1]), borrow(removed[1]) },
                                          Change { borrow(added[2]), borrow(removed[2]) },
                                          Change { borrow(added[3]), borrow(removed[3]) } });
            EXPECT_EQ(tuples_of(evaluator.get_result()), Interpreter(program, inputs).get(2)) << "graph " << graph << ", trial " << trial;
            current = inputs;
        }
    }
}

/// The probes that every path from a key's sources to its targets passes, over the static edges into cells where `open`
/// holds; per key and probe, by breadth-first search: whether a target is reachable, and whether it is reachable
/// avoiding the probe. Independent of the evaluator and of the program.
Tuples must_pass_of(const std::vector<Tuples>& inputs)
{
    const auto& [edges, sources, targets, open, probes] = std::tie(inputs[0], inputs[1], inputs[2], inputs[3], inputs[4]);
    const auto reaches = [&](uint_t key, std::optional<uint_t> avoided)
    {
        auto seen = std::set<uint_t>();
        auto queue = std::vector<uint_t>();
        for (const auto& source : sources)
            if (source[0] == key && source[1] != avoided && seen.insert(source[1]).second)
                queue.push_back(source[1]);
        for (size_t i = 0; i < queue.size(); ++i)
        {
            if (targets.contains(Tuple { key, queue[i] }))
                return true;
            for (const auto& edge : edges)
                if (edge[0] == queue[i] && edge[1] != avoided && open.contains(Tuple { edge[1] }) && seen.insert(edge[1]).second)
                    queue.push_back(edge[1]);
        }
        return false;
    };
    auto result = Tuples();
    for (const auto& source : sources)
        if (reaches(source[0], std::nullopt))
            for (const auto& probe : probes)
                if (!reaches(source[0], probe[0]))
                    result.insert(Tuple { source[0], probe[0] });
    return result;
}

TEST(DatabaseDatalog, MustPassProbesUnderUpdates)
{
    // must_pass(k, c): every path from k's sources to its targets passes the probe c, the targets being reachable. Its
    // reachability goals read no weight, so they are searched for any path. Equal to the naive interpreter and to
    // must_pass_of after every update.
    Programs p;
    const auto e = p.relation(100, 2, 0), source = p.relation(101, 2, 1), target = p.relation(102, 2, 2), open = p.relation(103, 1, 3),
               probe = p.relation(104, 1, 4);
    const auto reach = p.weighted(0, 3), reachable = p.weighted(1, 2), avoid = p.weighted(2, 4), escapes = p.weighted(3, 3);
    const auto escaped = p.relation(4, 2), must_pass = p.relation(5, 2);
    constexpr uint_t k = 0, x = 1, y = 2, w = 3, v = 4, c = 5;
    using Operation = dl::Arithmetic::Operation;
    using Kind = dl::Comparison::Kind;
    const auto step = dl::Arithmetic { Operation::ADD, var(w), obj(1), v };
    const auto program = p.program({
        p.rule(p.atom(reach, { var(k), var(x), obj(0) }), { p.atom(source, { var(k), var(x) }) }),
        p.rule(p.atom(reach, { var(k), var(y), var(v) }),
               { p.atom(reach, { var(k), var(x), var(w) }), p.atom(e, { var(x), var(y) }), p.atom(open, { var(y) }) },
               {},
               {},
               {},
               { step }),
        p.rule(p.atom(reachable, { var(k), var(w) }), { p.atom(reach, { var(k), var(x), var(w) }), p.atom(target, { var(k), var(x) }) }),
        p.rule(p.atom(avoid, { var(k), var(c), var(x), obj(0) }),
               { p.atom(source, { var(k), var(x) }), p.atom(probe, { var(c) }) },
               {},
               { dl::Comparison { Kind::NOT_EQUAL, var(x), var(c) } }),
        p.rule(p.atom(avoid, { var(k), var(c), var(y), var(v) }),
               { p.atom(avoid, { var(k), var(c), var(x), var(w) }), p.atom(e, { var(x), var(y) }), p.atom(open, { var(y) }) },
               {},
               { dl::Comparison { Kind::NOT_EQUAL, var(y), var(c) } },
               {},
               { step }),
        p.rule(p.atom(escapes, { var(k), var(c), var(w) }), { p.atom(avoid, { var(k), var(c), var(x), var(w) }), p.atom(target, { var(k), var(x) }) }),
        p.rule(p.atom(escaped, { var(k), var(c) }), { p.atom(escapes, { var(k), var(c), var(w) }) }),
        p.rule(p.atom(must_pass, { var(k), var(c) }),
               { p.atom(reachable, { var(k), var(w) }), p.atom(probe, { var(c) }) },
               { p.atom(escaped, { var(k), var(c) }) }),
    });
    std::mt19937 random(41);
    const auto draw = [&](size_t count, size_t arity, uint_t key_range)
    {
        auto tuples = Tuples();
        for (size_t i = 0; i < count; ++i)
        {
            auto tuple = Tuple();
            for (size_t a = 0; a < arity; ++a)
                tuple.push_back(static_cast<uint_t>(random() % (a == 0 && arity == 2 ? key_range : 30)));
            tuples.insert(tuple);
        }
        return tuples;
    };
    const auto arity = [](size_t slot) { return slot == 3 || slot == 4 ? size_t { 1 } : size_t { 2 }; };
    auto must_pass_rows = size_t { 0 };
    for (size_t graph = 0; graph < 20; ++graph)
    {
        constexpr auto static_slots = std::array<size_t, 2> { 0, 4 };
        dl::Evaluator evaluator(layer(), program, std::array { must_pass }, static_slots);
        auto current = std::vector<Tuples> { draw(60, 2, 30), draw(4, 2, 3), draw(4, 2, 3), draw(26, 1, 30), draw(10, 1, 30) };
        const auto read = [&](const std::vector<Tuples>& inputs)
        {
            auto relations = std::vector<Relation>();
            for (size_t slot = 0; slot < inputs.size(); ++slot)
                relations.push_back(relation_of(inputs[slot], arity(slot)));
            return relations;
        };
        const auto relations = read(current);
        evaluator.initialize(std::array { borrow(relations[0]), borrow(relations[1]), borrow(relations[2]), borrow(relations[3]), borrow(relations[4]) });
        EXPECT_EQ(tuples_of(evaluator.get_result()), Interpreter(program, current).get(5)) << "graph " << graph;
        EXPECT_EQ(tuples_of(evaluator.get_result()), must_pass_of(current)) << "graph " << graph;
        for (size_t trial = 0; trial < 40; ++trial)
        {
            auto inputs = current;
            // Toggle a few cells of `open`; every fifth update also redraws the sources, every seventh the targets.
            for (size_t i = 0; i < 3; ++i)
            {
                const auto cell = Tuple { static_cast<uint_t>(random() % 30) };
                if (!inputs[3].erase(cell))
                    inputs[3].insert(cell);
            }
            if (trial % 5 == 4)
                inputs[1] = draw(4, 2, 3);
            if (trial % 7 == 6)
                inputs[2] = draw(4, 2, 3);
            auto added = std::vector<Relation>(), removed = std::vector<Relation>();
            for (size_t slot = 0; slot < inputs.size(); ++slot)
            {
                Tuples a, r;
                std::ranges::set_difference(inputs[slot], current[slot], std::inserter(a, a.end()));
                std::ranges::set_difference(current[slot], inputs[slot], std::inserter(r, r.end()));
                added.push_back(relation_of(a, arity(slot)));
                removed.push_back(relation_of(r, arity(slot)));
            }
            evaluator.update(std::array { Change { borrow(added[0]), borrow(removed[0]) },
                                          Change { borrow(added[1]), borrow(removed[1]) },
                                          Change { borrow(added[2]), borrow(removed[2]) },
                                          Change { borrow(added[3]), borrow(removed[3]) },
                                          Change { borrow(added[4]), borrow(removed[4]) } });
            const auto result = tuples_of(evaluator.get_result());
            EXPECT_EQ(result, Interpreter(program, inputs).get(5)) << "graph " << graph << ", trial " << trial;
            EXPECT_EQ(result, must_pass_of(inputs)) << "graph " << graph << ", trial " << trial;
            must_pass_rows += result.size();
            current = inputs;
        }
    }
    EXPECT_GT(must_pass_rows, 0);
}

TEST(DatabaseDatalog, DeleteAndRederiveKeepsRowsWithAnotherDerivation)
{
    // reach over the path 0 -> 1 -> ... -> 199 with a shortcut 190 -> 195. Cutting 192 -> 193 over-deletes 193, ..., 199,
    // few enough of the rows to rederive instead of evaluating anew; 195, ..., 199 are rederived through the shortcut.
    Programs p;
    const auto e = p.relation(100, 2, 0), start = p.relation(101, 1, 1), reach = p.relation(0, 1);
    constexpr uint_t x = 0, y = 1, n = 200;
    const auto program = p.program({ p.rule(p.atom(reach, { var(x) }), { p.atom(start, { var(x) }) }),
                                     p.rule(p.atom(reach, { var(y) }), { p.atom(reach, { var(x) }), p.atom(e, { var(x), var(y) }) }) });
    dl::Evaluator evaluator(layer(), program, std::array { reach });
    auto path = Tuples {};
    for (uint_t v = 0; v + 1 < n; ++v)
        path.insert({ v, v + 1 });
    path.insert({ 190, 195 });
    const auto edges = relation_of(path, 2), starts = relation_of({ { 0 } }, 1);
    evaluator.initialize(std::array { borrow(edges), borrow(starts) });
    EXPECT_EQ(evaluator.get_result().size(), n);
    const auto none2 = relation_of({}, 2), none1 = relation_of({}, 1), cut = relation_of({ { 192, 193 } }, 2);
    evaluator.update(std::array { Change { borrow(none2), borrow(cut) }, Change { borrow(none1), borrow(none1) } });
    EXPECT_EQ(tuples_of(evaluator.get_delta().removed), (Tuples { { 193 }, { 194 } }));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_EQ(evaluator.get_result().size(), n - 2);
}

/// Object columns are read as their index, so that relations of object rows are inputs; other column types are rejected.
TEST(DatabaseDatalog, ReadsObjectColumnsAsTheirIndex)
{
    Programs p;
    const auto e = p.relation(100, 2, 0), copy = p.relation(0, 2);
    constexpr uint_t x = 0, y = 1;
    const auto program = p.program({ p.rule(p.atom(copy, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }) });
    dl::Evaluator evaluator(layer(), program, std::array { copy });

    using Object = urd::index<ygg::formalism::Object<void>>;
    const auto columns = std::array { ygg::ColumnLayout { urd::index<ygg::Column>(0), ygg::type_id<Object> },
                                      ygg::ColumnLayout { urd::index<ygg::Column>(1), ygg::type_id<Object> } };
    auto objects = Relation { std::span<const ygg::ColumnLayout>(columns) };
    for (const auto& [from, to] : std::array<std::pair<uint_t, uint_t>, 2> { { { 3, 5 }, { 5, 8 } } })
    {
        const auto values = std::array { ygg::encode_value(Object(from)), ygg::encode_value(Object(to)) };
        objects.insert(std::span<const ygg::Value>(values));
    }
    evaluator.evaluate(std::array { borrow(objects) });
    EXPECT_EQ(tuples_of(evaluator.get_result()), (Tuples { { 3, 5 }, { 5, 8 } }));

    const auto doubles = std::array { ygg::ColumnLayout { urd::index<ygg::Column>(0), ygg::type_id<double> },
                                      ygg::ColumnLayout { urd::index<ygg::Column>(1), ygg::type_id<double> } };
    const auto other = Relation { std::span<const ygg::ColumnLayout>(doubles) };
    EXPECT_THROW(evaluator.evaluate(std::array { borrow(other) }), std::invalid_argument);
}

TEST(DatabaseDatalog, RejectsInvalidRules)
{
    Programs p;
    const auto e = p.relation(100, 2, 0);
    const auto r = p.relation(0, 2), s = p.relation(1, 1);
    constexpr uint_t x = 0, y = 1, z = 2, n = 3;
    EXPECT_THROW(p.atom(e, { var(x) }), std::invalid_argument);                                                     // arity
    EXPECT_THROW(p.rule(p.atom(e, { var(x), var(y) }), { p.atom(e, { var(x), var(y) }) }), std::invalid_argument);  // input head
    EXPECT_THROW(p.rule(p.atom(r, { var(x), var(x) }), { p.atom(e, { var(x), var(x) }) }), std::invalid_argument);  // repeated head
    EXPECT_THROW(p.rule(p.atom(r, { var(x), obj(1) }), { p.atom(e, { var(x), var(y) }) }), std::invalid_argument);  // constant head
    EXPECT_THROW(p.rule(p.atom(r, { var(x), var(z) }), { p.atom(e, { var(x), var(y) }) }), std::invalid_argument);  // unsafe head
    EXPECT_THROW(p.rule(p.atom(s, { var(x) }), { p.atom(e, { var(x), var(y) }) }, { p.atom(e, { var(z), var(x) }) }),
                 std::invalid_argument);                                                                         // unsafe negation
    EXPECT_THROW(p.aggregate(Function::SUM, n, z, { p.atom(e, { var(x), var(y) }) }), std::invalid_argument);    // unbound value
    EXPECT_THROW(p.aggregate(Function::COUNT, x, 0, { p.atom(e, { var(x), var(y) }) }), std::invalid_argument);  // result in body

    // Not stratified: r depends on itself through negation.
    const auto program = p.program({ p.rule(p.atom(s, { var(x) }), { p.atom(e, { var(x), var(y) }) }, { p.atom(s, { var(y) }) }) });
    EXPECT_THROW(dl::Evaluator(layer(), program, std::array { s }), std::invalid_argument);

    // A weighted rule must not filter on, join on or subtract from a weight, nor ignore one; weighted and unweighted
    // relations are not recursive together.
    using Operation = dl::Arithmetic::Operation;
    const auto d = p.weighted(40, 2), u = p.relation(41, 1);
    const auto base = p.rule(p.atom(d, { var(x), obj(0) }), { p.atom(e, { var(x), var(y) }) });
    const auto invalid = [&](dl::RuleView rule) { EXPECT_THROW(dl::Evaluator(layer(), p.program({ base, rule }), std::array { d }), std::invalid_argument); };
    invalid(p.rule(p.atom(d, { var(z), var(n) }),
                   { p.atom(d, { var(y), var(x) }), p.atom(e, { var(y), var(z) }) },
                   {},
                   { dl::Comparison { dl::Comparison::Kind::LESS, var(x), obj(3) } },
                   {},
                   { dl::Arithmetic { Operation::ADD, var(x), obj(1), n } }));
    invalid(p.rule(p.atom(d, { var(z), var(n) }),
                   { p.atom(d, { var(y), var(x) }), p.atom(e, { var(y), var(z) }) },
                   {},
                   {},
                   {},
                   { dl::Arithmetic { Operation::SUBTRACT, var(x), obj(1), n } }));
    invalid(p.rule(p.atom(d, { var(z), obj(1) }), { p.atom(d, { var(y), var(x) }), p.atom(e, { var(y), var(z) }) }));
    invalid(p.rule(p.atom(d, { var(y), var(x) }), { p.atom(d, { var(y), var(x) }), p.atom(e, { var(x), var(y) }) }));
    const auto mixed = p.program({ base,
                                   p.rule(p.atom(u, { var(y) }), { p.atom(d, { var(y), var(x) }) }),
                                   p.rule(p.atom(d, { var(y), var(x) }), { p.atom(u, { var(y) }), p.atom(e, { var(y), var(x) }) }) });
    EXPECT_THROW(dl::Evaluator(layer(), mixed, std::array { d }), std::invalid_argument);
}
