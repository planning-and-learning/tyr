/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_NORMALIZATION_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_NORMALIZATION_HPP_

#include "tyr/datalog/lifted/database/optimization/details/join_block.hpp"
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <optional>
#include <span>
#include <utility>
#include <vector>

/// Algebraic normalization by the classic heuristics (H. Garcia-Molina, J. D. Ullman,
/// J. Widom, "Database Systems: The Complete Book", 2nd ed., 2008, ch. 16): push
/// selections down through join, projection, rename, union, and difference (and into the
/// lhs of semijoins and antijoins); push projections through rename and union, and into the
/// lhs of semijoins and antijoins (keeping the labels shared with the rhs), never through
/// difference (selections already go below projections); collapse nested
/// projections; propagate empty relations. One deterministic bottom-up pass.
namespace tyr::datalog::database::detail
{
class Normalizer
{
    template<class Tag>
    using Operation = urd::data<Query<Tag>>;

    OperatorBuilder m_build;
    urd::Store<urd::map<urd::index<Query<>>, QueryView<>>, urd::dense> m_normalized;

    QueryView<> join(QueryView<> lhs, QueryView<> rhs)
    {
        const auto result = m_build.join(lhs, rhs);
        return is<QueryEmptyTag>(lhs) || is<QueryEmptyTag>(rhs) ? m_build.empty(result.columns()) : result;
    }
    QueryView<> union_(QueryView<> lhs, QueryView<> rhs)
    {
        if (is<QueryEmptyTag>(lhs))
            return rhs;
        if (is<QueryEmptyTag>(rhs))
            return lhs;
        return m_build.template binary<QueryUnionTag>(lhs, rhs);
    }
    QueryView<> difference(QueryView<> lhs, QueryView<> rhs)
    {
        if (is<QueryEmptyTag>(lhs) || is<QueryEmptyTag>(rhs))
            return lhs;
        return m_build.template binary<QueryDifferenceTag>(lhs, rhs);
    }
    /// A semijoin (`Tag` = QuerySemijoinTag) or antijoin: an empty rhs keeps nothing or everything.
    template<class Tag>
    QueryView<> filter_join(QueryView<> lhs, QueryView<> rhs)
    {
        if (is<QueryEmptyTag>(lhs))
            return lhs;
        if (is<QueryEmptyTag>(rhs))
            return std::same_as<Tag, QuerySemijoinTag> ? m_build.empty(lhs.columns()) : lhs;
        return m_build.template binary<Tag>(lhs, rhs);
    }
    /// Labels of the renamed query's argument for the given renamed labels.
    static urd::Store<urd::sequence<urd::index<ygg::Column>>> inverse(QueryView<QueryRenameTag> rename, std::span<const urd::index<ygg::Column>> renamed)
    {
        const auto before = rename.get_arg().columns(), after = rename.columns();
        urd::Store<urd::sequence<urd::index<ygg::Column>>> result;
        for (const auto label : renamed)
            result.push_back(before[static_cast<size_t>(std::ranges::find(after, label, &ygg::ColumnLayout::label) - after.begin())].label);
        return result;
    }
    QueryView<> rename(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
    {
        if (std::ranges::equal(arg.columns(), columns, {}, &ygg::ColumnLayout::label))
            return arg;
        const auto result = m_build.template relabel<QueryRenameTag>(arg, columns);
        return is<QueryEmptyTag>(arg) ? m_build.empty(result.columns()) : result;
    }
    QueryView<> project(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns);
    QueryView<> finish_project(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
    {
        const auto result = m_build.template relabel<QueryProjectTag>(arg, columns);
        return is<QueryEmptyTag>(arg) ? m_build.empty(result.columns()) : result;
    }

    /// The labels of `arg` in `columns` or in `extra`, in the order of `arg`.
    static urd::Store<urd::sequence<urd::index<ygg::Column>>> kept_labels(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns, std::span<const ygg::ColumnLayout> extra)
    {
        urd::Store<urd::sequence<urd::index<ygg::Column>>> labels;
        for (const auto& column : arg.columns())
            if (std::ranges::find(columns, column.label) != columns.end() || std::ranges::find(extra, column.label, &ygg::ColumnLayout::label) != extra.end())
                labels.push_back(column.label);
        return labels;
    }
    template<class Tag>
    QueryView<> project_filter_join(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
    {
        const auto operation = as<Tag>(arg);
        const auto lhs = operation.get_lhs();
        const auto kept = kept_labels(lhs, columns, operation.get_rhs().columns());
        // Nothing to drop below: the projection stays above.
        if (kept.size() == lhs.columns().size())
            return finish_project(arg, columns);
        return project(filter_join<Tag>(project(lhs, kept), operation.get_rhs()), columns);
    }

    static urd::Store<urd::sequence<urd::index<ygg::Column>>> tested(const Operation<QuerySelectEqualTag>& data) { return { data.lhs_column, data.rhs_column }; }
    static urd::Store<urd::sequence<urd::index<ygg::Column>>> tested(const Operation<QuerySelectValueTag>& data) { return { data.column }; }
    static void retarget(Operation<QuerySelectEqualTag>& data, QueryView<QueryRenameTag> rename)
    {
        const std::array renamed { data.lhs_column, data.rhs_column };
        const auto inner = inverse(rename, renamed);
        data.lhs_column = inner[0];
        data.rhs_column = inner[1];
    }
    static void retarget(Operation<QuerySelectValueTag>& data, QueryView<QueryRenameTag> rename)
    {
        const std::array renamed { data.column };
        data.column = inverse(rename, renamed)[0];
    }
    template<class Tag>
    QueryView<> select(const Operation<Tag>& data, QueryView<> arg)
    {
        const auto columns = tested(data);
        const auto within = [&](QueryView<> query)
        { return std::ranges::all_of(columns, [&](urd::index<ygg::Column> label) { const auto schema = query.columns();
              return std::ranges::find(schema, label, &ygg::ColumnLayout::label) != schema.end(); }); };
        if (is<QueryEmptyTag>(arg))
            return arg;
        if (is<QueryJoinTag>(arg))
        {
            const auto operation = as<QueryJoinTag>(arg);
            if (within(operation.get_lhs()))
                return join(select(data, operation.get_lhs()), operation.get_rhs());
            if (within(operation.get_rhs()))
                return join(operation.get_lhs(), select(data, operation.get_rhs()));
        }
        if (is<QueryProjectTag>(arg))
        {
            const auto operation = as<QueryProjectTag>(arg);
            return project(select(data, operation.get_arg()), operation.get_labels());
        }
        if (is<QueryRenameTag>(arg))
        {
            const auto operation = as<QueryRenameTag>(arg);
            auto inner = data;
            retarget(inner, operation);
            return rename(select(inner, operation.get_arg()), operation.get_labels());
        }
        if (is<QueryUnionTag>(arg))
        {
            const auto operation = as<QueryUnionTag>(arg);
            return union_(select(data, operation.get_lhs()), select(data, operation.get_rhs()));
        }
        if (is<QueryDifferenceTag>(arg))
        {
            const auto operation = as<QueryDifferenceTag>(arg);
            return difference(select(data, operation.get_lhs()), select(data, operation.get_rhs()));
        }
        // The output of a semijoin or antijoin is its lhs.
        if (is<QuerySemijoinTag>(arg))
        {
            const auto operation = as<QuerySemijoinTag>(arg);
            return filter_join<QuerySemijoinTag>(select(data, operation.get_lhs()), operation.get_rhs());
        }
        if (is<QueryAntijoinTag>(arg))
        {
            const auto operation = as<QueryAntijoinTag>(arg);
            return filter_join<QueryAntijoinTag>(select(data, operation.get_lhs()), operation.get_rhs());
        }
        return m_build.template make<Tag>(
            [&](auto& filtered)
            {
                filtered = data;
                filtered.arg = arg.index();
            });
    }

    /// Rebuilds a query whose children are already normalized.
    template<class Tag>
    QueryView<> normalize(QueryView<> source, QueryView<Tag> query)
    {
        const auto child = [&](QueryView<> operand) { return (*this)(operand); };
        if constexpr (std::same_as<Tag, QueryJoinTag>)
            return join(child(query.get_lhs()), child(query.get_rhs()));
        else if constexpr (std::same_as<Tag, QueryUnionTag>)
            return union_(child(query.get_lhs()), child(query.get_rhs()));
        else if constexpr (std::same_as<Tag, QueryDifferenceTag>)
            return difference(child(query.get_lhs()), child(query.get_rhs()));
        else if constexpr (std::same_as<Tag, QuerySemijoinTag> || std::same_as<Tag, QueryAntijoinTag>)
            return filter_join<Tag>(child(query.get_lhs()), child(query.get_rhs()));
        else if constexpr (std::same_as<Tag, QueryProjectTag>)
            return project(child(query.get_arg()), query.get_labels());
        else if constexpr (std::same_as<Tag, QueryRenameTag>)
            return rename(child(query.get_arg()), query.get_labels());
        else if constexpr (std::same_as<Tag, QuerySelectEqualTag> || std::same_as<Tag, QuerySelectValueTag>)
            return select(query.data(), child(query.get_arg()));
        else
        {
            urd::Store<urd::sequence<QueryView<>>> children;
            bool absent = false;
            for_each_child(query,
                           [&](QueryView<> operand)
                           {
                               children.push_back(child(operand));
                               absent |= is<QueryEmptyTag>(children.back());
                           });
            const auto result = m_build.clone(source, children);
            if constexpr (std::same_as<Tag, QueryGenericJoinTag>)
                if (absent)
                    return m_build.empty(result.columns());
            return result;
        }
    }

public:
    Normalizer(QueryRepository& repository, QueryBuilder& builder) : m_build(repository, builder) {}

    /// The normalized equivalent of a query of the source repository.
    QueryView<> operator()(QueryView<> source)
    {
        if (const auto it = m_normalized.find(source.index()); it != m_normalized.end())
            return it->second;
        const auto normalized = std::visit([&](auto query) { return normalize(source, query); }, source.get_variant());
        m_normalized.insert_or_assign(source.index(), normalized);
        return normalized;
    }
};
}  // namespace tyr::datalog::database::detail

#endif
