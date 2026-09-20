#pragma once

#include <filesystem>
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
    auto suite = std::filesystem::path {};
    auto remaining = 1;
    for (int i = 1; i < argc; ++i)
    {
        const auto argument = std::string_view(argv[i]);
        if (argument.starts_with("--suite-json="))
            suite = argument.substr(13);
        else if (argument == "--suite-json")
        {
            if (++i == argc)
                throw std::invalid_argument("Missing value for --suite-json.");
            suite = argv[i];
        }
        else
            argv[remaining++] = argv[i];
    }
    argc = remaining;
    argv[argc] = nullptr;
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
