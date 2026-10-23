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
#include <cassert>
#include <compare>
#include <map>
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
    using StaticLiteral = formalism::datalog::LiteralView<LiftedTag, formalism::StaticTag>;
    using FluentLiteral = formalism::datalog::LiteralView<LiftedTag, formalism::FluentTag>;
    using FluentAtom = formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>;

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
            body.static_literals.push_back(literal);
        for (const auto literal : condition.get_literals<formalism::FluentTag>())
            body.fluent_literals.push_back(literal);

        auto head_parameters = Parameters {};
        for (const auto head : heads)
            formalism::datalog::collect_parameters(head, head_parameters);

        auto projections = std::vector<std::pair<RuleSplitting::Predicate, std::vector<ygg::uint_t>>> {};
        m_used_projections.clear();
        while (auto projection = choose_projection(body, head_parameters, variables.size()))
        {
            restrict_kept_variables(projection->rest, projection->kept, projection->projected);
            auto [atom, positions] = get_or_create_projection(variables, projection->projected, projection->kept);
            projection->rest.fluent_literals.push_back(atom);
            projections.emplace_back(atom.get_atom().get_predicate().get_index(), std::move(positions));
            body = std::move(projection->rest);
        }
        if (projections.empty())
            return std::nullopt;

        const auto [main_rule, positions] = emit_rule(variables, body, heads, cost);
        m_splitting.origins.emplace(main_rule, RuleSplitting::Origin { variables.size(), positions });
        for (auto& [predicate, projection_positions] : projections)
        {
            [[maybe_unused]] const auto inserted =
                m_splitting.projection_origins.emplace(std::pair(main_rule, predicate), RuleSplitting::Origin { variables.size(), std::move(projection_positions) })
                    .second;
            assert(inserted && "A rule uses each projection predicate once.");
        }
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

    struct ProjectionKey
    {
        std::vector<ygg::uint_t> static_literals;
        std::vector<ygg::uint_t> fluent_literals;
        std::vector<ygg::uint_t> kept;

        auto operator<=>(const ProjectionKey&) const = default;
    };

    struct Projection
    {
        Body projected;
        Body rest;
        std::vector<formalism::ParameterIndex> kept;
    };

    /// At most this many variables are kept by a projection: wider projections can derive far more atoms than they save.
    static constexpr size_t max_kept_variables = 2;

    template<formalism::FactKind F>
    static Parameters parameters_of(formalism::datalog::LiteralView<LiftedTag, F> literal)
    { return formalism::datalog::collect_parameters(literal); }

    static Parameters parameters_of(const Body& body)
    {
        auto result = Parameters {};
        for (const auto literal : body.static_literals)
            formalism::datalog::collect_parameters(literal, result);
        for (const auto literal : body.fluent_literals)
            formalism::datalog::collect_parameters(literal, result);
        return result;
    }

    /// The projection of a single variable, with the static literals whose variables it covers, that keeps the fewest variables.
    /// A projection must leave a precondition with a variable outside it, which it then joins with: otherwise the projection
    /// rule enumerates the same bindings as the rule did.
    std::optional<Projection> choose_projection(const Body& body, const Parameters& head_parameters, size_t num_variables) const
    {
        auto best = std::optional<Projection> {};
        for (ygg::uint_t position = 0; position < num_variables; ++position)
        {
            const auto variable = formalism::ParameterIndex(position);
            if (head_parameters.contains(variable))
                continue;

            auto candidate = Projection {};
            const auto partition = [&](const auto& literals, auto& projected, auto& rest)
            {
                for (const auto literal : literals)
                    (parameters_of(literal).contains(variable) ? projected : rest).push_back(literal);
            };
            partition(body.static_literals, candidate.projected.static_literals, candidate.rest.static_literals);
            partition(body.fluent_literals, candidate.projected.fluent_literals, candidate.rest.fluent_literals);
            // A projection of static literals alone would derive the same atoms in every evaluation.
            if (candidate.projected.fluent_literals.empty() || (candidate.rest.static_literals.empty() && candidate.rest.fluent_literals.empty()))
                continue;

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

            if (!best || candidate.kept.size() < best->kept.size())
                best = std::move(candidate);
        }
        return best;
    }

    /// Restricts each kept variable of a projection by the static literals of the rest of the rule: the objects that such a
    /// literal admits at the variable form a static unary predicate, which the projection rule gets in addition. The rest keeps
    /// its literals, so this only adds constraints that every binding of the rule satisfies.
    void restrict_kept_variables(const Body& rest, const std::vector<formalism::ParameterIndex>& kept, Body& projected)
    {
        for (const auto variable : kept)
            for (const auto literal : rest.static_literals)
                if (const auto parameters = parameters_of(literal); literal.get_polarity() && parameters.contains(variable) && parameters.size() > 1)
                    projected.static_literals.push_back(get_or_create_domain_literal(literal, variable));
    }

    /// A literal over the variable alone that admits the objects that the static literal admits at the variable.
    StaticLiteral get_or_create_domain_literal(StaticLiteral literal, formalism::ParameterIndex variable)
    {
        const auto atom = literal.get_atom();
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
        for (const auto binding : m_static_facts.get_sets()[ygg::uint_t(atom.get_predicate().get_index())].get_bindings())
        {
            auto element = std::vector<ygg::Data<formalism::Term>> {};
            for (const auto object : binding.get_objects())
                element.push_back(ygg::Data<formalism::Term>(object.get_index()));
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

    /// The positions among the source rule's variables of the variables of a body, in order.
    static std::vector<ygg::uint_t> positions_of(const Body& body)
    {
        auto positions = std::vector<ygg::uint_t> {};
        for (const auto parameter : parameters_of(body))
            positions.push_back(ygg::uint_t(parameter));
        std::ranges::sort(positions);
        return positions;
    }

    /// The projection atom over the kept variables for the projected body, and the positions of the projection rule's
    /// variables among the source rule's variables. A projection with the same literals and kept variables (after
    /// renumbering the variables by their order) reuses the earlier predicate and rule.
    std::pair<FluentLiteral, std::vector<ygg::uint_t>> get_or_create_projection(const ygg::IndexList<formalism::Variable>& variables,
                                                                               const Body& projected,
                                                                               const std::vector<formalism::ParameterIndex>& kept)
    {
        auto positions = positions_of(projected);
        auto local = ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex> {};
        for (size_t i = 0; i < positions.size(); ++i)
            local.emplace(formalism::ParameterIndex(positions[i]), formalism::ParameterIndex(ygg::uint_t(i)));

        auto key = ProjectionKey {};
        for (const auto literal : projected.static_literals)
            key.static_literals.push_back(ygg::uint_t(renumber(literal, local)));
        for (const auto literal : projected.fluent_literals)
            key.fluent_literals.push_back(ygg::uint_t(renumber(literal, local)));
        std::ranges::sort(key.static_literals);
        std::ranges::sort(key.fluent_literals);
        for (const auto variable : kept)
            key.kept.push_back(ygg::uint_t(local.at(variable)));

        if (const auto it = m_projections.find(key); it != m_projections.end() && !m_used_projections.contains(it->second))
        {
            m_used_projections.insert(it->second);
            return { create_literal<formalism::FluentTag>(it->second, kept), std::move(positions) };
        }

        const auto atom = create_projection_atom(kept);
        const auto [projection_rule, rule_positions] = emit_rule(variables, projected, { atom.get_atom() }, ygg::float_t(0));
        assert(rule_positions == positions);
        m_program.predicate_rules.push_back(projection_rule);
        m_splitting.projection_rules.emplace(atom.get_atom().get_predicate().get_index(), projection_rule);
        m_projections.emplace(std::move(key), atom.get_atom().get_predicate().get_index());
        m_used_projections.insert(atom.get_atom().get_predicate().get_index());
        return { atom, std::move(positions) };
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
    formalism::datalog::LiteralView<LiftedTag, F> create_literal(ygg::Index<formalism::Predicate<F>> predicate,
                                                                 const std::vector<formalism::ParameterIndex>& parameters)
    {
        auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, F>>(m_context.builder);
        atom->predicate = predicate;
        for (const auto parameter : parameters)
            atom->terms.push_back(ygg::Data<formalism::Term>(parameter));
        auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, F>>(m_context.builder);
        literal->atom = formalism::datalog::insert(m_context.destination, *atom).first.get_index();
        literal->polarity = true;
        return formalism::datalog::insert(m_context.destination, *literal).first;
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
    ygg::Index<formalism::datalog::Literal<LiftedTag, F>> renumber(formalism::datalog::LiteralView<LiftedTag, F> literal,
                                                                   const ygg::UnorderedMap<formalism::ParameterIndex, formalism::ParameterIndex>& local)
    {
        auto data = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, F>>(m_context.builder);
        data->atom = renumber(literal.get_atom(), local);
        data->polarity = literal.get_polarity();
        return formalism::datalog::insert(m_context.destination, *data).first.get_index();
    }

    /// The rule over the variables that its body uses, in action rule order, and the action rule position of each of them.
    std::pair<ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>, std::vector<ygg::uint_t>>
    emit_rule(const ygg::IndexList<formalism::Variable>& variables,
              const Body& body,
              const std::vector<FluentAtom>& heads,
              ygg::float_t cost)
    {
        auto positions = positions_of(body);
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
            rule->heads.push_back(renumber(head, local));
        rule->cost = cost;
        return { formalism::datalog::insert(m_context.destination, *rule).first.get_index(), std::move(positions) };
    }

    ygg::Data<formalism::datalog::Program<LiftedTag>>& m_program;
    RuleSplitting& m_splitting;
    formalism::datalog::CopyContext& m_context;
    const PredicateFactSets<formalism::StaticTag>& m_static_facts;
    size_t m_num_projections = 0;
    std::map<ProjectionKey, RuleSplitting::Predicate> m_projections;
    /// The projection predicates of the rule being split: a rule uses a shared projection at most once, since its origin is
    /// kept per main rule and predicate.
    ygg::UnorderedSet<RuleSplitting::Predicate> m_used_projections;
    ygg::UnorderedMap<std::pair<ygg::Index<formalism::datalog::Atom<LiftedTag, formalism::StaticTag>>, formalism::ParameterIndex>, StaticLiteral>
        m_domain_literals;
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
    {
        const auto copy = formalism::datalog::copy(rule, context).first.get_index();
        result->function_rules.push_back(copy);
        splitting.rules.function.emplace(rule.get_index(), copy);
    }

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
            heads.push_back(formalism::datalog::copy(head, context).first);
        if (const auto main_rule = splitter.split(formalism::datalog::copy(rule.get_body(), context).first, heads, rule.get_cost()))
        {
            result->predicate_rules.push_back(*main_rule);
            splitting.rules.predicate.emplace(rule.get_index(), *main_rule);
        }
        else
        {
            const auto copy = formalism::datalog::copy(rule, context).first.get_index();
            result->predicate_rules.push_back(copy);
            splitting.rules.predicate.emplace(rule.get_index(), copy);
        }
    }

    return formalism::datalog::insert(repository, *result).first;
}

}
