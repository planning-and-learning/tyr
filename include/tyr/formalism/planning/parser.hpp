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
#include <loki/semantic/options.hpp>
#include <memory>
#include <optional>
#include <string>

namespace tyr::formalism::planning
{
class Parser
{
public:
    Parser(const std::filesystem::path& domain_filepath,
           const loki::semantic::ParserOptions& options = loki::semantic::ParserOptions(),
           const loki::semantic::TranslatorOptions& translator_options = loki::semantic::TranslatorOptions());
    Parser(const std::string& domain_description,
           std::optional<std::filesystem::path> domain_filepath,
           const loki::semantic::ParserOptions& options = loki::semantic::ParserOptions(),
           const loki::semantic::TranslatorOptions& translator_options = loki::semantic::TranslatorOptions());

    PlanningTask<LiftedTag> parse_task(const std::filesystem::path& task_filepath, const loki::semantic::ParserOptions& options = loki::semantic::ParserOptions());
    PlanningTask<LiftedTag> parse_task(const std::string& task_description,
                                       std::optional<std::filesystem::path> task_filepath,
                                       const loki::semantic::ParserOptions& options = loki::semantic::ParserOptions());

    PlanningDomain get_domain() const;

    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&&) noexcept;
    ~Parser();

private:
    /// The loki parser and domain translation, kept out of this header: a parser is a high-level entity, and its
    /// includers need only the tyr domain and tasks.
    struct LokiState;

    std::unique_ptr<LokiState> m_loki;
    PlanningDomain m_domain;
};
}

#endif
