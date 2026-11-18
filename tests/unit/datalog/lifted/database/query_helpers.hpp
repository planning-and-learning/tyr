#ifndef YGG_TESTS_UNIT_DATABASE_QUERY_HELPERS_HPP_
#define YGG_TESTS_UNIT_DATABASE_QUERY_HELPERS_HPP_

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <tyr/datalog/lifted/database/semantics/distance.hpp>
#include <tyr/datalog/lifted/database/semantics/operations.hpp>
#include <tyr/datalog/lifted/database/semantics/relation_view.hpp>
#include <tyr/datalog/lifted/database/syntax/query.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/formalism/builder_impl.hpp>  // the tests' own storage, RelationRecords, is no library's
#include <yggdrasil/formalism/canonicalization.hpp>
#include <yggdrasil/formalism/free_relations.hpp>
#include <yggdrasil/formalism/scratch_relation.hpp>

/// The scratch relations of the tests, and a relation drawn from them: what the tests build, read and pass as an
/// operation's output (it converts to the ygg::FreeScratchRelation it holds). Its rows are in tuple order.
namespace tyr::datalog::tests
{
inline ygg::FreeScratchRelations& scratch()
{
    static ygg::FreeScratchRelations relations;
    return relations;
}

class Relation
{
public:
    Relation() : Relation(std::span<const ygg::ColumnLayout>()) {}

    explicit Relation(std::span<const ygg::ColumnLayout> columns) : m_scratch(scratch().draw(columns)) {}

    // NOLINTNEXTLINE(google-explicit-constructor): passed where an operation takes its output
    operator ygg::FreeScratchRelation&() noexcept { return *m_scratch; }

    // NOLINTNEXTLINE(google-explicit-constructor): read where an operation checks its output against an input
    operator const ygg::FreeScratchRelation&() const noexcept { return *m_scratch; }

    [[nodiscard]] ygg::FreeScratchRelation& get() noexcept { return *m_scratch; }

    [[nodiscard]] ygg::FreeRelationBuilderView view() const { return m_scratch->view(); }

    [[nodiscard]] std::span<const ygg::ColumnLayout> columns() const { return view().columns(); }

    [[nodiscard]] size_t size() const { return view().size(); }

    [[nodiscard]] size_t arity() const { return view().arity(); }

    [[nodiscard]] bool empty() const { return view().empty(); }

    template<ygg::RowValues V>
    [[nodiscard]] bool contains(const V& values) const
    { return view().contains(values); }

    /// The position of the row of these values in tuple order, if it holds one.
    template<ygg::RowValues V>
    [[nodiscard]] std::optional<size_t> find(const V& values) const
    { return view().find(values); }

    /// Its position in tuple order.
    template<ygg::RowValues V>
    size_t insert(const V& values)
    { return m_scratch->insert(values).first; }

    template<ygg::RowValues V>
    bool erase(const V& values)
    { return m_scratch->erase(values); }

    void clear() noexcept { m_scratch->clear(); }

    void initialize(std::span<const ygg::ColumnLayout> columns) { m_scratch->relation.initialize(m_scratch->rows, columns); }

    void rename(std::span<const ygg::ColumnLayout> columns) { m_scratch->relation.rename(m_scratch->rows, columns); }

    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(m_scratch->relation); }

private:
    urd::handle<ygg::FreeScratchRelation> m_scratch;
};

inline ygg::FreeRelationBuilderView borrow(const Relation& relation) { return relation.view(); }

/// A change of a relation, as a test describes one: its added and its removed rows.
struct Change
{
    Relation added;
    Relation removed;

    explicit Change(std::span<const ygg::ColumnLayout> columns) : added(columns), removed(columns) {}

    void clear() noexcept
    {
        added.clear();
        removed.clear();
    }

    /// The (added, removed) change, as views.
    [[nodiscard]] std::pair<ygg::FreeRelationBuilderView, ygg::FreeRelationBuilderView> change() const { return { added.view(), removed.view() }; }

    [[nodiscard]] size_t memory_usage() const noexcept { return added.memory_usage() + removed.memory_usage(); }
};

/// A row's values, as it is held: a row view, or a builder's row child resolved.
template<typename R>
std::vector<ygg::Value> values_of(const R& row)
{ return ygg::with_values(row, [](const auto& values) { return std::vector<ygg::Value>(std::ranges::begin(values), std::ranges::end(values)); }); }

/// The record of a scratch relation's row (its rows are records, held by their handles): the row stays the same row
/// while this stays.
template<typename R>
const void* record_of(const R& row)
{ return &std::get<2>(row).get_values(); }

/// The value of a row's column, as it is held.
template<typename R>
ygg::Value value_of(const R& row, size_t column)
{ return ygg::with_values(row, [column](const auto& values) { return std::ranges::begin(values)[static_cast<std::ptrdiff_t>(column)]; }); }
}  // namespace tyr::datalog::tests

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
database::QueryView<> binary(database::QueryRepository& repository, database::QueryView<> left_operand, database::QueryView<> right_operand)
{
    auto data = database::checkout<database::Query<Tag>>(builder());
    data->left_operand = require(repository, left_operand);
    data->right_operand = require(repository, right_operand);
    return database::insert_query(repository, builder(), *data);
}

inline database::QueryView<> join(database::QueryRepository& repository, database::QueryView<> left_operand, database::QueryView<> right_operand)
{ return binary<database::QueryJoinTag>(repository, left_operand, right_operand); }

inline database::QueryView<> union_(database::QueryRepository& repository, database::QueryView<> left_operand, database::QueryView<> right_operand)
{ return binary<database::QueryUnionTag>(repository, left_operand, right_operand); }

inline database::QueryView<> difference(database::QueryRepository& repository, database::QueryView<> left_operand, database::QueryView<> right_operand)
{ return binary<database::QueryDifferenceTag>(repository, left_operand, right_operand); }

inline database::QueryView<> semijoin(database::QueryRepository& repository, database::QueryView<> left_operand, database::QueryView<> right_operand)
{ return binary<database::QuerySemijoinTag>(repository, left_operand, right_operand); }

inline database::QueryView<> antijoin(database::QueryRepository& repository, database::QueryView<> left_operand, database::QueryView<> right_operand)
{ return binary<database::QueryAntijoinTag>(repository, left_operand, right_operand); }

template<typename Tag>
database::QueryView<> relabel(database::QueryRepository& repository, database::QueryView<> arg, std::span<const urd::index<ygg::Column>> labels)
{
    auto data = database::checkout<database::Query<Tag>>(builder());
    data->operand = require(repository, arg);
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
select_equal(database::QueryRepository& repository, database::QueryView<> arg, urd::index<ygg::Column> left, urd::index<ygg::Column> right)
{
    auto data = database::checkout<database::Query<database::QuerySelectEqualTag>>(builder());
    data->operand = require(repository, arg);
    data->left_column = left;
    data->right_column = right;
    return database::insert_query(repository, builder(), *data);
}

inline database::QueryView<>
select_encoded(database::QueryRepository& repository, database::QueryView<> arg, urd::index<ygg::Column> column, ygg::Value value, ygg::TypeId type)
{
    auto data = database::checkout<database::Query<database::QuerySelectValueTag>>(builder());
    data->operand = require(repository, arg);
    data->column = column;
    data->constant = value;
    data->constant_type = type;
    return database::insert_query(repository, builder(), *data);
}

template<ygg::ColumnValueType T>
database::QueryView<> select_value(database::QueryRepository& repository, database::QueryView<> arg, urd::index<ygg::Column> column, T value)
{
    const auto columns = columns_of(arg);
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

/// The scratch records free relations are interned from (rows, columns, relation), reused by every test.
using RelationRecords = ::ygg::formalism::BuilderStorage<ygg::Row<ygg::Schema>, ygg::Columns, ygg::Relation<ygg::Schema>>;

inline RelationRecords& records()
{
    static RelationRecords storage;
    return storage;
}

/// Interns the columns and the relation of `relation` (its pool and rows set) in `repository`.
inline std::pair<ygg::FreeRelationView, bool>
intern_relation(ygg::FreeRelationRepository& repository, urd::data<ygg::Relation<ygg::Schema>>& relation, std::span<const ygg::ColumnLayout> labels)
{
    auto columns = checkout<ygg::Columns>(records());
    columns->values.set(labels.begin(), labels.end());
    relation.columns = repository.insert(*columns).first.index();
    return repository.insert(relation);
}

/// Interns the relation `source` reads (a builder's view, or a relation of any layer) in `repository`: each row, then the
/// columns, then the relation.
template<typename V>
std::pair<ygg::FreeRelationView, bool> intern(ygg::FreeRelationRepository& repository, const V& source)
{
    auto relation = checkout<ygg::Relation<ygg::Schema>>(records());
    relation->pool = repository.schema(source.columns());
    auto row = checkout<ygg::Row<ygg::Schema>>(records());
    row->relation = relation->pool;
    for (const auto& held : source.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             row->values.set(std::ranges::begin(values), std::ranges::end(values));
                             urd::append(relation->rows, repository.insert(*row).first);
                         });
    ygg::canonicalize(*relation, repository);
    return intern_relation(repository, *relation, source.columns());
}

inline std::pair<ygg::FreeRelationView, bool> intern(ygg::FreeRelationRepository& repository, const Relation& source)
{ return intern(repository, source.view()); }

/// `source` (of `repository`) under new columns: the same pool and rows.
inline ygg::FreeRelationView rename(ygg::FreeRelationRepository& repository, ygg::FreeRelationView source, std::span<const ygg::ColumnLayout> labels)
{
    auto relation = checkout<ygg::Relation<ygg::Schema>>(records());
    relation->pool = repository[source.index()].pool;
    urd::store(relation->rows, source.get_rows());
    return intern_relation(repository, *relation, labels).first;
}

template<typename V>
Relation projected(const V& input, std::span<const urd::index<ygg::Column>> labels)
{
    const database::ProjectionPlan plan(input.columns(), labels);
    Relation out(plan.output_columns());
    database::Workspace workspace;
    database::project(input, plan, out, workspace);
    return out;
}

template<typename V>
Relation projected(const V& input, std::initializer_list<urd::index<ygg::Column>> labels)
{ return projected(input, std::span<const urd::index<ygg::Column>>(labels)); }

template<typename L, typename R>
Relation joined(const L& left_operand, const R& right_operand)
{
    const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
    Relation out(plan.output_columns());
    database::Workspace workspace;
    database::join(left_operand, right_operand, plan, out, workspace);
    return out;
}

template<typename L, typename R>
Relation unioned(const L& left_operand, const R& right_operand)
{
    const database::SetPlan plan(left_operand.columns(), right_operand.columns());
    Relation out(plan.output_columns());
    database::union_(left_operand, right_operand, plan, out);
    return out;
}

template<typename L, typename R>
Relation subtracted(const L& left_operand, const R& right_operand)
{
    const database::SetPlan plan(left_operand.columns(), right_operand.columns());
    Relation out(plan.output_columns());
    database::difference(left_operand, right_operand, plan, out);
    return out;
}

template<typename S, typename E, typename T>
Relation distances(const S& sources, const E& edges, const T& targets, const database::DistancePlan& plan)
{
    Relation out(plan.output_columns());
    database::DistanceWorkspace workspace(plan);
    database::distance(sources, edges, targets, plan, out, workspace);
    return out;
}
}  // namespace tyr::datalog::tests::ops

/// Shared by the min-distance tests: relations borrowed as views, rows decoded to uint_t values, reference distances.
namespace tyr::datalog::tests::distances
{
inline urd::Store<urd::sequence<ygg::FreeRelationBuilderView>> borrow_all(const urd::Store<urd::sequence<Relation>>& relations)
{
    urd::Store<urd::sequence<ygg::FreeRelationBuilderView>> views;
    views.reserve(relations.size());
    for (const auto& relation : relations)
        views.push_back(relation.view());
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

/// A row's values, as it is held (a row view, or a builder's row child resolved), decoded.
template<typename R>
    requires(!std::convertible_to<const R&, std::span<const ygg::Value>>)
urd::Store<urd::sequence<ygg::uint_t>> decode(const R& row)
{
    const auto values = values_of(row);
    return decode(std::span<const ygg::Value>(values));
}

/// Half the distance to the targets in a supergraph of every state: consistent in all of them.
struct HalfDistance
{
    urd::Store<urd::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t>> distances;
    std::span<const ygg::ColumnLayout> columns;

    [[nodiscard]] ygg::uint_t estimate(std::span<const ygg::Value> vertex) const
    {
        const auto found = distances.find(decode(vertex));
        return found == distances.end() ? std::numeric_limits<ygg::uint_t>::max() : found->second / 2;
    }
};

/// Backward breadth-first distances to the targets over materialized edges.
inline urd::Store<urd::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t>>
distances_to(ygg::FreeRelationBuilderView edges, ygg::FreeRelationBuilderView targets, size_t k)
{
    urd::Store<urd::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t>> result;
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
