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

#include "tyr/datalog/static_variable_elimination.hpp"

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/term_operations.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tyr::datalog
{
namespace
{
class Eliminator
{
public:
    Eliminator(ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
               formalism::datalog::CopyContext& context,
               const PredicateFactSets<formalism::StaticTag>& static_facts) :
        m_program(program),
        m_context(context),
        m_static_facts(static_facts)
    {
    }

    using FluentAtom = formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>;

    /// The rewritten rule with the given parts in the destination repository and its origin, or nullopt if no variable can be
    /// eliminated.
    std::optional<std::pair<StaticVariableElimination::Rule, StaticVariableElimination::Origin>>
    rewrite(formalism::datalog::ConjunctiveConditionView<LiftedTag> condition, const std::vector<FluentAtom>& heads, ygg::float_t cost)
    {
        if (!condition.get_numeric_constraints().empty())
            return std::nullopt;

        auto static_literals = std::vector<StaticLiteral> {};
        for (const auto literal : condition.get_literals<formalism::StaticTag>())
            static_literals.push_back(literal);
        auto needed = Parameters {};  ///< The variables of the heads and of the fluent literals: never eliminated.
        for (const auto head : heads)
            formalism::datalog::collect_parameters(head, needed);
        for (const auto literal : condition.get_literals<formalism::FluentTag>())
            formalism::datalog::collect_parameters(literal, needed);

        auto projections = std::vector<std::pair<StaticLiteral, StaticVariableElimination::Projection>> {};
        auto excluded = Parameters {};
        while (const auto variable = choose_variable(static_literals, needed, condition.get_variables().size(), excluded))
        {
            auto projected = std::vector<StaticLiteral> {};
            auto rest = std::vector<StaticLiteral> {};
            for (const auto literal : static_literals)
                (formalism::datalog::collect_parameters(literal).contains(*variable) ? projected : rest).push_back(literal);
            // The static literals over projected variables only belong to the projection, too.
            const auto covered = parameters_of(projected);
            std::erase_if(rest,
                          [&](const auto literal)
                          {
                              const auto parameters = formalism::datalog::collect_parameters(literal);
                              const auto inside = !parameters.empty() && std::ranges::all_of(parameters, [&](auto p) { return covered.contains(p); });
                              if (inside)
                                  projected.push_back(literal);
                              return inside;
                          });
            auto kept = std::vector<formalism::ParameterIndex> {};
            const auto rest_parameters = parameters_of(rest);
            for (const auto p : covered)
                if (needed.contains(p) || rest_parameters.contains(p))
                    kept.push_back(p);
            std::ranges::sort(kept);

            auto projection = (kept.empty() || kept.size() > max_kept_variables) ? std::nullopt : project(projected, kept, projections);
            if (!projection)
            {
                excluded.insert(*variable);
                continue;
            }
            rest.push_back(projection->first);
            static_literals = std::move(rest);
            projections.push_back(std::move(*projection));
        }
        if (projections.empty())
            return std::nullopt;

        auto origin = StaticVariableElimination::Origin { condition.get_variables().size(), {}, {} };
        for (auto& [literal, projection] : projections)
            if (std::ranges::find(static_literals, literal) != static_literals.end())
                origin.projections.push_back(std::move(projection));
        return std::pair(emit_rule(condition, static_literals, heads, cost, origin.source_positions), std::move(origin));
    }

private:
    using StaticLiteral = formalism::datalog::LiteralView<LiftedTag, formalism::StaticTag>;
    using Parameters = ygg::UnorderedSet<formalism::ParameterIndex>;
    using Substitution = formalism::unification::SubstitutionFunction<ygg::Data<formalism::Term>>;
    using Tuple = ygg::IndexList<formalism::Object>;

    /// At most this many variables are kept, so that the consistency graph encodes the new static predicate exactly.
    static constexpr size_t max_kept_variables = 2;
    /// A projection whose join visits more assignments is not made.
    static constexpr size_t max_join_steps = 10'000'000;

    static Parameters parameters_of(const std::vector<StaticLiteral>& literals)
    {
        auto result = Parameters {};
        for (const auto literal : literals)
            formalism::datalog::collect_parameters(literal, result);
        return result;
    }

    /// The first variable that only static literals use, each of them positive or bound by a positive one.
    static std::optional<formalism::ParameterIndex>
    choose_variable(const std::vector<StaticLiteral>& static_literals, const Parameters& needed, size_t num_variables, const Parameters& excluded)
    {
        auto bound = Parameters {};
        for (const auto literal : static_literals)
            if (literal.get_polarity())
                formalism::datalog::collect_parameters(literal, bound);
        for (ygg::uint_t position = 0; position < num_variables; ++position)
        {
            const auto variable = formalism::ParameterIndex(position);
            if (needed.contains(variable) || excluded.contains(variable) || !bound.contains(variable))
                continue;
            // Every variable sharing a literal with it must be bound by a positive literal, or the join cannot enumerate it.
            auto ok = true;
            for (const auto literal : static_literals)
                if (const auto parameters = formalism::datalog::collect_parameters(literal); parameters.contains(variable))
                    ok = ok && std::ranges::all_of(parameters, [&](auto p) { return bound.contains(p); });
            if (ok)
                return variable;
        }
        return std::nullopt;
    }

    /// The tuples of a static predicate: its facts, or the atoms of a projection created before.
    const std::vector<Tuple>& tuples_of(ygg::Index<formalism::Predicate<formalism::StaticTag>> predicate)
    {
        auto [it, inserted] = m_tuples.try_emplace(predicate);
        if (inserted)
            for (const auto binding : m_static_facts.get_sets()[ygg::uint_t(predicate)].get_bindings())
            {
                auto tuple = Tuple {};
                for (const auto object : binding.get_objects())
                    tuple.push_back(object.get_index());
                it->second.push_back(std::move(tuple));
            }
        return it->second;
    }

    const ygg::UnorderedSet<Tuple>& tuple_set_of(ygg::Index<formalism::Predicate<formalism::StaticTag>> predicate)
    {
        auto [it, inserted] = m_tuple_sets.try_emplace(predicate);
        if (inserted)
            for (const auto& tuple : tuples_of(predicate))
                it->second.insert(tuple);
        return it->second;
    }

    /// A literal over the kept variables of a new static predicate whose atoms are the kept parts of the assignments that satisfy
    /// the literals, with one supporting assignment of the other variables per atom; nullopt if the join is too large. The
    /// literals are joined by backtracking, always extending by the positive literal with the most bound terms. Earlier
    /// projections of the rule that the literals use contribute the variables they eliminated to the supports.
    std::optional<std::pair<StaticLiteral, StaticVariableElimination::Projection>>
    project(const std::vector<StaticLiteral>& literals,
            const std::vector<formalism::ParameterIndex>& kept,
            const std::vector<std::pair<StaticLiteral, StaticVariableElimination::Projection>>& earlier)
    {
        struct Pattern
        {
            ygg::Index<formalism::Predicate<formalism::StaticTag>> predicate;
            std::vector<ygg::Data<formalism::Term>> terms;
            const StaticVariableElimination::Projection* projection;
        };
        auto positive = std::vector<Pattern> {};
        auto negative = std::vector<Pattern> {};
        for (const auto literal : literals)
        {
            auto pattern = Pattern { literal.get_atom().get_predicate().get_index(), {}, nullptr };
            for (const auto term : literal.get_atom().get_terms())
                pattern.terms.push_back(term.get_data());
            if (const auto it = std::ranges::find(earlier, literal, &std::pair<StaticLiteral, StaticVariableElimination::Projection>::first);
                it != earlier.end())
                pattern.projection = &it->second;
            (literal.get_polarity() ? positive : negative).push_back(std::move(pattern));
        }
        // Filled before the join, which then only reads them: inserting would move the containers' elements.
        for (const auto& pattern : positive)
            tuples_of(pattern.predicate);
        for (const auto& pattern : negative)
            tuple_set_of(pattern.predicate);

        const auto covered = parameters_of(literals);
        auto eliminated = std::vector<formalism::ParameterIndex> {};
        for (const auto p : covered)
            if (std::ranges::find(kept, p) == kept.end())
                eliminated.push_back(p);
        std::ranges::sort(eliminated);

        const auto ground = [](const Substitution& sigma, const Pattern& pattern)
        {
            auto tuple = Tuple {};
            for (const auto& term : pattern.terms)
                tuple.push_back(formalism::unification::get_object(
                    formalism::unification::is_parameter(term) ? *sigma[formalism::unification::get_parameter(term)] : term));
            return tuple;
        };

        auto projection = StaticVariableElimination::Projection {};
        for (const auto p : kept)
            projection.kept_positions.push_back(ygg::uint_t(p));
        const auto record = [&](const Substitution& sigma)
        {
            for (const auto& pattern : negative)
                if (tuple_set_of(pattern.predicate).contains(ground(sigma, pattern)))
                    return;
            auto key = Tuple {};
            for (const auto p : kept)
                key.push_back(formalism::unification::get_object(*sigma[p]));
            if (projection.supports.contains(key))
                return;
            auto support = std::vector<std::pair<ygg::uint_t, ygg::Index<formalism::Object>>> {};
            for (const auto p : eliminated)
                support.emplace_back(ygg::uint_t(p), formalism::unification::get_object(*sigma[p]));
            for (const auto& pattern : positive)
                if (pattern.projection)
                    for (const auto& pair : pattern.projection->supports.at(ground(sigma, pattern)))
                        support.push_back(pair);
            projection.supports.emplace(std::move(key), std::move(support));
        };

        auto used = std::vector<bool>(positive.size(), false);
        auto steps = size_t(0);
        auto aborted = false;
        auto element = std::vector<ygg::Data<formalism::Term>> {};
        const auto extend = [&](const auto& self, const Substitution& sigma) -> void
        {
            auto next = std::optional<size_t> {};
            auto next_bound = size_t(0);
            for (size_t i = 0; i < positive.size(); ++i)
            {
                if (used[i])
                    continue;
                const auto bound = size_t(std::ranges::count_if(positive[i].terms,
                                                                [&](const auto& term) {
                                                                    return !formalism::unification::is_parameter(term)
                                                                           || sigma[formalism::unification::get_parameter(term)].has_value();
                                                                }));
                if (!next || bound > next_bound)
                {
                    next = i;
                    next_bound = bound;
                }
            }
            if (!next)
                return record(sigma);

            const auto& pattern = positive[*next];
            used[*next] = true;
            for (const auto& tuple : tuples_of(pattern.predicate))
            {
                if (aborted || ++steps > max_join_steps)
                {
                    aborted = true;
                    break;
                }
                element.clear();
                for (const auto object : tuple)
                    element.push_back(ygg::Data<formalism::Term>(object));
                if (auto extended = formalism::unification::match(pattern.terms, element, sigma))
                    self(self, *extended);
            }
            used[*next] = false;
        };
        extend(extend, Substitution(std::vector<formalism::ParameterIndex>(covered.begin(), covered.end())));
        if (aborted)
            return std::nullopt;

        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::StaticTag>>(m_context.builder);
        predicate->name = "__tyr_static_projection_" + std::to_string(m_num_projections++);
        predicate->arity = kept.size();
        const auto new_predicate = formalism::datalog::insert(m_context.destination, *predicate).first.get_index();
        m_program.static_predicates.push_back(new_predicate);

        auto& tuples = m_tuples[new_predicate];
        for (const auto& [tuple, _] : projection.supports)
            tuples.push_back(tuple);
        std::ranges::sort(tuples);
        for (const auto& tuple : tuples)
        {
            auto binding = formalism::datalog::checkout<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>(m_context.builder);
            binding->relation = new_predicate;
            binding->objects = tuple;
            auto atom = formalism::datalog::checkout<formalism::datalog::Atom<GroundTag, formalism::StaticTag>>(m_context.builder);
            atom->binding = formalism::datalog::insert(m_context.destination, *binding).first.get_index();
            m_program.static_atoms.push_back(formalism::datalog::insert(m_context.destination, *atom).first.get_index());
        }

        auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, formalism::StaticTag>>(m_context.builder);
        atom->predicate = new_predicate;
        for (const auto p : kept)
            atom->terms.push_back(ygg::Data<formalism::Term>(p));
        auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, formalism::StaticTag>>(m_context.builder);
        literal->atom = formalism::datalog::insert(m_context.destination, *atom).first.get_index();
        literal->polarity = true;
        return std::pair(formalism::datalog::insert(m_context.destination, *literal).first, std::move(projection));
    }

    /// The atom with each parameter renumbered to its position among the rewritten rule's variables.
    template<formalism::FactKind F>
    ygg::Index<formalism::datalog::Atom<LiftedTag, F>> renumber(formalism::datalog::AtomView<LiftedTag, F> atom,
                                                                const ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex>& local)
    {
        auto data = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, F>>(m_context.builder);
        data->predicate = atom.get_predicate().get_index();
        for (const auto term : atom.get_terms())
        {
            const auto value = term.get_data();
            data->terms.push_back(
                formalism::unification::is_parameter(value) ? ygg::Data<formalism::Term>(local.at(formalism::unification::get_parameter(value))) : value);
        }
        return formalism::datalog::insert(m_context.destination, *data).first.get_index();
    }

    template<formalism::FactKind F>
    ygg::Index<formalism::datalog::Literal<LiftedTag, F>> renumber(formalism::datalog::LiteralView<LiftedTag, F> literal,
                                                                   const ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex>& local)
    {
        auto data = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, F>>(m_context.builder);
        data->atom = renumber(literal.get_atom(), local);
        data->polarity = literal.get_polarity();
        return formalism::datalog::insert(m_context.destination, *data).first.get_index();
    }

    /// The rule over the variables that its body uses, in source rule order; writes the source position of each of them.
    StaticVariableElimination::Rule emit_rule(formalism::datalog::ConjunctiveConditionView<LiftedTag> condition,
                                              const std::vector<StaticLiteral>& static_literals,
                                              const std::vector<FluentAtom>& heads,
                                              ygg::float_t cost,
                                              std::vector<ygg::uint_t>& positions)
    {
        auto parameters = parameters_of(static_literals);
        for (const auto literal : condition.get_literals<formalism::FluentTag>())
            formalism::datalog::collect_parameters(literal, parameters);
        for (const auto p : parameters)
            positions.push_back(ygg::uint_t(p));
        std::ranges::sort(positions);
        auto local = ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex> {};
        for (size_t i = 0; i < positions.size(); ++i)
            local.emplace(formalism::ParameterIndex(positions[i]), formalism::ParameterIndex(ygg::uint_t(i)));

        auto new_condition = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(m_context.builder);
        for (const auto position : positions)
            new_condition->variables.push_back(condition.get_variables()[position].get_index());
        for (const auto literal : static_literals)
            new_condition->static_literals.push_back(renumber(literal, local));
        for (const auto literal : condition.get_literals<formalism::FluentTag>())
            new_condition->fluent_literals.push_back(renumber(literal, local));
        const auto body = formalism::datalog::insert(m_context.destination, *new_condition).first;

        auto new_rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(m_context.builder);
        ygg::extend(body.get_variables(), new_rule->variables);
        new_rule->body = body.get_index();
        for (const auto head : heads)
            new_rule->heads.push_back(renumber(head, local));
        new_rule->cost = cost;
        return formalism::datalog::insert(m_context.destination, *new_rule).first.get_index();
    }

    ygg::Data<formalism::datalog::Program<LiftedTag>>& m_program;
    formalism::datalog::CopyContext& m_context;
    const PredicateFactSets<formalism::StaticTag>& m_static_facts;
    size_t m_num_projections = 0;
    ygg::UnorderedMap<ygg::Index<formalism::Predicate<formalism::StaticTag>>, std::vector<Tuple>> m_tuples;
    ygg::UnorderedMap<ygg::Index<formalism::Predicate<formalism::StaticTag>>, ygg::UnorderedSet<Tuple>> m_tuple_sets;
};

}

formalism::datalog::ProgramView<LiftedTag>
eliminate_static_variables(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, StaticVariableElimination& result)
{
    auto builder = formalism::datalog::Builder {};
    auto context = formalism::datalog::CopyContext { builder, repository };
    auto output = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);
    formalism::datalog::copy_except_rules(program, *output, context);
    for (const auto rule : program.get_rules<formalism::FunctionTag>())
        output->function_rules.push_back(formalism::datalog::copy(rule, context).first.get_index());

    // The static predicates come first in the repository, as the fact sets require.
    auto static_facts = PredicateFactSets<formalism::StaticTag>(ygg::make_view(output->static_predicates, repository), repository);
    for (const auto atom : output->static_atoms)
        static_facts.insert(ygg::make_view(atom, repository));
    auto eliminator = Eliminator(*output, context, static_facts);
    for (const auto rule : program.get_rules<formalism::PredicateTag>())
    {
        // Only rules of the program are inserted: rule workspaces are indexed by rule.
        auto heads = std::vector<Eliminator::FluentAtom> {};
        for (const auto head : rule.get_heads())
            heads.push_back(formalism::datalog::copy(head, context).first);
        if (auto rewritten = eliminator.rewrite(formalism::datalog::copy(rule.get_body(), context).first, heads, rule.get_cost()))
        {
            output->predicate_rules.push_back(rewritten->first);
            result.rules.emplace(rule.get_index(), rewritten->first);
            result.origins.emplace(rewritten->first, std::move(rewritten->second));
        }
        else
        {
            const auto copy = formalism::datalog::copy(rule, context).first.get_index();
            output->predicate_rules.push_back(copy);
            result.rules.emplace(rule.get_index(), copy);
        }
    }

    return formalism::datalog::insert(repository, *output).first;
}

void restore_eliminated_variables(const StaticVariableElimination::Origin& origin,
                                  const ygg::IndexList<formalism::Object>& binding,
                                  ygg::IndexList<formalism::Object>& source_binding)
{
    source_binding.resize(origin.num_source_variables);
    for (size_t i = 0; i < binding.size(); ++i)
        source_binding[origin.source_positions[i]] = binding[i];
    for (const auto& projection : origin.projections)
    {
        auto key = ygg::IndexList<formalism::Object> {};
        for (const auto position : projection.kept_positions)
            key.push_back(source_binding[position]);
        for (const auto& [position, object] : projection.supports.at(key))
            source_binding[position] = object;
    }
}

}
