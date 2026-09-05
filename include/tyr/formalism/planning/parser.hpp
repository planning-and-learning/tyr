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

#ifndef TYR_FORMALISM_PLANNING_PARSER_HPP_
#define TYR_FORMALISM_PLANNING_PARSER_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/planning/declarations.hpp"

#include <filesystem>
#include <loki/loki.hpp>
#include <optional>

namespace fs = std::filesystem;

namespace tyr::formalism::planning
{
class Parser
{
public:
    Parser(const fs::path& domain_filepath,
           const loki::ParserOptions& options = loki::ParserOptions(),
           const loki::TranslatorOptions& translator_options = loki::TranslatorOptions());
    Parser(const std::string& domain_description,
           std::optional<fs::path> domain_filepath,
           const loki::ParserOptions& options = loki::ParserOptions(),
           const loki::TranslatorOptions& translator_options = loki::TranslatorOptions());

    PlanningTask parse_task(const fs::path& task_filepath, const loki::ParserOptions& options = loki::ParserOptions());
    PlanningTask
    parse_task(const std::string& task_description, std::optional<fs::path> task_filepath, const loki::ParserOptions& options = loki::ParserOptions());

    PlanningDomain get_domain() const;

private:
    loki::Parser m_loki_parser;
    loki::TranslatorOptions m_loki_translator_options;
    loki::DomainTranslationResult m_loki_domain_translation_result;

    PlanningDomain m_domain;
};
}

#endif
