#include "tyr/formalism/planning/formatter.hpp"

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"

#include <boost/json.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <loki/semantic/parser.hpp>
#include <optional>
#include <string>
#include <yggdrasil/io/project_path.hpp>
#include <yggdrasil/serialization/json.hpp>

namespace tyr::tests::planning_formatter
{

namespace
{

/// How many of each part a task and its domain have, as tests/support/generators/formalism/planning/
/// generate_parser_counts_suite.py records them.
boost::json::object counts(tyr::formalism::planning::TaskView<::tyr::LiftedTag> task)
{
    const auto domain = task.get_domain();
    return boost::json::object {
        { "domain",
          boost::json::object {
              { "constants", domain.get_constants().size() },
              { "static_predicates", domain.get_static_predicates().size() },
              { "fluent_predicates", domain.get_fluent_predicates().size() },
              { "derived_predicates", domain.get_derived_predicates().size() },
              { "static_functions", domain.get_static_functions().size() },
              { "fluent_functions", domain.get_fluent_functions().size() },
              { "has_auxiliary_function", domain.get_auxiliary_function().has_value() },
              { "actions", domain.get_actions().size() },
              { "axioms", domain.get_axioms().size() },
          } },
        { "task",
          boost::json::object {
              { "objects", task.get_objects().size() },
              { "derived_predicates", task.get_derived_predicates().size() },
              { "static_atoms", task.get_static_atoms().size() },
              { "fluent_atoms", task.get_fluent_atoms().size() },
              { "static_fterm_values", task.get_static_fterm_values().size() },
              { "fluent_fterm_values", task.get_fluent_fterm_values().size() },
              { "has_auxiliary_fterm_value", task.get_auxiliary_fterm_value().has_value() },
              { "axioms", task.get_axioms().size() },
              { "has_metric", task.get_metric().has_value() },
          } },
    };
}

}  // namespace

/// Every case of the parser counts suite (pypddl-datasets' test corpus) under each parser configuration: tyr parses it
/// to the recorded counts, prints it as PDDL that loki parses, and reads that text back to the same counts and the
/// same text.
TEST(TyrFormalismPlanningFormatter, PrintsTheParserCountsSuiteAsPddlThatParsesBack)
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / "tests/support/data/formalism/planning/parser_counts_suite.json");
    const auto& cases = ygg::common::as_array(ygg::common::as_object(suite, "suite"), "cases", "suite");
    ASSERT_FALSE(cases.empty());
    for (const auto& case_value : cases)
    {
        const auto& case_object = ygg::common::as_object(case_value, "case");
        const auto domain_file = ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "domain_file", "case"));
        const auto task_file = ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(case_object, "task_file", "case"));
        for (const auto& configuration_value : ygg::common::as_array(case_object, "configurations", "case"))
        {
            const auto& configuration = ygg::common::as_object(configuration_value, "configuration");
            SCOPED_TRACE(fmt::format("{} {}", ygg::common::as_string(case_object, "name", "case"), boost::json::serialize(configuration.at("parser_options"))));
            auto options = loki::semantic::ParserOptions {};
            options.add_action_costs = configuration.at("parser_options").as_object().at("add_action_costs").as_bool();
            const auto expected = boost::json::object { { "domain", configuration.at("domain") }, { "task", configuration.at("task") } };

            auto parser = tyr::formalism::planning::Parser(domain_file, options);
            const auto task = parser.parse_task(task_file, options);
            EXPECT_EQ(counts(task.get_task()), expected);

            // The text is the domain, then the problem.
            const auto text = fmt::format("{}", task);
            const auto problem_begin = text.find("\n(define (problem");
            ASSERT_NE(problem_begin, std::string::npos);
            const auto domain_text = text.substr(0, problem_begin);
            const auto problem_text = text.substr(problem_begin + 1);

            auto loki_parser = loki::semantic::Parser(domain_text, options);
            static_cast<void>(loki_parser.parse_task(problem_text));

            auto reparser = tyr::formalism::planning::Parser(domain_text, std::nullopt, options);
            const auto reparsed = reparser.parse_task(problem_text, std::nullopt, options);
            EXPECT_EQ(counts(reparsed.get_task()), expected);
            EXPECT_EQ(fmt::format("{}", reparsed), text);
        }
    }
}

}  // namespace tyr::tests::planning_formatter
