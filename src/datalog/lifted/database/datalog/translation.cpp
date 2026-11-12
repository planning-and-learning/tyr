/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/goal_search.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/program.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/stratification.hpp"

#include <algorithm>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <urd/repr/maps/flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/flat_set.hpp>
#include <urd/store.hpp>

/// Translates a program's records into tables and rules over variable slots, and groups them into strata.
namespace tyr::datalog::database::datalog::detail
{
namespace
{
class Translator
{
public:
    [[nodiscard]] CompiledProgram& program() noexcept { return m_program; }
    [[nodiscard]] const urd::Store<urd::sequence<Dependency>>& dependencies() const noexcept { return m_dependencies; }
    [[nodiscard]] urd::Store<urd::sequence<AggregateTable>>& aggregates() noexcept { return m_aggregates; }

    size_t table_of(RelationView relation)
    {
        auto& map = relation.is_input() ? m_input_tables : m_derived_tables;
        const auto key = relation.is_input() ? relation.get_input_slot() : relation.get_id();
        if (const auto it = map.find(key); it != map.end())
            return it->second;
        const auto table = add_table(relation.get_arity());
        m_program.tables[table]->weighted = relation.is_weighted();
        map.emplace(key, table);
        if (relation.is_input())
        {
            m_program.tables[table]->slot = key;
            if (m_program.slot_tables.size() <= key)
                m_program.slot_tables.resize(key + 1, npos);
            m_program.slot_tables[key] = table;
        }
        return table;
    }

    void add_rule(RuleView view)
    {
        auto rule = CompiledRule {};
        auto slot_of = urd::Store<urd::map<ygg::uint_t, ygg::uint_t>, urd::sorted>();
        // Positive atoms first, so that the outer variables an aggregate groups by are known.
        auto positives = urd::Store<urd::sequence<Literal>>();
        body(view.get_atoms(), std::span<const AtomView>(), {}, {}, slot_of, positives);
        auto outer = urd::Store<urd::set<ygg::uint_t>, urd::sorted>();
        for (const auto atom : view.get_atoms())
            for (const auto t : atom.get_terms())
                if (t.is_variable())
                    outer.insert(t.value);
        auto aggregate_literals = urd::Store<urd::sequence<Literal>>();
        for (const auto aggregate : view.get_aggregates())
            aggregate_literals.push_back(add_aggregate(aggregate, outer, slot_of));
        auto others = urd::Store<urd::sequence<Literal>>();
        body(std::span<const AtomView>(), view.get_negated(), view.get_comparisons(), view.get_arithmetic(), slot_of, others);
        rule.literals = std::move(positives);
        rule.literals.insert(rule.literals.end(), aggregate_literals.begin(), aggregate_literals.end());
        rule.literals.insert(rule.literals.end(), others.begin(), others.end());
        const auto head = view.get_head();
        rule.head = table_of(head.get_relation());
        for (const auto t : head.get_terms())
            rule.head_terms.push_back(t.is_variable() ? Term::variable(slot_of.at(t.value)) : t);
        rule.slots = slot_of.size();
        finish_rule(std::move(rule));
    }

private:
    CompiledProgram m_program;
    urd::Store<urd::sequence<Dependency>> m_dependencies;
    urd::Store<urd::sequence<AggregateTable>> m_aggregates;
    urd::Store<urd::map<size_t, size_t>, urd::sorted> m_derived_tables;
    urd::Store<urd::map<size_t, size_t>, urd::sorted> m_input_tables;

    size_t add_table(size_t arity)
    {
        m_program.tables.push_back(std::make_unique<Table>(arity));
        return m_program.tables.size() - 1;
    }

    /// The literals of a body; variables get slots of `slot_of`.
    template<typename Atoms, typename Negated>
    void body(const Atoms& atoms,
              const Negated& negated,
              std::span<const Comparison> comparisons,
              std::span<const Arithmetic> arithmetic,
              urd::Store<urd::map<ygg::uint_t, ygg::uint_t>, urd::sorted>& slot_of,
              urd::Store<urd::sequence<Literal>>& out)
    {
        const auto term = [&](Term t)
        {
            if (!t.is_variable())
                return t;
            const auto [it, inserted] = slot_of.try_emplace(t.value, ygg::to_uint_t(slot_of.size()));
            return Term::variable(it->second);
        };
        const auto literal = [&](Literal::Kind kind, AtomView atom)
        {
            auto result = Literal { kind, table_of(atom.get_relation()), {} };
            for (const auto t : atom.get_terms())
                result.terms.push_back(term(t));
            return result;
        };
        for (const auto atom : atoms)
            out.push_back(literal(Literal::Kind::POSITIVE, atom));
        for (const auto atom : negated)
            out.push_back(literal(Literal::Kind::NEGATED, atom));
        for (const auto& operation : arithmetic)
        {
            auto result = Literal { Literal::Kind::ARITHMETIC, npos, { term(operation.lhs), term(operation.rhs), term(Term::variable(operation.result)) } };
            result.operation = operation.operation;
            out.push_back(std::move(result));
        }
        for (const auto& comparison : comparisons)
        {
            auto result = Literal { Literal::Kind::COMPARISON, npos, { term(comparison.lhs), term(comparison.rhs) } };
            result.comparison = comparison.kind;
            out.push_back(std::move(result));
        }
    }

    void finish_rule(CompiledRule rule)
    {
        for (const auto& literal : rule.literals)
            if (literal.table != npos)
                m_dependencies.push_back(Dependency { rule.head, literal.table, literal.kind != Literal::Kind::POSITIVE });
        m_program.rules.push_back(std::move(rule));
    }

    /// An aggregate reads the table of its body's satisfying assignments (a rule of its own) and keeps the groups of
    /// that table; the enclosing rule reads its groups with the variables both bind.
    Literal add_aggregate(AggregateView view,
                          const urd::Store<urd::set<ygg::uint_t>, urd::sorted>& outer,
                          urd::Store<urd::map<ygg::uint_t, ygg::uint_t>, urd::sorted>& outer_slots)
    {
        auto inner_slots = urd::Store<urd::map<ygg::uint_t, ygg::uint_t>, urd::sorted>();
        auto inner = CompiledRule {};
        body(view.get_atoms(), view.get_negated(), view.get_comparisons(), view.get_arithmetic(), inner_slots, inner.literals);
        auto variables = urd::Store<urd::sequence<ygg::uint_t>>();  // In slot order.
        variables.resize(inner_slots.size());
        for (const auto& [variable, slot] : inner_slots)
            variables[slot] = variable;
        const auto source = add_table(variables.size());
        inner.head = source;
        for (ygg::uint_t slot = 0; slot < variables.size(); ++slot)
            inner.head_terms.push_back(Term::variable(slot));
        inner.slots = inner_slots.size();
        finish_rule(std::move(inner));

        auto aggregate = AggregateTable {};
        aggregate.source = source;
        aggregate.function = view.get_function();
        auto literal = Literal { Literal::Kind::AGGREGATE, npos, {} };
        for (size_t column = 0; column < variables.size(); ++column)
            if (outer.contains(variables[column]))
            {
                aggregate.group.push_back(column);
                literal.terms.push_back(Term::variable(outer_slots.at(variables[column])));
            }
        if (view.get_function() != AggregateFunction::COUNT)
            aggregate.value = inner_slots.at(view.get_value());
        aggregate.table = add_table(aggregate.group.size() + 1);
        if (view.get_function() == AggregateFunction::COUNT || view.get_function() == AggregateFunction::SUM)
            m_program.tables[aggregate.table]->fallback = 0;
        const auto [result, inserted] = outer_slots.try_emplace(view.get_result(), ygg::to_uint_t(outer_slots.size()));
        literal.terms.push_back(Term::variable(result->second));
        literal.table = aggregate.table;
        m_dependencies.push_back(Dependency { aggregate.table, source, true });
        m_aggregates.push_back(std::move(aggregate));
        return literal;
    }
};

/// Knuth's algorithm needs the weight of a weighted rule's head to be non-decreasing in, and at least, the weights of
/// its body atoms of the stratum (Felzenszwalb & McAllester 2007, §3): those weights may only flow, through + and max,
/// into the head's weight, never into a filter, a join or another operation.
void require_superior(const StratumInfo& stratum, const CompiledRule& rule)
{
    const auto fail = []
    { throw std::invalid_argument("Datalog: a weighted rule's weight must be the sum or maximum of its weighted atoms' weights and other terms."); };
    auto weights = urd::Store<urd::set<ygg::uint_t>, urd::sorted>();
    for (const auto& literal : rule.literals)
        if (literal.kind == Literal::Kind::POSITIVE && stratum.contains(literal.table))
        {
            const auto weight = literal.terms.back();
            if (!weight.is_variable() || !weights.insert(weight.value).second)
                fail();
        }
    const auto stratum_weights = weights;
    // Results of + and max over weights are weights.
    for (bool grew = true; grew;)
    {
        grew = false;
        for (const auto& literal : rule.literals)
            if (literal.kind == Literal::Kind::ARITHMETIC
                && (literal.operation == Arithmetic::Operation::ADD || literal.operation == Arithmetic::Operation::MAX)
                && std::ranges::any_of(literal.terms | std::views::take(2), [&](Term t) { return t.is_variable() && weights.contains(t.value); }))
                grew = weights.insert(literal.terms[2].value).second || grew;
    }
    // A weight occurs only as a stratum atom's weight column, as an operand or result of + and max, or as the head's weight.
    for (const auto& literal : rule.literals)
        for (size_t i = 0; i < literal.terms.size(); ++i)
        {
            const auto t = literal.terms[i];
            if (!t.is_variable() || !weights.contains(t.value))
                continue;
            const bool weight_column = literal.kind == Literal::Kind::POSITIVE && stratum.contains(literal.table) && i + 1 == literal.terms.size();
            const bool combined = literal.kind == Literal::Kind::ARITHMETIC
                                  && (literal.operation == Arithmetic::Operation::ADD || literal.operation == Arithmetic::Operation::MAX);
            if (!weight_column && !combined)
                fail();
        }
    for (size_t i = 0; i + 1 < rule.head_terms.size(); ++i)
        if (rule.head_terms[i].is_variable() && weights.contains(rule.head_terms[i].value))
            fail();
    // Every stratum weight flows into the head's weight.
    const auto head = rule.head_terms.back();
    if (stratum_weights.empty())
        return;
    if (!head.is_variable() || !weights.contains(head.value))
        fail();
    auto reaching = urd::Store<urd::set<ygg::uint_t>, urd::sorted> { head.value };
    for (bool grew = true; grew;)
    {
        grew = false;
        for (const auto& literal : rule.literals)
            if (literal.kind == Literal::Kind::ARITHMETIC && reaching.contains(literal.terms[2].value))
                for (const auto t : literal.terms | std::views::take(2))
                    if (t.is_variable())
                        grew = reaching.insert(t.value).second || grew;
    }
    if (!std::ranges::includes(reaching, stratum_weights))
        fail();
}

/// The tables the outputs depend on.
urd::Store<urd::sequence<bool>> needed_tables(const CompiledProgram& program, std::span<const Dependency> dependencies)
{
    auto needed = urd::Store<urd::sequence<bool>>(program.tables.size());
    auto pending = program.outputs;
    for (const auto table : pending)
        needed[table] = true;
    while (!pending.empty())
    {
        const auto table = pending.back();
        pending.pop_back();
        for (const auto& dependency : dependencies)
            if (dependency.head == table && !needed[dependency.body])
            {
                needed[dependency.body] = true;
                pending.push_back(dependency.body);
            }
    }
    return needed;
}
}  // namespace

CompiledProgram translate(ProgramView view, std::span<const RelationView> outputs, std::span<const size_t> static_slots)
{
    auto translator = Translator {};
    for (const auto rule : view.get_rules())
        translator.add_rule(rule);
    auto& program = translator.program();
    for (const auto relation : outputs)
        program.outputs.push_back(translator.table_of(relation));

    // Only the relations the outputs depend on are evaluated, and only the inputs they read are read.
    const auto needed = needed_tables(program, translator.dependencies());
    erase_if(program.rules, [&](const CompiledRule& rule) { return !needed[rule.head]; });
    for (auto& table : program.slot_tables)
        if (table != npos && !needed[table])
            table = npos;
    for (const auto slot : static_slots)
        if (slot < program.slot_tables.size() && program.slot_tables[slot] != npos)
            program.tables[program.slot_tables[slot]]->is_static = true;

    const auto components = stratify(program.tables.size(), translator.dependencies());

    auto stratum_of = urd::Store<urd::sequence<size_t>>(program.tables.size());
    program.recursive_tables.assign(program.tables.size(), false);
    auto& aggregates = translator.aggregates();
    auto aggregate_of = urd::Store<urd::map<size_t, size_t>, urd::sorted>();
    for (size_t a = 0; a < aggregates.size(); ++a)
        aggregate_of.emplace(aggregates[a].table, a);
    // The strata the outputs need, their rules and the tables those read; then one object per stratum.
    auto needed_components = urd::Store<urd::sequence<const Component*>>();
    auto infos = urd::Store<urd::sequence<StratumInfo>>();
    for (const auto& component : components)
    {
        // A component's relations depend on each other, so either all of them are needed or none.
        if (!needed[component.relations.front()])
            continue;
        // Input relations have no rules and no stratum.
        if (std::ranges::all_of(component.relations, [&](size_t t) { return program.tables[t]->slot.has_value(); }))
            continue;
        for (const auto table : component.relations)
            stratum_of[table] = infos.size();
        needed_components.push_back(&component);
        infos.push_back(StratumInfo { component.relations, {}, {} });
    }
    for (size_t r = 0; r < program.rules.size(); ++r)
    {
        auto& info = infos[stratum_of[program.rules[r].head]];
        info.rules.push_back(r);
        for (const auto& literal : program.rules[r].literals)
            if (literal.table != npos && std::ranges::find(info.reads, literal.table) == info.reads.end())
                info.reads.push_back(literal.table);
    }
    // The rules that read a weighted stratum s, if all of them are goals: a min head that copies the weight of its one
    // atom of s, with its other literals in earlier strata (the targets); then s may stop once each goal holds, since
    // rows settle in order of weight. A goal's key columns are key columns of the atom, which the rules of s keep (a row
    // of one key derives only rows of that key), so that s stops each key on its own. None if s has an output or
    // another reader.
    const auto goal_rules = [&](size_t s) -> urd::Store<urd::sequence<size_t>>
    {
        const auto& info = infos[s];
        if (std::ranges::any_of(info.tables, [&](size_t t) { return std::ranges::find(program.outputs, t) != program.outputs.end(); }))
            return {};
        auto goals = urd::Store<urd::sequence<size_t>>();
        for (size_t r = 0; r < program.rules.size(); ++r)
        {
            const auto& rule = program.rules[r];
            if (stratum_of[rule.head] == s)
                continue;
            auto reading = urd::Store<urd::sequence<size_t>>();
            for (size_t j = 0; j < rule.literals.size(); ++j)
                if (rule.literals[j].table != npos && info.contains(rule.literals[j].table))
                    reading.push_back(j);
            if (reading.empty())
                continue;
            const auto& head = *program.tables[rule.head];
            const auto& read = rule.literals[reading.front()];
            if (reading.size() != 1 || read.kind != Literal::Kind::POSITIVE || !head.weighted || rule.head_terms.back() != read.terms.back())
                return {};
            for (size_t i = 0; i + 1 < rule.head_terms.size(); ++i)
            {
                const auto key = std::find(read.terms.begin(), read.terms.end() - 1, rule.head_terms[i]);
                if (!rule.head_terms[i].is_variable() || key == read.terms.end() - 1 || info.tables.size() != 1)
                    return {};
                const auto column = static_cast<size_t>(key - read.terms.begin());
                for (const auto other : info.rules)
                    for (const auto& literal : program.rules[other].literals)
                        if (literal.kind == Literal::Kind::POSITIVE && info.contains(literal.table)
                            && (!literal.terms[column].is_variable() || program.rules[other].head_terms[column] != literal.terms[column]))
                            return {};
            }
            for (const auto& literal : rule.literals)
                if (literal.table != npos && !info.contains(literal.table) && !program.tables[literal.table]->slot && stratum_of[literal.table] >= s)
                    return {};
            goals.push_back(r);
        }
        return goals;
    };
    // Whether the weights of a weighted table can be observed: it is an output or an aggregate's body, or a rule uses
    // the weight of a row it reads otherwise than by copying it into the weight of a weighted head whose weights cannot
    // be observed. A table on the walk counts as observed, which keeps cycles conservative.
    auto observed = urd::Store<urd::sequence<std::optional<bool>>>(program.tables.size());
    auto weights_observed = std::function<bool(size_t)>();
    weights_observed = [&](size_t table) -> bool
    {
        if (observed[table])
            return *observed[table];
        observed[table] = true;
        auto result = std::ranges::find(program.outputs, table) != program.outputs.end()
                      || std::ranges::any_of(aggregates, [&](const AggregateTable& aggregate) { return aggregate.source == table; });
        for (size_t r = 0; r < program.rules.size() && !result; ++r)
        {
            const auto& rule = program.rules[r];
            for (const auto& literal : rule.literals)
            {
                if (literal.table != table || (literal.kind != Literal::Kind::POSITIVE && literal.kind != Literal::Kind::NEGATED))
                    continue;
                const auto weight = literal.terms.back();
                if (!weight.is_variable())
                {
                    result = true;
                    break;
                }
                auto uses = std::ranges::count(rule.head_terms, weight);
                for (const auto& other : rule.literals)
                    uses += std::ranges::count(other.terms, weight);
                const auto copied = program.tables[rule.head]->weighted && rule.head_terms.back() == weight;
                if (uses > 2 || (uses == 2 && !(copied && !weights_observed(rule.head))))
                {
                    result = true;
                    break;
                }
            }
        }
        observed[table] = result;
        return result;
    };
    for (size_t s = 0; s < infos.size(); ++s)
    {
        auto& info = infos[s];
        const auto weighted = std::ranges::count_if(info.tables, [&](size_t t) { return program.tables[t]->weighted; });
        if (weighted > 0)
        {
            if (static_cast<size_t>(weighted) != info.tables.size())
                throw std::invalid_argument("Datalog: a weighted relation is recursive with an unweighted one.");
            for (const auto r : info.rules)
                require_superior(info, program.rules[r]);
            auto goals = goal_rules(s);
            // Where the stratum stops depends on the targets: a change of them alone updates it.
            for (const auto g : goals)
                for (const auto& literal : program.rules[g].literals)
                    if (literal.table != npos && !info.contains(literal.table) && std::ranges::find(info.reads, literal.table) == info.reads.end())
                        info.reads.push_back(literal.table);
            auto search = GoalSearch::make(program, info, goals);
            if (search)
                search->any_path = std::ranges::none_of(goals, [&](size_t g) { return weights_observed(program.rules[g].head); });
            auto unit = search ? UnitSearch::make(program, info, *search) : nullptr;
            // Only a unit search tries the heuristic's proofs of unreachability.
            auto heuristic = search ? GoalHeuristic::make(program, *search, unit != nullptr) : nullptr;
            auto frontier = search ? GoalFrontier::make(program, *search) : nullptr;
            program.strata.push_back(std::make_unique<WeightedStratum>(std::move(info),
                                                                       program,
                                                                       std::move(goals),
                                                                       std::move(heuristic),
                                                                       std::move(frontier),
                                                                       std::move(unit),
                                                                       search ? search->target_tables : urd::Store<urd::sequence<size_t>> {}));
        }
        else if (info.tables.size() == 1 && aggregate_of.contains(info.tables.front()))
        {
            auto& aggregate = aggregates[aggregate_of.at(info.tables.front())];
            info.reads.push_back(aggregate.source);
            program.strata.push_back(std::make_unique<AggregateStratum>(std::move(info), std::move(aggregate)));
        }
        else if (needed_components[s]->recursive)
        {
            for (const auto table : info.tables)
                program.recursive_tables[table] = true;
            program.strata.push_back(std::make_unique<RecursiveStratum>(std::move(info), program));
        }
        else
            program.strata.push_back(std::make_unique<PlainStratum>(std::move(info), program));
    }
    plan_rules(program);
    return std::move(program);
}
}  // namespace tyr::datalog::database::datalog::detail
