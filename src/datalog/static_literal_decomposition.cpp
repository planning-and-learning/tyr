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

#include "tyr/datalog/static_literal_decomposition.hpp"

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/term_operations.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
namespace
{
class Decomposer
{
public:
    using StaticLiteral = formalism::datalog::LiteralView<LiftedTag, formalism::StaticTag>;
    using StaticLiteralIndex = ygg::Index<formalism::datalog::Literal<LiftedTag, formalism::StaticTag>>;

    Decomposer(ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
               formalism::datalog::CopyContext& context,
               const PredicateFactSets<formalism::StaticTag>& static_facts) :
        m_program(&program),
        m_context(&context),
        m_static_facts(&static_facts)
    {
    }

    /// The literals over the pairs of the literal's variables that replace it, or nullopt if it is not their join.
    const std::optional<std::vector<StaticLiteralIndex>>& decompose(StaticLiteral literal)
    {
        auto [it, inserted] = m_decompositions.try_emplace(literal.get_index());
        if (inserted)
            it->second = compute(literal);
        return it->second;
    }

private:
    using Substitution = formalism::unification::SubstitutionFunction<ygg::Data<formalism::Term>>;
    using Tuple = std::vector<ygg::uint_t>;

    /// A join check visiting more assignments gives up and keeps the literal.
    static constexpr size_t max_join_steps = 10'000'000;

    /// A pair key holds the first object in its upper and the second in its lower 32 bits.
    static constexpr unsigned pair_key_shift = 32;
    static constexpr std::uint64_t pair_key_lower_mask = (std::uint64_t { 1 } << pair_key_shift) - 1;

    static std::uint64_t pair_key(ygg::uint_t first, ygg::uint_t second) noexcept { return (static_cast<std::uint64_t>(first) << pair_key_shift) | second; }

    std::optional<std::vector<StaticLiteralIndex>> compute(StaticLiteral literal)
    {
        if (!literal.get_polarity())
            return std::nullopt;
        const auto parameter_set = formalism::datalog::collect_parameters(literal);
        auto parameters = std::vector<formalism::ParameterIndex>(parameter_set.begin(), parameter_set.end());
        std::ranges::sort(parameters);
        const auto arity = parameters.size();
        // Over at most two variables, a literal is exact in the consistency graph unless its atom has constants or repeated
        // variables; those become a literal over its variables.
        const auto is_plain = literal.get_atom().get_terms().size() == arity;
        if (arity == 0 || (arity < 3 && is_plain))
            return std::nullopt;

        // The literal's relation over its variables.
        auto pattern = std::vector<ygg::Data<formalism::Term>> {};
        for (const auto term : literal.get_atom().get_terms())
            pattern.push_back(term.get_data());
        auto relation = ygg::UnorderedSet<Tuple> {};
        auto element = std::vector<ygg::Data<formalism::Term>> {};
        for (const auto binding : m_static_facts->get_sets()[static_cast<ygg::uint_t>(literal.get_atom().get_predicate().get_index())].get_bindings())
        {
            element.clear();
            for (const auto object : binding.get_objects())
                element.emplace_back(object.get_index());
            if (const auto sigma = formalism::unification::match(pattern, element, Substitution(parameters)))
            {
                auto tuple = Tuple {};
                for (const auto p : parameters)
                {
                    const auto& value = (*sigma)[p];
                    YGG_ASSUME(value.has_value());  ///< A match binds every parameter of the pattern.
                    tuple.push_back(static_cast<ygg::uint_t>(formalism::unification::get_object(*value)));
                }
                relation.insert(std::move(tuple));
            }
        }
        if (relation.empty())
            return std::nullopt;
        if (arity < 3)
            return std::vector<StaticLiteralIndex> { create_projection_literal(relation, parameters) };

        // The pairwise projections, and for the first variable its partners in every other.
        auto pairs = std::vector<ygg::UnorderedSet<std::uint64_t>>(arity * arity);
        auto first_values = ygg::UnorderedSet<ygg::uint_t> {};
        auto partners = std::vector<ygg::UnorderedMap<ygg::uint_t, std::vector<ygg::uint_t>>>(arity);
        for (const auto& tuple : relation)
        {
            first_values.insert(tuple[0]);
            for (size_t i = 0; i < arity; ++i)
                for (size_t j = i + 1; j < arity; ++j)
                    if (pairs[i * arity + j].insert(pair_key(tuple[i], tuple[j])).second && i == 0)
                        partners[j][tuple[0]].push_back(tuple[j]);
        }

        // The join of the projections contains the relation; it is exact iff it has no more tuples.
        auto assignment = Tuple(arity);
        auto count = static_cast<size_t>(0);
        auto steps = static_cast<size_t>(0);
        const auto exceeded = [&] { return count > relation.size() || steps > max_join_steps; };
        const auto extend = [&](const auto& self, size_t k) -> void
        {
            if (k == arity)
            {
                ++count;
                return;
            }
            const auto consistent = [&](ygg::uint_t value)
            {
                for (size_t j = 1; j < k; ++j)
                    if (!pairs[j * arity + k].contains(pair_key(assignment[j], value)))
                        return false;
                return true;
            };
            const auto visit = [&](ygg::uint_t value)
            {
                ++steps;
                if (exceeded() || !consistent(value))
                    return;
                assignment[k] = value;
                self(self, k + 1);
            };
            if (k == 0)
                for (const auto value : first_values)
                    visit(value);
            else if (const auto it = partners[k].find(assignment[0]); it != partners[k].end())
                for (const auto value : it->second)
                    visit(value);
        };
        extend(extend, 0);
        if (exceeded())
            return std::nullopt;

        auto result = std::vector<StaticLiteralIndex> {};
        for (size_t i = 0; i < arity; ++i)
            for (size_t j = i + 1; j < arity; ++j)
                result.push_back(create_pair_literal(pairs[i * arity + j], parameters[i], parameters[j]));
        return result;
    }

    /// A literal over the parameters (in order) whose atoms are the tuples.
    StaticLiteralIndex create_projection_literal(const ygg::UnorderedSet<Tuple>& tuples, const std::vector<formalism::ParameterIndex>& parameters)
    {
        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::StaticTag>>(m_context->builder());
        predicate->name = "__tyr_projection_" + std::to_string(m_num_predicates++);
        predicate->arity = parameters.size();
        const auto new_predicate = formalism::datalog::insert(m_context->destination(), *predicate).first.get_index();
        m_program->static_predicates.push_back(new_predicate);

        auto sorted = std::vector<Tuple>(tuples.begin(), tuples.end());
        std::ranges::sort(sorted);
        for (const auto& tuple : sorted)
        {
            auto binding = formalism::datalog::checkout<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>(m_context->builder());
            binding->relation = new_predicate;
            for (const auto object : tuple)
                binding->objects.push_back(ygg::Index<formalism::Object>(object));
            auto atom = formalism::datalog::checkout<formalism::datalog::Atom<GroundTag, formalism::StaticTag>>(m_context->builder());
            atom->binding = formalism::datalog::insert(m_context->destination(), *binding).first.get_index();
            m_program->static_atoms.push_back(formalism::datalog::insert(m_context->destination(), *atom).first.get_index());
        }

        auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, formalism::StaticTag>>(m_context->builder());
        atom->predicate = new_predicate;
        for (const auto parameter : parameters)
            atom->terms.push_back(ygg::Data<formalism::Term>(parameter));
        auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, formalism::StaticTag>>(m_context->builder());
        literal->atom = formalism::datalog::insert(m_context->destination(), *atom).first.get_index();
        literal->polarity = true;
        return formalism::datalog::insert(m_context->destination(), *literal).first.get_index();
    }

    StaticLiteralIndex create_pair_literal(const ygg::UnorderedSet<std::uint64_t>& pairs, formalism::ParameterIndex first, formalism::ParameterIndex second)
    {
        auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::StaticTag>>(m_context->builder());
        predicate->name = "__tyr_pair_projection_" + std::to_string(m_num_predicates++);
        predicate->arity = 2;
        const auto new_predicate = formalism::datalog::insert(m_context->destination(), *predicate).first.get_index();
        m_program->static_predicates.push_back(new_predicate);

        auto sorted = std::vector<std::uint64_t>(pairs.begin(), pairs.end());
        std::ranges::sort(sorted);
        for (const auto key : sorted)
        {
            auto binding = formalism::datalog::checkout<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>(m_context->builder());
            binding->relation = new_predicate;
            binding->objects.push_back(ygg::Index<formalism::Object>(static_cast<ygg::uint_t>(key >> pair_key_shift)));
            binding->objects.push_back(ygg::Index<formalism::Object>(static_cast<ygg::uint_t>(key & pair_key_lower_mask)));
            auto atom = formalism::datalog::checkout<formalism::datalog::Atom<GroundTag, formalism::StaticTag>>(m_context->builder());
            atom->binding = formalism::datalog::insert(m_context->destination(), *binding).first.get_index();
            m_program->static_atoms.push_back(formalism::datalog::insert(m_context->destination(), *atom).first.get_index());
        }

        auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, formalism::StaticTag>>(m_context->builder());
        atom->predicate = new_predicate;
        atom->terms.push_back(ygg::Data<formalism::Term>(first));
        atom->terms.push_back(ygg::Data<formalism::Term>(second));
        auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, formalism::StaticTag>>(m_context->builder());
        literal->atom = formalism::datalog::insert(m_context->destination(), *atom).first.get_index();
        literal->polarity = true;
        return formalism::datalog::insert(m_context->destination(), *literal).first.get_index();
    }

    ygg::Data<formalism::datalog::Program<LiftedTag>>* m_program;
    formalism::datalog::CopyContext* m_context;
    const PredicateFactSets<formalism::StaticTag>* m_static_facts;
    size_t m_num_predicates = 0;
    ygg::UnorderedMap<StaticLiteralIndex, std::optional<std::vector<StaticLiteralIndex>>> m_decompositions;
};

}

formalism::datalog::ProgramView<LiftedTag>
decompose_static_literals(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, StaticLiteralDecomposition& result)
{
    auto builder = formalism::datalog::Builder {};
    auto context = formalism::datalog::CopyContext { builder, repository };
    auto output = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);
    formalism::datalog::copy_except_rules(program, *output, context);
    for (const auto rule : program.get_rules<formalism::FunctionTag>())
    {
        const auto copy = formalism::datalog::copy(rule, context).first.get_index();
        output->function_rules.push_back(copy);
        result.rules.function.emplace(rule.get_index(), copy);
    }

    // The static predicates come first in the repository, as the fact sets require.
    auto static_facts = PredicateFactSets<formalism::StaticTag>(ygg::make_view(output->static_predicates, repository), repository);
    for (const auto atom : output->static_atoms)
        static_facts.insert(ygg::make_view(atom, repository));
    auto decomposer = Decomposer(*output, context, static_facts);
    for (const auto rule : program.get_rules<formalism::PredicateTag>())
    {
        // Only rules of the program are inserted: rule workspaces are indexed by rule.
        const auto body = formalism::datalog::copy(rule.get_body(), context).first;
        auto condition = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(builder);
        auto decomposed = false;
        for (const auto literal : body.get_literals<formalism::StaticTag>())
        {
            if (const auto& literals = decomposer.decompose(literal))
            {
                condition->static_literals.insert(condition->static_literals.end(), literals->begin(), literals->end());
                decomposed = true;
            }
            else
                condition->static_literals.push_back(literal.get_index());
        }
        if (!decomposed)
        {
            const auto copy = formalism::datalog::copy(rule, context).first.get_index();
            output->predicate_rules.push_back(copy);
            result.rules.predicate.emplace(rule.get_index(), copy);
            continue;
        }

        for (const auto variable : body.get_variables())
            condition->variables.push_back(variable.get_index());
        for (const auto literal : body.get_literals<formalism::FluentTag>())
            condition->fluent_literals.push_back(literal.get_index());
        for (const auto numeric_constraint : body.get_numeric_constraints())
            condition->numeric_constraints.push_back(numeric_constraint.get_data());
        const auto new_body = formalism::datalog::insert(repository, *condition).first;

        auto new_rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(builder);
        ygg::extend(new_body.get_variables(), new_rule->variables);
        new_rule->body = new_body.get_index();
        for (const auto head : rule.get_heads())
            new_rule->heads.push_back(formalism::datalog::copy(head, context).first.get_index());
        new_rule->cost = rule.get_cost();
        const auto rewritten = formalism::datalog::insert(repository, *new_rule).first.get_index();
        output->predicate_rules.push_back(rewritten);
        result.rules.predicate.emplace(rule.get_index(), rewritten);
    }

    return formalism::datalog::insert(repository, *output).first;
}

}
