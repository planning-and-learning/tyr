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
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/detail/repository_instantiations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/term_operations.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <boost/mp11.hpp>
#include <optional>
#include <string>
#include <urd/config.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

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
    using StaticLiteral = urd::index<formalism::Literal<LiftedTag, formalism::StaticTag>>;
    using FluentLiteral = urd::index<formalism::Literal<LiftedTag, formalism::FluentTag>>;
    using DerivedLiteral = urd::index<formalism::Literal<LiftedTag, formalism::DerivedTag>>;
    /// The heads of kind F of a rule.
    template<typename F>
    using Heads = urd::Store<urd::sequence<urd::index<formalism::Atom<LiftedTag, F>>>>;
    using RuleHeads = MutableFamily<Heads>;

    Splitter(urd::builder<formalism::datalog::Program<LiftedTag>, formalism::datalog::Repository>& program,
             RuleSplitting& splitting,
             formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
             const TaggedFactSets<formalism::StaticTag>& static_facts) :
        m_program(&program),
        m_splitting(&splitting),
        m_context(&context),
        m_static_facts(&static_facts)
    {
    }

    /// The main rule of the rule with the given parts in the destination repository, or nullopt if nothing can be projected.
    /// The projection rules are added to the program.
    std::optional<RuleSplitting::Rule> split(formalism::datalog::ConjunctiveConditionView<LiftedTag> condition, const RuleHeads& heads, ygg::float_t cost)
    {
        if (!condition.get_numeric_constraints().empty())
            return std::nullopt;

        // Copies, since emitting rules inserts into the repository that holds the condition.
        auto variables = urd::flat_vector<urd::index<formalism::Variable>> {};
        for (const auto variable : condition.get_variables())
            variables.push_back(variable.index());
        auto parameters = formalism::datalog::checkout<formalism::Parameters>(m_context->builder());
        for (const auto variable : variables)
            urd::append(parameters->variables, m_context->destination().view(variable));
        m_parameters = m_context->destination().insert(*parameters, m_context->builder()).first.index();
        auto body = Body {};
        for (const auto literal : condition.get_literals<formalism::StaticTag>())
            body.static_literals.push_back(literal.index());
        for (const auto literal : condition.get_literals<formalism::FluentTag>())
            body.fluent_literals.push_back(literal.index());
        for (const auto literal : condition.get_literals<formalism::DerivedTag>())
            body.derived_literals.push_back(literal.index());

        auto head_parameters = Parameters {};
        boost::mp11::mp_for_each<MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto head : heads.template get<F>())
                    formalism::datalog::collect_parameters(urd::resolve(head, m_context->destination()), head_parameters);
            });

        auto projections = urd::Store<urd::sequence<std::pair<Body, DerivedLiteral>>> {};
        while (auto projection = choose_projection(body, head_parameters, variables.size()))
        {
            restrict_kept_variables(projection->rest, projection->kept, projection->projected);
            const auto atom = create_projection_atom(projection->kept);
            projection->rest.derived_literals.push_back(atom);
            projections.emplace_back(std::move(projection->projected), atom);
            body = std::move(projection->rest);
        }
        if (projections.empty())
            return std::nullopt;

        for (const auto& [projected, atom] : projections)
        {
            auto projection_heads = RuleHeads {};
            projection_heads.template get<formalism::DerivedTag>().push_back(urd::resolve(atom, m_context->destination()).get_atom().index());
            const auto [projection_rule, positions] = emit_rule(variables, projected, projection_heads, static_cast<ygg::float_t>(0));
            urd::append(m_program->rules, m_context->destination().view(projection_rule));
            m_splitting->origins.emplace(projection_rule, RuleSplitting::Origin { variables.size(), positions });
            m_splitting->projection_rules.emplace(urd::resolve(atom, m_context->destination()).get_atom().get_predicate().index(), projection_rule);
        }
        const auto [main_rule, positions] = emit_rule(variables, body, heads, cost);
        m_splitting->origins.emplace(main_rule, RuleSplitting::Origin { variables.size(), positions });
        return main_rule;
    }

private:
    using Parameters = urd::Store<urd::set<formalism::ParameterIndex>>;
    using Substitution = formalism::unification::SubstitutionFunction<formalism::Term>;

    struct Body
    {
        urd::Store<urd::sequence<StaticLiteral>> static_literals;
        urd::Store<urd::sequence<FluentLiteral>> fluent_literals;
        urd::Store<urd::sequence<DerivedLiteral>> derived_literals;

        /// Whether the body has no literal over facts that evaluation derives.
        [[nodiscard]] bool has_no_mutable_literals() const noexcept { return fluent_literals.empty() && derived_literals.empty(); }

        [[nodiscard]] bool empty() const noexcept { return static_literals.empty() && has_no_mutable_literals(); }
    };

    struct Projection
    {
        Body projected;
        Body rest;
        urd::Store<urd::sequence<formalism::ParameterIndex>> kept;
    };

    /// At most this many variables are kept by a projection: wider projections can derive far more atoms than they save.
    static constexpr size_t max_kept_variables = 2;

    template<formalism::FactKind F>
    [[nodiscard]] Parameters parameters_of(urd::index<formalism::Literal<LiftedTag, F>> literal) const
    {
        return formalism::datalog::collect_parameters(urd::resolve(literal, m_context->destination()));
    }

    [[nodiscard]] Parameters parameters_of(const Body& body) const
    {
        auto result = Parameters {};
        for (const auto literal : body.static_literals)
            formalism::datalog::collect_parameters(urd::resolve(literal, m_context->destination()), result);
        for (const auto literal : body.fluent_literals)
            formalism::datalog::collect_parameters(urd::resolve(literal, m_context->destination()), result);
        for (const auto literal : body.derived_literals)
            formalism::datalog::collect_parameters(urd::resolve(literal, m_context->destination()), result);
        return result;
    }

    /// The projection of a single variable, with the static literals whose variables it covers, that keeps the fewest variables.
    /// A projection must leave a precondition with a variable outside it, which it then joins with: otherwise the projection
    /// rule enumerates the same bindings as the rule did.
    [[nodiscard]] std::optional<Projection> choose_projection(const Body& body, const Parameters& head_parameters, size_t num_variables) const
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
            partition(body.derived_literals, candidate.projected.derived_literals, candidate.rest.derived_literals);
            // A projection of static literals alone would derive the same atoms in every evaluation.
            if (candidate.projected.has_no_mutable_literals() || candidate.rest.empty())
                continue;

            const auto covered = parameters_of(candidate.projected);
            erase_if(candidate.rest.static_literals,
                     [&](const auto literal)
                     {
                         const auto parameters = parameters_of(literal);
                         const auto inside = !parameters.empty() && std::ranges::all_of(parameters, [&](auto p) { return covered.contains(p); });
                         if (inside)
                             candidate.projected.static_literals.push_back(literal);
                         return inside;
                     });

            if (candidate.rest.empty())
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
    void restrict_kept_variables(const Body& rest, const urd::Store<urd::sequence<formalism::ParameterIndex>>& kept, Body& projected)
    {
        for (const auto variable : kept)
            for (const auto literal : rest.static_literals)
                if (const auto parameters = parameters_of(literal);
                    urd::resolve(literal, m_context->destination()).get_polarity() && parameters.contains(variable) && parameters.size() > 1)
                    projected.static_literals.push_back(get_or_create_domain_literal(literal, variable));
    }

    /// A literal over the variable alone that admits the objects that the static literal admits at the variable.
    StaticLiteral get_or_create_domain_literal(StaticLiteral literal, formalism::ParameterIndex variable)
    {
        const auto atom = urd::resolve(literal, m_context->destination()).get_atom();
        const auto key = std::pair(atom.index(), variable);
        if (const auto it = m_domain_literals.find(key); it != m_domain_literals.end())
            return it->second;

        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::StaticTag>>(m_context->builder());
        predicate->name = "__tyr_domain_" + std::to_string(m_domain_literals.size());
        predicate->arity = 1;
        const auto new_predicate = m_context->destination().insert(*predicate, m_context->builder()).first.index();
        urd::append(m_program->static_predicates, m_context->destination().view(new_predicate));

        auto pattern = urd::Store<urd::sequence<formalism::Term>> {};
        for (const auto term : atom.get_terms())
            urd::append(pattern, term);
        const auto atom_parameters = formalism::datalog::collect_parameters(atom);
        const auto sigma = Substitution(urd::Store<urd::sequence<formalism::ParameterIndex>>(atom_parameters.begin(), atom_parameters.end()));

        auto objects = urd::Store<urd::set<urd::index<formalism::Object>>> {};
        for (const auto binding : rows(*m_static_facts, atom.get_predicate().index().value))
        {
            auto element = urd::Store<urd::sequence<formalism::Term>> {};
            for (const auto object : binding.template get<urd::index<formalism::Object>>())
                element.emplace_back(object);
            if (const auto match = formalism::unification::match(pattern, element, sigma))
            {
                const auto& value = (*match)[variable];
                URD_ASSUME(value.has_value());  ///< The variable occurs in the matched atom.
                const auto object = formalism::unification::get_object(*value);
                if (objects.insert(object).second)
                {
                    auto domain_binding = formalism::datalog::checkout<ygg::Row<formalism::Predicate<formalism::StaticTag>>>(m_context->builder());
                    urd::store(domain_binding->relation, m_context->destination().view(new_predicate));
                    domain_binding->values.push_back(object.value);
                    auto ground_atom = formalism::datalog::checkout<formalism::Atom<GroundTag, formalism::StaticTag>>(m_context->builder());
                    urd::store(ground_atom->row, m_context->destination().insert(*domain_binding, m_context->builder()).first);
                    urd::append(m_program->static_atoms, m_context->destination().insert(*ground_atom, m_context->builder()).first);
                }
            }
        }

        const auto result = create_literal<formalism::StaticTag>(new_predicate, { variable });
        m_domain_literals.emplace(key, result);
        return result;
    }

    /// A literal over a new derived predicate, the projection's, of the kept variables.
    DerivedLiteral create_projection_atom(const urd::Store<urd::sequence<formalism::ParameterIndex>>& kept)
    {
        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::DerivedTag>>(m_context->builder());
        predicate->name = "__tyr_projection_" + std::to_string(m_num_projections++);
        predicate->arity = kept.size();
        const auto new_predicate = m_context->destination().insert(*predicate, m_context->builder()).first.index();
        urd::append(m_program->derived_predicates, m_context->destination().view(new_predicate));
        return create_literal<formalism::DerivedTag>(new_predicate, kept);
    }

    template<formalism::FactKind F>
    urd::index<formalism::Literal<LiftedTag, F>> create_literal(urd::index<formalism::Predicate<F>> predicate,
                                                                const urd::Store<urd::sequence<formalism::ParameterIndex>>& parameters)
    {
        auto atom = formalism::datalog::checkout<formalism::Atom<LiftedTag, F>>(m_context->builder());
        const auto& destination = m_context->destination();
        urd::store(atom->parameters, destination.view(m_parameters));
        urd::store(atom->predicate, destination.view(predicate));
        for (const auto parameter : parameters)
            urd::build_member(atom->terms.emplace_back(), formalism::Term(parameter), destination);
        auto literal = formalism::datalog::checkout<formalism::Literal<LiftedTag, F>>(m_context->builder());
        urd::store(literal->atom, m_context->destination().insert(*atom, m_context->builder()).first);
        literal->polarity = true;
        return m_context->destination().insert(*literal, m_context->builder()).first.index();
    }

    /// The atom with each parameter renumbered to its position among the emitted rule's variables, its `parameters`.
    template<formalism::FactKind F>
    urd::index<formalism::Atom<LiftedTag, F>> renumber(formalism::datalog::AtomView<LiftedTag, F> atom,
                                                       const urd::Store<urd::map<formalism::ParameterIndex, formalism::ParameterIndex>>& local,
                                                       urd::index<formalism::Parameters> parameters)
    {
        auto data = formalism::datalog::checkout<formalism::Atom<LiftedTag, F>>(m_context->builder());
        const auto& destination = m_context->destination();
        urd::store(data->parameters, destination.view(parameters));
        urd::store(data->predicate, atom.get_predicate());
        for (const auto term : atom.get_terms())
        {
            const auto value = urd::stored<formalism::Term>(term);
            urd::build_member(data->terms.emplace_back(),
                              formalism::unification::is_parameter(value) ? formalism::Term(local.at(formalism::unification::get_parameter(value))) : value,
                              destination);
        }
        return m_context->destination().insert(*data, m_context->builder()).first.index();
    }

    template<formalism::FactKind F>
    urd::index<formalism::Literal<LiftedTag, F>> renumber(urd::index<formalism::Literal<LiftedTag, F>> literal,
                                                          const urd::Store<urd::map<formalism::ParameterIndex, formalism::ParameterIndex>>& local,
                                                          urd::index<formalism::Parameters> parameters)
    {
        const auto view = urd::resolve(literal, m_context->destination());
        auto data = formalism::datalog::checkout<formalism::Literal<LiftedTag, F>>(m_context->builder());
        urd::store(data->atom, m_context->destination().view(renumber(view.get_atom(), local, parameters)));
        data->polarity = view.get_polarity();
        return m_context->destination().insert(*data, m_context->builder()).first.index();
    }

    /// The rule over the variables that its body uses, in action rule order, and the action rule position of each of them.
    std::pair<urd::index<formalism::datalog::Rule<LiftedTag>>, urd::Store<urd::sequence<ygg::uint_t>>>
    emit_rule(const urd::flat_vector<urd::index<formalism::Variable>>& variables, const Body& body, const RuleHeads& heads, ygg::float_t cost)
    {
        auto positions = urd::Store<urd::sequence<ygg::uint_t>> {};
        for (const auto parameter : parameters_of(body))
            positions.push_back(parameter.value);
        std::ranges::sort(positions);
        auto local = urd::Store<urd::map<formalism::ParameterIndex, formalism::ParameterIndex>> {};
        for (size_t i = 0; i < positions.size(); ++i)
            local.emplace(formalism::ParameterIndex(positions[i]), formalism::ParameterIndex(static_cast<ygg::uint_t>(i)));

        auto condition = formalism::datalog::checkout<formalism::ConjunctiveCondition<LiftedTag>>(m_context->builder());
        for (const auto position : positions)
            urd::append(condition->variables, m_context->destination().view(variables[position]));
        auto parameters = formalism::datalog::checkout<formalism::Parameters>(m_context->builder());
        urd::build_member(parameters->variables, condition->variables, m_context->destination());
        const auto rule_parameters = m_context->destination().insert(*parameters, m_context->builder()).first.index();
        for (const auto literal : body.static_literals)
            urd::append(condition->static_literals, m_context->destination().view(renumber(literal, local, rule_parameters)));
        for (const auto literal : body.fluent_literals)
            urd::append(condition->fluent_literals, m_context->destination().view(renumber(literal, local, rule_parameters)));
        for (const auto literal : body.derived_literals)
            urd::append(condition->derived_literals, m_context->destination().view(renumber(literal, local, rule_parameters)));
        const auto new_condition = m_context->destination().insert(*condition, m_context->builder()).first;

        auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag>>(m_context->builder());
        for (const auto variable : new_condition.get_variables())
            urd::append(rule->variables, variable);
        urd::store(rule->body, new_condition);
        for (const auto head : heads.template get<formalism::FluentTag>())
            urd::append(rule->fluent_heads, m_context->destination().view(renumber(urd::resolve(head, m_context->destination()), local, rule_parameters)));
        for (const auto head : heads.template get<formalism::DerivedTag>())
            urd::append(rule->derived_heads, m_context->destination().view(renumber(urd::resolve(head, m_context->destination()), local, rule_parameters)));
        rule->cost = cost;
        return { m_context->destination().insert(*rule, m_context->builder()).first.index(), std::move(positions) };
    }

    urd::builder<formalism::datalog::Program<LiftedTag>, formalism::datalog::Repository>* m_program;
    RuleSplitting* m_splitting;
    formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>* m_context;
    const TaggedFactSets<formalism::StaticTag>* m_static_facts;
    size_t m_num_projections = 0;
    /// The variables of the rule being split: the parameters of the atoms built for it before they are renumbered.
    urd::index<formalism::Parameters> m_parameters;
    urd::Store<urd::map<std::pair<urd::index<formalism::Atom<LiftedTag, formalism::StaticTag>>, formalism::ParameterIndex>, StaticLiteral>> m_domain_literals;
};

}

formalism::datalog::ProgramView<LiftedTag>
split_rules(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, RuleSplitting& splitting)
{
    auto builder = formalism::datalog::Builder {};
    auto context = formalism::WriteContext { builder, repository };
    auto result = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);
    formalism::datalog::list_except_rules(program, *result);
    // The static predicates come first in the repository, as the fact sets require.
    // The program's own static predicates and atoms: the result's, copied within this repository.
    auto static_facts = TaggedFactSets<formalism::StaticTag>(program.template get_predicates<formalism::StaticTag>(), repository);
    for (const auto atom : program.template get_atoms<formalism::StaticTag>())
        insert(static_facts, atom.get_row().index());
    auto splitter = Splitter(*result, splitting, context, static_facts);
    // Only rules of the program are inserted: rule workspaces are indexed by rule.
    for (const auto rule : program.get_rules())
    {
        auto heads = Splitter::RuleHeads {};
        boost::mp11::mp_for_each<MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto head : rule.template get_heads<F>())
                    heads.template get<F>().push_back(head.index());
            });
        if (const auto main_rule = splitter.split(rule.get_body(), heads, rule.get_cost()))
        {
            urd::append(result->rules, repository.view(*main_rule));
            splitting.rules.emplace(rule.index(), *main_rule);
        }
        else
        {
            // The rule as it is: the output's layer stands on the source's.
            urd::append(result->rules, rule);
            splitting.rules.emplace(rule.index(), rule.index());
        }
    }

    return repository.insert(*result, builder).first;
}

}
