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

#include <loki/loki.hpp>

namespace tyr::formalism::planning
{

Parser::Parser(const std::filesystem::path& domain_filepath, const loki::ParserOptions& options, const loki::TranslatorOptions& translator_options) :
    Parser(loki::semantic::read_file(domain_filepath), domain_filepath, options, translator_options)
{
}

struct Parser::LokiState
{
    loki::Parser parser;
    loki::TranslatorOptions translator_options;
    loki::DomainTranslationResult domain_translation_result;

    LokiState(const std::string& domain_description, const loki::ParserOptions& options, const loki::TranslatorOptions& translator_options_) :
        parser(domain_description, options),
        translator_options(translator_options_),
        domain_translation_result(loki::translate(parser.get_domain(), translator_options))
    {
    }
};

Parser::Parser(const std::string& domain_description,
               std::optional<std::filesystem::path> domain_filepath,
               const loki::ParserOptions& options,
               const loki::TranslatorOptions& translator_options) :
    m_loki(std::make_unique<LokiState>(domain_description, options, translator_options)),
    m_domain(LokiToTyrTranslator().translate(m_loki->domain_translation_result.get_translated_domain(), std::move(domain_filepath)))
{
}

Parser::Parser(Parser&&) noexcept = default;
Parser& Parser::operator=(Parser&&) noexcept = default;
Parser::~Parser() = default;

PlanningTask<LiftedTag> Parser::parse_task(const std::filesystem::path& task_filepath, const loki::ParserOptions& options)
{ return parse_task(loki::semantic::read_file(task_filepath), task_filepath, options); }

PlanningTask<LiftedTag> Parser::parse_task(const std::string& task_description, std::optional<std::filesystem::path> task_filepath, const loki::ParserOptions&)
{
    return LokiToTyrTranslator().translate(
        loki::translate(m_loki->parser.parse_task(task_description), m_loki->domain_translation_result, m_loki->translator_options).get_translated_task(),
        m_domain,
        std::move(task_filepath));
}

PlanningDomain Parser::get_domain() const { return m_domain; }

}
