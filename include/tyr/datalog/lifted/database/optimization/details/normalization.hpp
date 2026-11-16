/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_NORMALIZATION_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_NORMALIZATION_HPP_

#include "tyr/datalog/lifted/database/optimization/details/join_block.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <optional>
#include <span>
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

/// Algebraic normalization by the classic heuristics (H. Garcia-Molina, J. D. Ullman,
/// J. Widom, "Database Systems: The Complete Book", 2nd ed., 2008, ch. 16): push
/// selections down through join, projection, rename, union, and difference (and into the
/// left operand of semijoins and antijoins); push projections through rename and union, and into the
/// left operand of semijoins and antijoins (keeping the labels shared with the right operand), never through
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

    QueryView<> join(QueryView<> left_operand, QueryView<> right_operand)
    {
        const auto result = m_build.join(left_operand, right_operand);
        return is<QueryEmptyTag>(left_operand) || is<QueryEmptyTag>(right_operand) ? m_build.empty(columns_of(result)) : result;
    }
    QueryView<> union_(QueryView<> left_operand, QueryView<> right_operand)
    {
        if (is<QueryEmptyTag>(left_operand))
            return right_operand;
        if (is<QueryEmptyTag>(right_operand))
            return left_operand;
        return m_build.template binary<QueryUnionTag>(left_operand, right_operand);
    }
    QueryView<> difference(QueryView<> left_operand, QueryView<> right_operand)
    {
        if (is<QueryEmptyTag>(left_operand) || is<QueryEmptyTag>(right_operand))
            return left_operand;
        return m_build.template binary<QueryDifferenceTag>(left_operand, right_operand);
    }
    /// A semijoin (`Tag` = QuerySemijoinTag) or antijoin: an empty right operand keeps nothing or everything.
    template<class Tag>
    QueryView<> filter_join(QueryView<> left_operand, QueryView<> right_operand)
    {
        if (is<QueryEmptyTag>(left_operand))
            return left_operand;
        if (is<QueryEmptyTag>(right_operand))
            return std::same_as<Tag, QuerySemijoinTag> ? m_build.empty(columns_of(left_operand)) : left_operand;
        return m_build.template binary<Tag>(left_operand, right_operand);
    }
    /// Labels of the renamed query's argument for the given renamed labels.
    static urd::Store<urd::sequence<urd::index<ygg::Column>>> inverse(QueryView<QueryRenameTag> rename, std::span<const urd::index<ygg::Column>> renamed)
    {
        const auto before = columns_of(rename.get_operand()), after = rename.columns();
        urd::Store<urd::sequence<urd::index<ygg::Column>>> result;
        for (const auto label : renamed)
            result.push_back(before[static_cast<size_t>(std::ranges::find(after, label, &ygg::ColumnLayout::label) - after.begin())].label);
        return result;
    }
    QueryView<> rename(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
    {
        if (std::ranges::equal(columns_of(arg), columns, {}, &ygg::ColumnLayout::label))
            return arg;
        const auto result = m_build.template relabel<QueryRenameTag>(arg, columns);
        return is<QueryEmptyTag>(arg) ? m_build.empty(columns_of(result)) : result;
    }
    QueryView<> project(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns);
    QueryView<> finish_project(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
    {
        const auto result = m_build.template relabel<QueryProjectTag>(arg, columns);
        return is<QueryEmptyTag>(arg) ? m_build.empty(columns_of(result)) : result;
    }

    /// The labels of `arg` in `columns` or in `extra`, in the order of `arg`.
    static urd::Store<urd::sequence<urd::index<ygg::Column>>>
    kept_labels(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns, std::span<const ygg::ColumnLayout> extra)
    {
        urd::Store<urd::sequence<urd::index<ygg::Column>>> labels;
        for (const auto& column : columns_of(arg))
            if (std::ranges::find(columns, column.label) != columns.end() || std::ranges::find(extra, column.label, &ygg::ColumnLayout::label) != extra.end())
                labels.push_back(column.label);
        return labels;
    }
    template<class Tag>
    QueryView<> project_filter_join(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
    {
        const auto operation = as<Tag>(arg);
        const auto left_operand = operation.get_left_operand();
        const auto kept = kept_labels(left_operand, columns, columns_of(operation.get_right_operand()));
        // Nothing to drop below: the projection stays above.
        if (kept.size() == columns_of(left_operand).size())
            return finish_project(arg, columns);
        return project(filter_join<Tag>(project(left_operand, kept), operation.get_right_operand()), columns);
    }

    /// A selection's condition alone (its columns; a value selection's constant too): what select carries down. The
    /// operand is set where the selection is built, and constructing the query plans it.
    static Operation<QuerySelectEqualTag> condition(urd::index<ygg::Column> left_column, urd::index<ygg::Column> right_column)
    {
        auto result = Operation<QuerySelectEqualTag>();
        result.left_column = left_column;
        result.right_column = right_column;
        return result;
    }
    static Operation<QuerySelectValueTag> condition(urd::index<ygg::Column> column, ygg::Value constant, ygg::TypeId constant_type)
    {
        auto result = Operation<QuerySelectValueTag>();
        result.column = column;
        result.constant = constant;
        result.constant_type = constant_type;
        return result;
    }
    static Operation<QuerySelectEqualTag> condition(const Operation<QuerySelectEqualTag>& data) { return condition(data.left_column, data.right_column); }
    static Operation<QuerySelectValueTag> condition(const Operation<QuerySelectValueTag>& data)
    { return condition(data.column, data.constant, data.constant_type); }
    static Operation<QuerySelectEqualTag> condition(QueryView<QuerySelectEqualTag> query)
    { return condition(query.get_left_column(), query.get_right_column()); }
    static Operation<QuerySelectValueTag> condition(QueryView<QuerySelectValueTag> query)
    { return condition(query.get_column(), query.get_constant(), query.get_constant_type()); }

    static urd::Store<urd::sequence<urd::index<ygg::Column>>> tested(const Operation<QuerySelectEqualTag>& data)
    { return { data.left_column, data.right_column }; }
    static urd::Store<urd::sequence<urd::index<ygg::Column>>> tested(const Operation<QuerySelectValueTag>& data) { return { data.column }; }
    static void retarget(Operation<QuerySelectEqualTag>& data, QueryView<QueryRenameTag> rename)
    {
        const std::array renamed { data.left_column, data.right_column };
        const auto inner = inverse(rename, renamed);
        data.left_column = inner[0];
        data.right_column = inner[1];
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
        {
            return std::ranges::all_of(columns,
                                       [&](urd::index<ygg::Column> label)
                                       {
                                           const auto schema = columns_of(query);
                                           return std::ranges::find(schema, label, &ygg::ColumnLayout::label) != schema.end();
                                       });
        };
        if (is<QueryEmptyTag>(arg))
            return arg;
        if (is<QueryJoinTag>(arg))
        {
            const auto operation = as<QueryJoinTag>(arg);
            if (within(operation.get_left_operand()))
                return join(select(data, operation.get_left_operand()), operation.get_right_operand());
            if (within(operation.get_right_operand()))
                return join(operation.get_left_operand(), select(data, operation.get_right_operand()));
        }
        if (is<QueryProjectTag>(arg))
        {
            const auto operation = as<QueryProjectTag>(arg);
            return project(select(data, operation.get_operand()), operation.get_labels());
        }
        if (is<QueryRenameTag>(arg))
        {
            const auto operation = as<QueryRenameTag>(arg);
            auto inner = condition(data);
            retarget(inner, operation);
            return rename(select(inner, operation.get_operand()), operation.get_labels());
        }
        if (is<QueryUnionTag>(arg))
        {
            const auto operation = as<QueryUnionTag>(arg);
            return union_(select(data, operation.get_left_operand()), select(data, operation.get_right_operand()));
        }
        if (is<QueryDifferenceTag>(arg))
        {
            const auto operation = as<QueryDifferenceTag>(arg);
            return difference(select(data, operation.get_left_operand()), select(data, operation.get_right_operand()));
        }
        // The output of a semijoin or antijoin is its left operand.
        if (is<QuerySemijoinTag>(arg))
        {
            const auto operation = as<QuerySemijoinTag>(arg);
            return filter_join<QuerySemijoinTag>(select(data, operation.get_left_operand()), operation.get_right_operand());
        }
        if (is<QueryAntijoinTag>(arg))
        {
            const auto operation = as<QueryAntijoinTag>(arg);
            return filter_join<QueryAntijoinTag>(select(data, operation.get_left_operand()), operation.get_right_operand());
        }
        return m_build.template make<Tag>(
            [&](auto& filtered)
            {
                filtered = condition(data);
                filtered.operand = arg.index();
            });
    }

    /// Rebuilds a query whose children are already normalized.
    template<class Tag>
    QueryView<> normalize(QueryView<> source, QueryView<Tag> query)
    {
        const auto child = [&](QueryView<> operand) { return (*this)(operand); };
        if constexpr (std::same_as<Tag, QueryJoinTag>)
            return join(child(query.get_left_operand()), child(query.get_right_operand()));
        else if constexpr (std::same_as<Tag, QueryUnionTag>)
            return union_(child(query.get_left_operand()), child(query.get_right_operand()));
        else if constexpr (std::same_as<Tag, QueryDifferenceTag>)
            return difference(child(query.get_left_operand()), child(query.get_right_operand()));
        else if constexpr (std::same_as<Tag, QuerySemijoinTag> || std::same_as<Tag, QueryAntijoinTag>)
            return filter_join<Tag>(child(query.get_left_operand()), child(query.get_right_operand()));
        else if constexpr (std::same_as<Tag, QueryProjectTag>)
            return project(child(query.get_operand()), query.get_labels());
        else if constexpr (std::same_as<Tag, QueryRenameTag>)
            return rename(child(query.get_operand()), query.get_labels());
        else if constexpr (std::same_as<Tag, QuerySelectEqualTag> || std::same_as<Tag, QuerySelectValueTag>)
            return select(condition(query), child(query.get_operand()));
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
                    return m_build.empty(columns_of(result));
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
