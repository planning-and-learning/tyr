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

#ifndef TYR_DATALOG_STATISTICS_RULE_HPP_
#define TYR_DATALOG_STATISTICS_RULE_HPP_

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tyr::datalog
{

struct RuleStatistics
{
    uint64_t num_executions { 0 };
    std::chrono::nanoseconds initialize_time { 0 };
    std::chrono::nanoseconds process_generate_time { 0 };
    std::chrono::nanoseconds total_time { 0 };
};

struct RuleWorkerStatistics
{
    uint64_t num_executions { 0 };
    uint64_t num_generated_rules { 0 };
    uint64_t num_rejected_rules { 0 };
};

struct AggregatedRuleStatistics
{
    uint64_t num_executions { 0 };
    std::chrono::nanoseconds initialize_time { 0 };
    std::chrono::nanoseconds process_generate_time { 0 };
    std::chrono::nanoseconds total_time { 0 };

    size_t sample_count { 0 };
    std::chrono::nanoseconds tot_time_min { 0 };
    std::chrono::nanoseconds tot_time_max { 0 };
    std::chrono::nanoseconds tot_time_median { 0 };
    std::chrono::nanoseconds avg_time_min { 0 };
    std::chrono::nanoseconds avg_time_max { 0 };
    std::chrono::nanoseconds avg_time_median { 0 };
};

struct AggregatedRuleWorkerStatistics
{
    uint64_t num_executions { 0 };
    uint64_t num_generated_rules { 0 };
    uint64_t num_rejected_rules { 0 };
};

inline AggregatedRuleStatistics compute_aggregated_rule_statistics(const std::vector<datalog::RuleStatistics>& statistics)
{
    AggregatedRuleStatistics result {};

    std::vector<std::chrono::nanoseconds> samples;
    samples.reserve(statistics.size());
    std::vector<std::chrono::nanoseconds> avg_samples;
    avg_samples.reserve(statistics.size());

    for (const auto& rs : statistics)
    {
        if (rs.num_executions == 0)
            continue;

        samples.push_back(rs.total_time);
        avg_samples.push_back(rs.total_time / rs.num_executions);
        result.num_executions += rs.num_executions;
        result.total_time += rs.total_time;
        result.initialize_time += rs.initialize_time;
        result.process_generate_time += rs.process_generate_time;
    }

    result.sample_count = samples.size();
    if (samples.empty())
        return result;

    std::sort(samples.begin(), samples.end(), [](auto a, auto b) { return a.count() < b.count(); });
    std::sort(avg_samples.begin(), avg_samples.end(), [](auto a, auto b) { return a.count() < b.count(); });

    result.tot_time_min = samples.front();
    result.tot_time_max = samples.back();

    result.avg_time_min = avg_samples.front();
    result.avg_time_max = avg_samples.back();

    const std::size_t n = samples.size();
    if (n % 2 == 1)
    {
        result.tot_time_median = samples[n / 2];
        result.avg_time_median = avg_samples[n / 2];
    }
    else
    {
        {
            const auto a = samples[n / 2 - 1].count();
            const auto b = samples[n / 2].count();
            result.tot_time_median = std::chrono::nanoseconds { (a + b) / 2 };
        }
        {
            const auto a = avg_samples[n / 2 - 1].count();
            const auto b = avg_samples[n / 2].count();
            result.avg_time_median = std::chrono::nanoseconds { (a + b) / 2 };
        }
    }

    return result;
}

inline AggregatedRuleWorkerStatistics compute_aggregated_rule_worker_statistics(const std::vector<RuleWorkerStatistics>& statistics)
{
    auto result = AggregatedRuleWorkerStatistics {};

    for (const auto& rs : statistics)
    {
        result.num_executions += rs.num_executions;
        result.num_generated_rules += rs.num_generated_rules;
        result.num_rejected_rules += rs.num_rejected_rules;
    }

    return result;
}

}

#endif
