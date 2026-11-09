#ifndef YGG_TESTS_UNIT_DATABASE_QUERY_HELPERS_HPP_
#define YGG_TESTS_UNIT_DATABASE_QUERY_HELPERS_HPP_

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <map>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <vector>
#include <tyr/datalog/lifted/database/semantics/distance.hpp>
#include <tyr/datalog/lifted/database/semantics/operations.hpp>
#include <tyr/datalog/lifted/database/semantics/relation_view.hpp>
#include <tyr/datalog/lifted/database/syntax/query.hpp>

/// Test-only query construction on top of checkout + insert_query.
namespace tyr::datalog::tests::qb
{
/// A schema whose columns all have type T (by default ygg::uint_t).
template<ygg::ColumnValueType T = ygg::uint_t>
std::vector<ygg::ColumnLayout> columns_of(std::span<const urd::index<ygg::Column>> labels)
{
    std::vector<ygg::ColumnLayout> columns;
    for (const auto label : labels)
        columns.push_back({ label, ygg::type_id<T> });
    return columns;
}
template<ygg::ColumnValueType T = ygg::uint_t>
std::vector<ygg::ColumnLayout> columns_of(std::initializer_list<urd::index<ygg::Column>> labels)
{ return columns_of<T>(std::span<const urd::index<ygg::Column>>(labels)); }

inline database::QueryRepository repository() { return database::QueryRepository(); }

inline database::QueryBuilder& builder()
{
    static database::QueryBuilder storage;
    return storage;
}

inline urd::index<database::Query<>> require(const database::QueryRepository& repository, database::QueryView<> query)
{
    if (&get_repository(query.context()) != &repository)
        throw std::invalid_argument("Query: operand belongs to another repository.");
    return query.index();
}

inline database::QueryView<> input(database::QueryRepository& repository, size_t slot, std::span<const ygg::ColumnLayout> columns)
{
    auto data = database::checkout<database::Query<database::QueryInputTag>>(builder());
    data->input_slot = slot;
    data->columns.set(columns.begin(), columns.end());
    return database::insert_query(repository, builder(), *data);
}
inline database::QueryView<> input(database::QueryRepository& repository, size_t slot, std::span<const urd::index<ygg::Column>> columns)
{ return input(repository, slot, columns_of(columns)); }
inline database::QueryView<> input(database::QueryRepository& repository, size_t slot, std::initializer_list<urd::index<ygg::Column>> columns)
{ return input(repository, slot, std::span<const urd::index<ygg::Column>>(columns)); }

inline database::QueryView<> empty(database::QueryRepository& repository, std::span<const ygg::ColumnLayout> columns)
{
    auto data = database::checkout<database::Query<database::QueryEmptyTag>>(builder());
    data->columns.set(columns.begin(), columns.end());
    return database::insert_query(repository, builder(), *data);
}
inline database::QueryView<> empty(database::QueryRepository& repository, std::span<const urd::index<ygg::Column>> columns)
{ return empty(repository, columns_of(columns)); }
inline database::QueryView<> empty(database::QueryRepository& repository, std::initializer_list<urd::index<ygg::Column>> columns)
{ return empty(repository, std::span<const urd::index<ygg::Column>>(columns)); }

template<typename Tag>
database::QueryView<> binary(database::QueryRepository& repository, database::QueryView<> lhs, database::QueryView<> rhs)
{
    auto data = database::checkout<database::Query<Tag>>(builder());
    data->lhs = require(repository, lhs);
    data->rhs = require(repository, rhs);
    return database::insert_query(repository, builder(), *data);
}
inline database::QueryView<> join(database::QueryRepository& repository, database::QueryView<> lhs, database::QueryView<> rhs)
{ return binary<database::QueryJoinTag>(repository, lhs, rhs); }
inline database::QueryView<> union_(database::QueryRepository& repository, database::QueryView<> lhs, database::QueryView<> rhs)
{ return binary<database::QueryUnionTag>(repository, lhs, rhs); }
inline database::QueryView<> difference(database::QueryRepository& repository, database::QueryView<> lhs, database::QueryView<> rhs)
{ return binary<database::QueryDifferenceTag>(repository, lhs, rhs); }

inline database::QueryView<> semijoin(database::QueryRepository& repository, database::QueryView<> lhs, database::QueryView<> rhs)
{ return binary<database::QuerySemijoinTag>(repository, lhs, rhs); }
inline database::QueryView<> antijoin(database::QueryRepository& repository, database::QueryView<> lhs, database::QueryView<> rhs)
{ return binary<database::QueryAntijoinTag>(repository, lhs, rhs); }

template<typename Tag>
database::QueryView<> relabel(database::QueryRepository& repository, database::QueryView<> arg, std::span<const urd::index<ygg::Column>> labels)
{
    auto data = database::checkout<database::Query<Tag>>(builder());
    data->arg = require(repository, arg);
    data->labels.set(labels.begin(), labels.end());
    return database::insert_query(repository, builder(), *data);
}
inline database::QueryView<> project(database::QueryRepository& repository, database::QueryView<> arg, std::span<const urd::index<ygg::Column>> labels)
{ return relabel<database::QueryProjectTag>(repository, arg, labels); }
inline database::QueryView<> project(database::QueryRepository& repository, database::QueryView<> arg, std::initializer_list<urd::index<ygg::Column>> labels)
{ return project(repository, arg, std::span<const urd::index<ygg::Column>>(labels)); }
inline database::QueryView<> rename(database::QueryRepository& repository, database::QueryView<> arg, std::span<const urd::index<ygg::Column>> labels)
{ return relabel<database::QueryRenameTag>(repository, arg, labels); }
inline database::QueryView<> rename(database::QueryRepository& repository, database::QueryView<> arg, std::initializer_list<urd::index<ygg::Column>> labels)
{ return rename(repository, arg, std::span<const urd::index<ygg::Column>>(labels)); }

inline database::QueryView<>
select_equal(database::QueryRepository& repository, database::QueryView<> arg, urd::index<ygg::Column> lhs, urd::index<ygg::Column> rhs)
{
    auto data = database::checkout<database::Query<database::QuerySelectEqualTag>>(builder());
    data->arg = require(repository, arg);
    data->lhs_column = lhs;
    data->rhs_column = rhs;
    return database::insert_query(repository, builder(), *data);
}
inline database::QueryView<>
select_encoded(database::QueryRepository& repository, database::QueryView<> arg, urd::index<ygg::Column> column, ygg::Value value, ygg::TypeId type)
{
    auto data = database::checkout<database::Query<database::QuerySelectValueTag>>(builder());
    data->arg = require(repository, arg);
    data->column = column;
    data->constant = value;
    data->constant_type = type;
    return database::insert_query(repository, builder(), *data);
}
template<ygg::ColumnValueType T>
database::QueryView<> select_value(database::QueryRepository& repository, database::QueryView<> arg, urd::index<ygg::Column> column, T value)
{
    const auto columns = arg.columns();
    const auto found = std::ranges::find(columns, column, &ygg::ColumnLayout::label);
    if (found == columns.end())
        throw std::out_of_range("Columns: unknown column label.");
    if (found->type != ygg::type_id<T>)
        throw std::invalid_argument("Query: constant type does not match selected column.");
    return select_encoded(repository, arg, column, ygg::encode_value(value), ygg::type_id<T>);
}

inline database::QueryView<> distance(database::QueryRepository& repository,
                                      database::QueryView<> sources,
                                      database::QueryView<> edges,
                                      database::QueryView<> targets,
                                      urd::index<ygg::Column> distance_column)
{
    auto data = database::checkout<database::Query<database::QueryDistanceTag>>(builder());
    data->sources = require(repository, sources);
    data->edges = require(repository, edges);
    data->targets = require(repository, targets);
    data->distance_column = distance_column;
    return database::insert_query(repository, builder(), *data);
}

inline database::QueryView<> generic_join(database::QueryRepository& repository,
                                          std::span<const database::QueryView<>> inputs,
                                          std::span<const urd::index<ygg::Column>> variable_order,
                                          std::span<const urd::index<ygg::Column>> output_order = {})
{
    auto data = database::checkout<database::Query<database::QueryGenericJoinTag>>(builder());
    for (const auto input : inputs)
        data->inputs.push_back(require(repository, input));
    data->variable_order.set(variable_order.begin(), variable_order.end());
    data->output_order.set(output_order.begin(), output_order.end());
    return database::insert_query(repository, builder(), *data);
}
inline database::QueryView<> generic_join(database::QueryRepository& repository,
                                          std::initializer_list<database::QueryView<>> inputs,
                                          std::initializer_list<urd::index<ygg::Column>> variable_order,
                                          std::initializer_list<urd::index<ygg::Column>> output_order = {})
{
    return generic_join(repository,
                        std::span<const database::QueryView<>>(inputs),
                        std::span<const urd::index<ygg::Column>>(variable_order),
                        std::span<const urd::index<ygg::Column>>(output_order));
}
}  // namespace tyr::datalog::tests::qb

/// Test-only evaluation of one operation: its plan built from the inputs' columns, into a new output of the plan's
/// columns (the library takes the plan, the output and the workspace explicitly).
namespace tyr::datalog::tests::ops
{
/// uint_t columns labeled `labels`.
inline std::vector<ygg::ColumnLayout> cols(std::initializer_list<urd::index<ygg::Column>> labels) { return qb::columns_of(labels); }

template<typename V>
urd::builder<ygg::Relation<ygg::Schema>> projected(const V& input, std::span<const urd::index<ygg::Column>> labels)
{
    const database::ProjectionPlan plan(input.columns(), labels);
    urd::builder<ygg::Relation<ygg::Schema>> out(plan.output_columns());
    database::Workspace workspace;
    database::project(input, plan, out, workspace);
    return out;
}
template<typename V>
urd::builder<ygg::Relation<ygg::Schema>> projected(const V& input, std::initializer_list<urd::index<ygg::Column>> labels)
{ return projected(input, std::span<const urd::index<ygg::Column>>(labels)); }
template<typename L, typename R>
urd::builder<ygg::Relation<ygg::Schema>> joined(const L& lhs, const R& rhs)
{
    const database::JoinPlan plan(lhs.columns(), rhs.columns());
    urd::builder<ygg::Relation<ygg::Schema>> out(plan.output_columns());
    database::Workspace workspace;
    database::join(lhs, rhs, plan, out, workspace);
    return out;
}
template<typename L, typename R>
urd::builder<ygg::Relation<ygg::Schema>> unioned(const L& lhs, const R& rhs)
{
    const database::SetPlan plan(lhs.columns(), rhs.columns());
    urd::builder<ygg::Relation<ygg::Schema>> out(plan.output_columns());
    database::union_(lhs, rhs, plan, out);
    return out;
}
template<typename L, typename R>
urd::builder<ygg::Relation<ygg::Schema>> subtracted(const L& lhs, const R& rhs)
{
    const database::SetPlan plan(lhs.columns(), rhs.columns());
    urd::builder<ygg::Relation<ygg::Schema>> out(plan.output_columns());
    database::difference(lhs, rhs, plan, out);
    return out;
}
template<typename S, typename E, typename T>
urd::builder<ygg::Relation<ygg::Schema>> distances(const S& sources, const E& edges, const T& targets, const database::DistancePlan& plan)
{
    urd::builder<ygg::Relation<ygg::Schema>> out(plan.output_columns());
    database::DistanceWorkspace workspace(plan);
    database::distance(sources, edges, targets, plan, out, workspace);
    return out;
}
}  // namespace tyr::datalog::tests::ops

/// Shared by the min-distance tests: relations borrowed as views, rows decoded to uint_t values, reference distances.
namespace tyr::datalog::tests::distances
{
inline ygg::FreeBuilderRelationView borrow(const ygg::FreeRelationRepository& repository, const urd::builder<ygg::Relation<ygg::Schema>>& relation)
{ return repository.view(relation); }

inline urd::Store<urd::sequence<ygg::FreeBuilderRelationView>> borrow_all(const ygg::FreeRelationRepository& repository,
                                                                         const urd::Store<urd::sequence<urd::builder<ygg::Relation<ygg::Schema>>>>& relations)
{
    urd::Store<urd::sequence<ygg::FreeBuilderRelationView>> views;
    views.reserve(relations.size());
    for (const auto& relation : relations)
        views.push_back(borrow(repository, relation));
    return views;
}

inline urd::Store<urd::sequence<ygg::uint_t>> decode(std::span<const ygg::Value> row)
{
    urd::Store<urd::sequence<ygg::uint_t>> values;
    values.reserve(row.size());
    for (const auto value : row)
        values.push_back(ygg::decode_value<ygg::uint_t>(value));
    return values;
}
template<ygg::RowViewConcept<ygg::Schema> R>
urd::Store<urd::sequence<ygg::uint_t>> decode(const R& row)
{
    urd::Store<urd::sequence<ygg::uint_t>> values;
    values.reserve(row.size());
    for (size_t c = 0; c < row.size(); ++c)
        values.push_back(ygg::decode_value<ygg::uint_t>(row.value(c)));
    return values;
}

/// Half the distance to the targets in a supergraph of every state: consistent in all of them.
struct HalfDistance
{
    std::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t> distances;
    std::span<const ygg::ColumnLayout> columns;

    [[nodiscard]] ygg::uint_t estimate(std::span<const ygg::Value> vertex) const
    {
        const auto found = distances.find(decode(vertex));
        return found == distances.end() ? std::numeric_limits<ygg::uint_t>::max() : found->second / 2;
    }
};

/// Backward breadth-first distances to the targets over materialized edges.
inline std::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t> distances_to(ygg::FreeBuilderRelationView edges, ygg::FreeBuilderRelationView targets, size_t k)
{
    std::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t> result;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> frontier;
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const auto vertex = decode(targets.row(i));
        if (result.emplace(vertex, 0).second)
            frontier.push_back(vertex);
    }
    for (ygg::uint_t level = 1; !frontier.empty(); ++level)
    {
        urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> next;
        for (size_t i = 0; i < edges.size(); ++i)
        {
            const auto edge = decode(edges.row(i));
            const auto from = urd::Store<urd::sequence<ygg::uint_t>>(edge.begin(), edge.begin() + static_cast<std::ptrdiff_t>(k));
            const auto to = urd::Store<urd::sequence<ygg::uint_t>>(edge.begin() + static_cast<std::ptrdiff_t>(k), edge.end());
            if (std::ranges::find(frontier, to) != frontier.end() && result.emplace(from, level).second)
                next.push_back(from);
        }
        frontier = std::move(next);
    }
    return result;
}
}  // namespace tyr::datalog::tests::distances

#endif
