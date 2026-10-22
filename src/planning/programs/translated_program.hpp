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

#ifndef TYR_SRC_PLANNING_PROGRAMS_TRANSLATED_PROGRAM_HPP_
#define TYR_SRC_PLANNING_PROGRAMS_TRANSLATED_PROGRAM_HPP_

#include "common.hpp"
#include "tyr/datalog/rule_mapping.hpp"
#include "tyr/datalog/static_literal_decomposition.hpp"
#include "tyr/datalog/static_variable_elimination.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <memory>
#include <utility>

namespace tyr::planning
{

/// Maps the symbols and rules of a program transformation's source program to its output.
class SymbolRemapper
{
public:
    SymbolRemapper(formalism::datalog::Repository& destination, const datalog::RuleMapping& rules) :
        m_builder(),
        m_context { m_builder, destination },
        m_rules(rules)
    {
    }

    template<typename View>
    auto symbol(View view)
    {
        return formalism::datalog::copy(view, m_context).first;
    }

    template<formalism::RelationKind R>
    auto rule(ygg::Index<formalism::datalog::Rule<LiftedTag, R>> rule) const
    {
        return m_rules.get<R>().at(rule);
    }

    TranslationContext<LiftedTag> translation_context(const TranslationContext<LiftedTag>& source) { return remap_translation_context(source, m_context); }

private:
    formalism::datalog::Builder m_builder;
    formalism::datalog::CopyContext m_context;
    const datalog::RuleMapping& m_rules;
};

/// A Datalog program with the tables that refer to its symbols: Tables provide `Tables remap(SymbolRemapper&) const`.
template<typename Tables>
struct TranslatedProgram
{
    formalism::datalog::ProgramView<LiftedTag> program;
    std::shared_ptr<formalism::datalog::Repository> repository;
    Tables tables;
};

/// Applies a program transformation, `pass(program, destination, rules) -> program` that writes into a new repository of the
/// factory and maps every source rule in `rules`, and remaps the tables to its output.
template<typename Tables, typename Pass>
TranslatedProgram<Tables>
transform(const TranslatedProgram<Tables>& source, formalism::datalog::RepositoryFactory& factory, size_t num_objects, Pass&& pass)
{
    auto repository = factory.create_shared(num_objects);
    auto rules = datalog::RuleMapping {};
    const auto program = std::forward<Pass>(pass)(source.program, *repository, rules);
    auto remapper = SymbolRemapper(*repository, rules);
    return { program, std::move(repository), source.tables.remap(remapper) };
}

/// The pass of eliminate_static_variables; its restore information goes to `result`, keyed by its output rules.
inline auto eliminate_static_variables_pass(datalog::StaticVariableElimination& result)
{
    return [&result](formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& destination, datalog::RuleMapping& rules)
    {
        const auto output = datalog::eliminate_static_variables(program, destination, result);
        rules = result.rules;
        return output;
    };
}

inline auto decompose_static_literals_pass()
{
    return [](formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& destination, datalog::RuleMapping& rules)
    {
        auto result = datalog::StaticLiteralDecomposition {};
        const auto output = datalog::decompose_static_literals(program, destination, result);
        rules = std::move(result.rules);
        return output;
    };
}

}

#endif
