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

#include "tyr/formalism/planning/parser.hpp"

#include "loki_to_tyr.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <filesystem>
#include <loki/semantic/options.hpp>
#include <loki/semantic/parser.hpp>
#include <loki/semantic/translator.hpp>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <yggdrasil/io/project_path.hpp>

namespace tyr::formalism
{

Parser::Parser(const std::filesystem::path& domain_filepath,
               const loki::semantic::ParserOptions& options,
               const loki::semantic::TranslatorOptions& translator_options) :
    Parser(ygg::common::read_file(domain_filepath), domain_filepath, options, translator_options)
{
}

struct Parser::LokiState
{
    loki::semantic::Parser parser;
    loki::semantic::TranslatorOptions translator_options;
    loki::semantic::DomainTranslationResult domain_translation_result;

    LokiState(const std::string& domain_description,
              const loki::semantic::ParserOptions& options,
              const loki::semantic::TranslatorOptions& translator_options_) :
        parser(domain_description, options),
        translator_options(translator_options_),
        domain_translation_result(loki::semantic::translate(parser.get_domain(), translator_options))
    {
    }
};

Parser::Parser(const std::string& domain_description,
               std::optional<std::filesystem::path> domain_filepath,
               const loki::semantic::ParserOptions& options,
               const loki::semantic::TranslatorOptions& translator_options) :
    m_loki(std::make_unique<LokiState>(domain_description, options, translator_options)),
    m_domain(LokiToTyrTranslator().translate(m_loki->domain_translation_result.get_translated_domain(), std::move(domain_filepath)))
{
}

Parser::Parser(Parser&&) noexcept = default;
Parser& Parser::operator=(Parser&&) noexcept = default;
Parser::~Parser() = default;

PlanningTask<LiftedTag> Parser::parse_task(const std::filesystem::path& task_filepath, const loki::semantic::ParserOptions& options)
{ return parse_task(ygg::common::read_file(task_filepath), task_filepath, options); }

PlanningTask<LiftedTag>
Parser::parse_task(const std::string& task_description, std::optional<std::filesystem::path> task_filepath, const loki::semantic::ParserOptions&)
{
    const auto task_translation_result =
        loki::semantic::translate(m_loki->parser.parse_task(task_description), m_loki->domain_translation_result, m_loki->translator_options);
    const auto task = task_translation_result.get_translated_task();

    // A predicate only the task's timed initial literals change is fluent in this task: its domain translated again so.
    auto fluent = std::set<std::string>();
    for (const auto predicate : m_domain.get_domain().predicates<FluentTag>())
        fluent.insert(predicate.name().str());
    auto changed = std::set<std::string>();
    for (const auto timed_literal : task.timed_initial_literals())
    {
        const auto name = timed_literal.literal().atom().predicate().name().str();
        if (!fluent.contains(name))
            changed.insert(name);
    }
    if (changed.empty())
        return LokiToTyrTranslator().translate(task, m_domain, std::move(task_filepath));

    const auto translator = [&]
    {
        auto result = LokiToTyrTranslator();
        for (const auto& name : changed)
            result.make_fluent(name);
        return result;
    };
    const auto domain = translator().translate(m_loki->domain_translation_result.get_translated_domain(), m_domain.get_path());
    return translator().translate(task, domain, std::move(task_filepath));
}

PlanningDomain Parser::get_domain() const { return m_domain; }

}
