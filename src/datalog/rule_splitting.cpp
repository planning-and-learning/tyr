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

#include "tyr/datalog/rule_splitting.hpp"

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
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::datalog
{
namespace
{
/// Splits the rules of a program: the costs of a binding are sums or maxima over its preconditions, and the projected
/// preconditions share no variable with the rest of the rule except the projection's, so the minimal costs are unchanged.
/// Literals keep the source rule's parameter positions until a rule is emitted.
class Splitter
{
public:
    using StaticLiteral = ygg::Index<formalism::datalog::Literal<LiftedTag, formalism::StaticTag>>;
    using FluentLiteral = ygg::Index<formalism::datalog::Literal<LiftedTag, formalism::FluentTag>>;
    using FluentAtom = ygg::Index<formalism::datalog::Atom<LiftedTag, formalism::FluentTag>>;

    Splitter(ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
             RuleSplitting& splitting,
             formalism::datalog::CopyContext& context,
             const PredicateFactSets<formalism::StaticTag>& static_facts) :
        m_program(program),
        m_splitting(splitting),
        m_context(context),
        m_static_facts(static_facts)
    {
    }

    /// The main rule of the rule with the given parts in the destination repository, or nullopt if nothing can be projected.
    /// The projection rules are added to the program.
    std::optional<RuleSplitting::Rule> split(formalism::datalog::ConjunctiveConditionView<LiftedTag> condition,
                                             const std::vector<FluentAtom>& heads,
                                             ygg::float_t cost)
    {
        if (!condition.get_numeric_constraints().empty())
            return std::nullopt;

        // Copies, since emitting rules inserts into the repository that holds the condition.
        auto variables = ygg::IndexList<formalism::Variable> {};
        for (const auto variable : condition.get_variables())
            variables.push_back(variable.get_index());
        auto body = Body {};
        for (const auto literal : condition.get_literals<formalism::StaticTag>())
            body.static_literals.push_back(literal.get_index());
        for (const auto literal : condition.get_literals<formalism::FluentTag>())
            body.fluent_literals.push_back(literal.get_index());

        auto head_parameters = Parameters {};
        for (const auto head : heads)
            formalism::datalog::collect_parameters(ygg::make_view(head, m_context.destination), head_parameters);

        auto projections = std::vector<std::pair<Body, FluentLiteral>> {};
        auto num_static_projections = size_t(0);
        auto excluded = Parameters {};
        while (auto projection = choose_projection(body, head_parameters, variables.size(), excluded))
        {
            if (projection->is_static)
            {
                const auto literal = create_static_projection(projection->projected, projection->kept, variables.size());
                if (!literal)
                {
                    excluded.insert(projection->variable);
                    continue;
                }
                projection->rest.static_literals.push_back(*literal);
                body = std::move(projection->rest);
                ++num_static_projections;
                continue;
            }
            restrict_kept_variables(projection->rest, projection->kept, projection->projected);
            const auto atom = create_projection_atom(projection->kept);
            projection->rest.fluent_literals.push_back(atom);
            projections.emplace_back(std::move(projection->projected), atom);
            body = std::move(projection->rest);
        }
        if (projections.empty() && num_static_projections == 0)
            return std::nullopt;

        for (const auto& [projected, atom] : projections)
        {
            const auto [projection_rule, positions] =
                emit_rule(variables, projected, { ygg::make_view(atom, m_context.destination).get_atom().get_index() }, ygg::float_t(0));
            m_program.predicate_rules.push_back(projection_rule);
            m_splitting.origins.emplace(projection_rule, RuleSplitting::Origin { variables.size(), positions, static_projections_of(projected) });
            m_splitting.projection_rules.emplace(ygg::make_view(atom, m_context.destination).get_atom().get_predicate().get_index(), projection_rule);
        }
        const auto [main_rule, positions] = emit_rule(variables, body, heads, cost);
        m_splitting.origins.emplace(main_rule, RuleSplitting::Origin { variables.size(), positions, static_projections_of(body) });
        return main_rule;
    }

private:
    using Parameters = ygg::UnorderedSet<formalism::ParameterIndex>;
    using Substitution = formalism::unification::SubstitutionFunction<ygg::Data<formalism::Term>>;

    struct Body
    {
        std::vector<StaticLiteral> static_literals;
        std::vector<FluentLiteral> fluent_literals;
    };

    struct Projection
    {
        Body projected;
        Body rest;
        std::vector<formalism::ParameterIndex> kept;
        formalism::ParameterIndex variable;
        bool is_static = false;  ///< The projected literals are all static: their atoms are computed once (see create_static_projection).
    };

    /// At most this many variables are kept by a projection: wider projections can derive far more atoms than they save.
    static constexpr size_t max_kept_variables = 2;
    /// A static projection whose evaluation visits more assignments is not made.
    static constexpr size_t max_static_projection_steps = 10'000'000;

    template<formalism::FactKind F>
    Parameters parameters_of(ygg::Index<formalism::datalog::Literal<LiftedTag, F>> literal) const
    { return formalism::datalog::collect_parameters(ygg::make_view(literal, m_context.destination)); }

    Parameters parameters_of(const Body& body) const
    {
        auto result = Parameters {};
        for (const auto literal : body.static_literals)
            formalism::datalog::collect_parameters(ygg::make_view(literal, m_context.destination), result);
        for (const auto literal : body.fluent_literals)
            formalism::datalog::collect_parameters(ygg::make_view(literal, m_context.destination), result);
        return result;
    }

    /// The projection of a single variable, with the static literals whose variables it covers: a static one if possible, then
    /// the one that keeps the fewest variables. A projection must leave a precondition with a variable outside it, which it then
    /// joins with: otherwise the projection rule enumerates the same bindings as the rule did.
    std::optional<Projection>
    choose_projection(const Body& body, const Parameters& head_parameters, size_t num_variables, const Parameters& excluded) const
    {
        auto best = std::optional<Projection> {};
        for (ygg::uint_t position = 0; position < num_variables; ++position)
        {
            const auto variable = formalism::ParameterIndex(position);
            if (head_parameters.contains(variable) || excluded.contains(variable))
                continue;

            auto candidate = Projection {};
            candidate.variable = variable;
            const auto partition = [&](const auto& literals, auto& projected, auto& rest)
            {
                for (const auto literal : literals)
                    (parameters_of(literal).contains(variable) ? projected : rest).push_back(literal);
            };
            partition(body.static_literals, candidate.projected.static_literals, candidate.rest.static_literals);
            partition(body.fluent_literals, candidate.projected.fluent_literals, candidate.rest.fluent_literals);
            if (candidate.projected.static_literals.empty() && candidate.projected.fluent_literals.empty())
                continue;
            if (candidate.rest.static_literals.empty() && candidate.rest.fluent_literals.empty())
                continue;
            // A projection of static literals alone derives the same atoms in every evaluation, so they are computed once.
            candidate.is_static = candidate.projected.fluent_literals.empty();

            const auto covered = parameters_of(candidate.projected);
            std::erase_if(candidate.rest.static_literals,
                          [&](const auto literal)
                          {
                              const auto parameters = parameters_of(literal);
                              const auto inside = !parameters.empty() && std::ranges::all_of(parameters, [&](auto p) { return covered.contains(p); });
                              if (inside)
                                  candidate.projected.static_literals.push_back(literal);
                              return inside;
                          });

            if (candidate.rest.static_literals.empty() && candidate.rest.fluent_literals.empty())
                continue;
            const auto rest_parameters = parameters_of(candidate.rest);
            if (std::ranges::all_of(rest_parameters, [&](auto p) { return covered.contains(p); }))
                continue;

            for (const auto p : covered)
                if (head_parameters.contains(p) || rest_parameters.contains(p))
                    candidate.kept.push_back(p);
            std::ranges::sort(candidate.kept);
            if (candidate.kept.size() > max_kept_variables)
                continue;
            if (candidate.is_static && (candidate.kept.empty() || !binds_every_variable(candidate.projected, covered)))
                continue;

            if (!best || std::pair(!candidate.is_static, candidate.kept.size()) < std::pair(!best->is_static, best->kept.size()))
                best = std::move(candidate);
        }
        return best;
    }

    /// Whether every variable is bound by a positive literal of the body, so that joining those literals enumerates it.
    bool binds_every_variable(const Body& body, const Parameters& variables) const
    {
        auto bound = Parameters {};
        for (const auto literal : body.static_literals)
            if (ygg::make_view(literal, m_context.destination).get_polarity())
                formalism::datalog::collect_parameters(ygg::make_view(literal, m_context.destination), bound);
        return std::ranges::all_of(variables, [&](auto p) { return bound.contains(p); });
    }

    using Tuple = ygg::IndexList<formalism::Object>;

    /// The tuples of a static predicate: its facts, or the atoms of a static projection created before.
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

    /// The static projections that the body uses, as indices into the splitting's static projections.
    std::vector<size_t> static_projections_of(const Body& body) const
    {
        auto result = std::vector<size_t> {};
        for (const auto literal : body.static_literals)
            if (const auto it = m_static_projection_ids.find(literal); it != m_static_projection_ids.end())
                result.push_back(it->second);
        return result;
    }

    /// A literal over the kept variables of a new static predicate whose atoms are the kept parts of the assignments that satisfy
    /// the static literals, with one supporting assignment of the other variables per atom; nullopt if the join is too large.
    /// The literals are joined by backtracking, always extending by the positive literal with the most bound variables.
    std::optional<StaticLiteral>
    create_static_projection(const Body& projected, const std::vector<formalism::ParameterIndex>& kept, size_t num_variables)
    {
        struct Pattern
        {
            ygg::Index<formalism::Predicate<formalism::StaticTag>> predicate;
            std::vector<ygg::Data<formalism::Term>> terms;
            std::optional<size_t> static_projection;
        };
        auto positive = std::vector<Pattern> {};
        auto negative = std::vector<Pattern> {};
        for (const auto index : projected.static_literals)
        {
            const auto literal = ygg::make_view(index, m_context.destination);
            auto pattern = Pattern { literal.get_atom().get_predicate().get_index(), {}, std::nullopt };
            for (const auto term : literal.get_atom().get_terms())
                pattern.terms.push_back(term.get_data());
            if (const auto it = m_static_projection_ids.find(index); it != m_static_projection_ids.end())
                pattern.static_projection = it->second;
            (literal.get_polarity() ? positive : negative).push_back(std::move(pattern));
        }

        // Filled before the join, which then only reads them: inserting would move the containers' elements.
        for (const auto& pattern : positive)
            tuples_of(pattern.predicate);
        for (const auto& pattern : negative)
            tuple_set_of(pattern.predicate);

        auto eliminated = std::vector<formalism::ParameterIndex> {};
        for (const auto p : parameters_of(projected))
            if (std::ranges::find(kept, p) == kept.end())
                eliminated.push_back(p);
        std::ranges::sort(eliminated);

        auto assignment = std::vector<std::optional<ygg::Index<formalism::Object>>>(num_variables);
        const auto ground = [&](const Pattern& pattern)
        {
            auto tuple = Tuple {};
            for (const auto& term : pattern.terms)
                tuple.push_back(formalism::unification::is_parameter(term) ? *assignment[ygg::uint_t(formalism::unification::get_parameter(term))] :
                                                                              formalism::unification::get_object(term));
            return tuple;
        };

        auto projection = RuleSplitting::StaticProjection {};
        for (const auto p : kept)
            projection.kept_positions.push_back(ygg::uint_t(p));
        auto used = std::vector<bool>(positive.size(), false);
        auto steps = size_t(0);
        auto aborted = false;

        const auto record = [&]
        {
            for (const auto& pattern : negative)
                if (tuple_set_of(pattern.predicate).contains(ground(pattern)))
                    return;
            auto key = Tuple {};
            for (const auto p : kept)
                key.push_back(*assignment[ygg::uint_t(p)]);
            if (projection.supports.contains(key))
                return;
            auto support = std::vector<std::pair<ygg::uint_t, ygg::Index<formalism::Object>>> {};
            for (const auto p : eliminated)
                support.emplace_back(ygg::uint_t(p), *assignment[ygg::uint_t(p)]);
            // The supports of the static projections used here bind the variables that they eliminated before.
            for (const auto& pattern : positive)
                if (pattern.static_projection)
                    for (const auto& pair : m_splitting.static_projections[*pattern.static_projection].supports.at(ground(pattern)))
                        support.push_back(pair);
            projection.supports.emplace(std::move(key), std::move(support));
        };

        const auto extend = [&](const auto& self) -> void
        {
            if (aborted)
                return;
            auto next = std::optional<size_t> {};
            auto next_bound = size_t(0);
            for (size_t i = 0; i < positive.size(); ++i)
            {
                if (used[i])
                    continue;
                const auto bound = size_t(std::ranges::count_if(positive[i].terms,
                                                                [&](const auto& term) {
                                                                    return !formalism::unification::is_parameter(term)
                                                                           || assignment[ygg::uint_t(formalism::unification::get_parameter(term))];
                                                                }));
                if (!next || bound > next_bound)
                {
                    next = i;
                    next_bound = bound;
                }
            }
            if (!next)
            {
                record();
                return;
            }

            const auto& pattern = positive[*next];
            used[*next] = true;
            for (const auto& tuple : tuples_of(pattern.predicate))
            {
                if (++steps > max_static_projection_steps)
                {
                    aborted = true;
                    break;
                }
                auto newly_bound = std::vector<ygg::uint_t> {};
                auto matches = true;
                for (size_t k = 0; k < pattern.terms.size() && matches; ++k)
                {
                    const auto& term = pattern.terms[k];
                    if (!formalism::unification::is_parameter(term))
                        matches = formalism::unification::get_object(term) == tuple[k];
                    else if (auto& slot = assignment[ygg::uint_t(formalism::unification::get_parameter(term))])
                        matches = *slot == tuple[k];
                    else
                    {
                        slot = tuple[k];
                        newly_bound.push_back(ygg::uint_t(formalism::unification::get_parameter(term)));
                    }
                }
                if (matches)
                    self(self);
                for (const auto p : newly_bound)
                    assignment[p].reset();
                if (aborted)
                    break;
            }
            used[*next] = false;
        };
        extend(extend);
        if (aborted)
            return std::nullopt;

        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::StaticTag>>(m_context.builder);
        predicate->name = "__tyr_static_projection_" + std::to_string(m_splitting.static_projections.size());
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
            auto ground_atom = formalism::datalog::checkout<formalism::datalog::Atom<GroundTag, formalism::StaticTag>>(m_context.builder);
            ground_atom->binding = formalism::datalog::insert(m_context.destination, *binding).first.get_index();
            m_program.static_atoms.push_back(formalism::datalog::insert(m_context.destination, *ground_atom).first.get_index());
        }

        const auto literal = create_literal<formalism::StaticTag>(new_predicate, kept);
        m_static_projection_ids.emplace(literal, m_splitting.static_projections.size());
        m_splitting.static_projections.push_back(std::move(projection));
        return literal;
    }

    /// Restricts each kept variable of a projection by the static literals of the rest of the rule: the objects that such a
    /// literal admits at the variable form a static unary predicate, which the projection rule gets in addition. The rest keeps
    /// its literals, so this only adds constraints that every binding of the rule satisfies.
    void restrict_kept_variables(const Body& rest, const std::vector<formalism::ParameterIndex>& kept, Body& projected)
    {
        for (const auto variable : kept)
            for (const auto literal : rest.static_literals)
                if (const auto parameters = parameters_of(literal);
                    ygg::make_view(literal, m_context.destination).get_polarity() && parameters.contains(variable) && parameters.size() > 1)
                    projected.static_literals.push_back(get_or_create_domain_literal(literal, variable));
    }

    /// A literal over the variable alone that admits the objects that the static literal admits at the variable.
    StaticLiteral get_or_create_domain_literal(StaticLiteral literal, formalism::ParameterIndex variable)
    {
        const auto atom = ygg::make_view(literal, m_context.destination).get_atom();
        const auto key = std::pair(atom.get_index(), variable);
        if (const auto it = m_domain_literals.find(key); it != m_domain_literals.end())
            return it->second;

        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::StaticTag>>(m_context.builder);
        predicate->name = "__tyr_domain_" + std::to_string(m_domain_literals.size());
        predicate->arity = 1;
        const auto new_predicate = formalism::datalog::insert(m_context.destination, *predicate).first.get_index();
        m_program.static_predicates.push_back(new_predicate);

        auto pattern = std::vector<ygg::Data<formalism::Term>> {};
        for (const auto term : atom.get_terms())
            pattern.push_back(term.get_data());
        const auto atom_parameters = formalism::datalog::collect_parameters(atom);
        const auto sigma = Substitution(std::vector<formalism::ParameterIndex>(atom_parameters.begin(), atom_parameters.end()));

        auto objects = ygg::UnorderedSet<ygg::Index<formalism::Object>> {};
        for (const auto& tuple : tuples_of(atom.get_predicate().get_index()))
        {
            auto element = std::vector<ygg::Data<formalism::Term>> {};
            for (const auto object : tuple)
                element.push_back(ygg::Data<formalism::Term>(object));
            if (const auto match = formalism::unification::match(pattern, element, sigma))
            {
                const auto object = formalism::unification::get_object(*(*match)[variable]);
                if (objects.insert(object).second)
                {
                    auto domain_binding =
                        formalism::datalog::checkout<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>(m_context.builder);
                    domain_binding->relation = new_predicate;
                    domain_binding->objects.push_back(object);
                    auto ground_atom = formalism::datalog::checkout<formalism::datalog::Atom<GroundTag, formalism::StaticTag>>(m_context.builder);
                    ground_atom->binding = formalism::datalog::insert(m_context.destination, *domain_binding).first.get_index();
                    m_program.static_atoms.push_back(formalism::datalog::insert(m_context.destination, *ground_atom).first.get_index());
                }
            }
        }

        const auto result = create_literal<formalism::StaticTag>(new_predicate, { variable });
        m_domain_literals.emplace(key, result);
        return result;
    }

    FluentLiteral create_projection_atom(const std::vector<formalism::ParameterIndex>& kept)
    {
        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(m_context.builder);
        predicate->name = "__tyr_projection_" + std::to_string(m_num_projections++);
        predicate->arity = kept.size();
        const auto new_predicate = formalism::datalog::insert(m_context.destination, *predicate).first.get_index();
        m_program.fluent_predicates.push_back(new_predicate);
        return create_literal<formalism::FluentTag>(new_predicate, kept);
    }

    template<formalism::FactKind F>
    ygg::Index<formalism::datalog::Literal<LiftedTag, F>> create_literal(ygg::Index<formalism::Predicate<F>> predicate,
                                                                         const std::vector<formalism::ParameterIndex>& parameters)
    {
        auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, F>>(m_context.builder);
        atom->predicate = predicate;
        for (const auto parameter : parameters)
            atom->terms.push_back(ygg::Data<formalism::Term>(parameter));
        auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, F>>(m_context.builder);
        literal->atom = formalism::datalog::insert(m_context.destination, *atom).first.get_index();
        literal->polarity = true;
        return formalism::datalog::insert(m_context.destination, *literal).first.get_index();
    }

    /// The atom with each parameter renumbered to its position among the emitted rule's variables.
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
    ygg::Index<formalism::datalog::Literal<LiftedTag, F>> renumber(ygg::Index<formalism::datalog::Literal<LiftedTag, F>> literal,
                                                                   const ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex>& local)
    {
        const auto view = ygg::make_view(literal, m_context.destination);
        auto data = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, F>>(m_context.builder);
        data->atom = renumber(view.get_atom(), local);
        data->polarity = view.get_polarity();
        return formalism::datalog::insert(m_context.destination, *data).first.get_index();
    }

    /// The rule over the variables that its body uses, in action rule order, and the action rule position of each of them.
    std::pair<ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>, std::vector<ygg::uint_t>>
    emit_rule(const ygg::IndexList<formalism::Variable>& variables,
              const Body& body,
              const std::vector<FluentAtom>& heads,
              ygg::float_t cost)
    {
        auto positions = std::vector<ygg::uint_t> {};
        for (const auto parameter : parameters_of(body))
            positions.push_back(ygg::uint_t(parameter));
        std::ranges::sort(positions);
        auto local = ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex> {};
        for (size_t i = 0; i < positions.size(); ++i)
            local.emplace(formalism::ParameterIndex(positions[i]), formalism::ParameterIndex(ygg::uint_t(i)));

        auto condition = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(m_context.builder);
        for (const auto position : positions)
            condition->variables.push_back(variables[position]);
        for (const auto literal : body.static_literals)
            condition->static_literals.push_back(renumber(literal, local));
        for (const auto literal : body.fluent_literals)
            condition->fluent_literals.push_back(renumber(literal, local));
        const auto new_condition = formalism::datalog::insert(m_context.destination, *condition).first;

        auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(m_context.builder);
        ygg::extend(new_condition.get_variables(), rule->variables);
        rule->body = new_condition.get_index();
        for (const auto head : heads)
            rule->heads.push_back(renumber(ygg::make_view(head, m_context.destination), local));
        rule->cost = cost;
        return { formalism::datalog::insert(m_context.destination, *rule).first.get_index(), std::move(positions) };
    }

    ygg::Data<formalism::datalog::Program<LiftedTag>>& m_program;
    RuleSplitting& m_splitting;
    formalism::datalog::CopyContext& m_context;
    const PredicateFactSets<formalism::StaticTag>& m_static_facts;
    size_t m_num_projections = 0;
    ygg::UnorderedMap<std::pair<ygg::Index<formalism::datalog::Atom<LiftedTag, formalism::StaticTag>>, formalism::ParameterIndex>, StaticLiteral>
        m_domain_literals;
    ygg::UnorderedMap<ygg::Index<formalism::Predicate<formalism::StaticTag>>, std::vector<Tuple>> m_tuples;
    ygg::UnorderedMap<ygg::Index<formalism::Predicate<formalism::StaticTag>>, ygg::UnorderedSet<Tuple>> m_tuple_sets;
    ygg::UnorderedMap<StaticLiteral, size_t> m_static_projection_ids;
};

}

formalism::datalog::ProgramView<LiftedTag>
split_rules(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, RuleSplitting& splitting)
{
    auto builder = formalism::datalog::Builder {};
    auto context = formalism::datalog::CopyContext { builder, repository };
    auto result = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);
    formalism::datalog::copy_except_rules(program, *result, context);
    for (const auto rule : program.get_rules<formalism::FunctionTag>())
        result->function_rules.push_back(formalism::datalog::copy(rule, context).first.get_index());

    // The static predicates come first in the repository, as the fact sets require.
    auto static_facts = PredicateFactSets<formalism::StaticTag>(ygg::make_view(result->static_predicates, repository), repository);
    for (const auto atom : result->static_atoms)
        static_facts.insert(ygg::make_view(atom, repository));
    auto splitter = Splitter(*result, splitting, context, static_facts);
    // Only rules of the program are inserted: rule workspaces are indexed by rule.
    for (const auto rule : program.get_rules<formalism::PredicateTag>())
    {
        auto heads = std::vector<Splitter::FluentAtom> {};
        for (const auto head : rule.get_heads())
            heads.push_back(formalism::datalog::copy(head, context).first.get_index());
        if (const auto main_rule = splitter.split(formalism::datalog::copy(rule.get_body(), context).first, heads, rule.get_cost()))
        {
            result->predicate_rules.push_back(*main_rule);
            splitting.main_rules.emplace(rule.get_index(), *main_rule);
        }
        else
            result->predicate_rules.push_back(formalism::datalog::copy(rule, context).first.get_index());
    }

    return formalism::datalog::insert(repository, *result).first;
}

}
