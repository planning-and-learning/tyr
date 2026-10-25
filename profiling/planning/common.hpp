#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::profiling
{
struct BenchmarkCase
{
    std::string name;
    std::filesystem::path domain;
    std::filesystem::path task;
};

inline std::filesystem::path extract_suite_path(int& argc, char** argv)
{
    constexpr auto suite_option = std::string_view("--suite-json");
    constexpr auto suite_assignment = std::string_view("--suite-json=");
    // argv[argc] is the terminating null pointer.
    const auto arguments = std::span(argv, static_cast<std::size_t>(argc) + 1);
    const auto num_arguments = static_cast<std::size_t>(argc);
    auto suite = std::filesystem::path {};
    auto remaining = std::size_t { 1 };
    for (std::size_t i = 1; i < num_arguments; ++i)
    {
        const auto argument = std::string_view(arguments[i]);
        if (argument.starts_with(suite_assignment))
            suite = argument.substr(suite_assignment.size());
        else if (argument == suite_option)
        {
            if (++i == num_arguments)
                throw std::invalid_argument("Missing value for --suite-json.");
            suite = arguments[i];
        }
        else
            arguments[remaining++] = arguments[i];
    }
    argc = static_cast<int>(remaining);
    arguments[remaining] = nullptr;
    if (suite.empty())
        throw std::invalid_argument("Required option: --suite-json=<path>.");
    return suite;
}

inline std::vector<BenchmarkCase> load_suite(const std::filesystem::path& suite)
{
    const auto document = ygg::common::load_json_file(suite);
    const auto& root = ygg::common::as_object(document, "suite");
    const auto prefix = root.contains("prefix") ? ygg::common::suite_prefix_path(root) : std::filesystem::path(BENCHMARKS_DIR);
    auto cases = std::vector<BenchmarkCase> {};
    for (const auto& [domain_name, domain_value] : ygg::common::as_object(root, "domains", "suite"))
    {
        const auto& domain = ygg::common::as_object(domain_value, "domain");
        for (const auto& [task_name, task_value] : ygg::common::as_object(domain, "tasks", "domain"))
            cases.push_back({ std::string(domain_name) + "/" + std::string(task_name),
                              ygg::common::resolve_path(prefix, ygg::common::as_string(domain, "domain_file", "domain")),
                              ygg::common::resolve_path(prefix, ygg::common::as_string(task_value, "task")) });
    }
    if (cases.empty())
        throw std::invalid_argument("The suite must contain at least one task.");
    return cases;
}
}
