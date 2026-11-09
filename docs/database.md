# Relational algebra

`<tyr/datalog/lifted/database/semantics/operations.hpp>` provides header-only set operations in
`tyr::datalog::database`, each in one form: its inputs, a plan built from their columns, an output builder and, where it needs
scratch storage, a `Workspace`. Relations have runtime columns and rows packed by their widths (urd's rows of unsigned columns,
`urd::by_column_widths`); the relation model itself (columns, schemas, row sets, relations) is yggdrasil formalism's.
Different columns can hold different types without a tag or variant in every cell.
Nullary relations and runtime arities remain supported.

## Types and schemas

Columns hold one of a fixed set of value types, the alternatives of `ygg::ColumnValue` (in
`<yggdrasil/formalism/values.hpp>`): `uint32_t`, `uint64_t`, `int32_t`, `int64_t`, `float`, `double`, `bool` and
`urd::index<formalism::Object<void>>` (object indices, in the bits the layer gives them). A column's `ygg::TypeId` is
its type's index in that variant (`type_id<T>`; an object index of any tag, `urd::index<formalism::Object<Tag>>`,
has the object alternative's), which columns store, so relations, schemas, plans and repositories carry no list of
value types. A relation's column types live where its rows are stored: the layer's pool of its key keeps them
(`relations().types(key)`, written when the pool is made in any layer and checked on every later use). Its `Columns`
record is interned by the layer only, from the caller's labels and the pool's types (insert, copy and rename check
the caller's types against the pool's once, `std::invalid_argument` otherwise). A row stores each value as an unsigned `ygg::Value` (`encode_value`, `decode_value`; `encode` and
`decode(bits, type)` for a `ColumnValue`).
Indices are stored as `uint_t` (`index.value`); the code that owns a relation
knows which tag a column holds and converts back with `Index<Tag>(value)`.

```cpp
#include <tyr/datalog/lifted/database/semantics/operations.hpp>

using namespace tyr::datalog::database;
const urd::index<Column> source(0), target(1), distance(2);

const std::array<ColumnLayout, 3> columns { ColumnLayout { source, type_id<std::uint32_t> },
                                            ColumnLayout { target, type_id<std::uint32_t> },
                                            ColumnLayout { distance, type_id<double> } };
Repository repository;
const auto paths = repository.get_relation_builder<Schema>(columns);
const std::array<ygg::Value, 3> first { ygg::encode_value(3u), ygg::encode_value(8u), ygg::encode_value(2.5) };
const std::array<ygg::Value, 3> second { ygg::encode_value(3u), ygg::encode_value(9u), ygg::encode_value(4.0) };
paths->insert(first);
paths->insert(second);

const auto view = repository.view(*paths);  // a BuilderRelationView
const auto length = ygg::decode_value<double>(view.row(0).value(2));
const std::array labels { source, target };
const ProjectionPlan plan(view.columns(), labels);
urd::builder<Relation<Schema>> reachable(plan.output_columns());
Workspace workspace;
project(view, plan, reachable, workspace);
```

`Column` identifies a schema-local label. Columns are a `std::span<const ColumnLayout>`: each `ColumnLayout` records a
label and a `TypeId`; where a column lies in a row, the builder's widths say (`.widths()`, an `urd::column_widths`:
each column whole, a bool in one bit, an object 32 bits; a layer's builders and pools: an object in the layer's object
bits). Interned `Columns` records identify by the ordered labels and
types. A label's position is found with `std::ranges::find(columns, label, &ColumnLayout::label)`. Labels must be
unique and types valid: `builder<Relation<F>>` validates them in its constructor, `initialize` and `rename`, and the
plans validate the columns they are built from (`ygg::validate_columns`).

A value is read, hashed, compared and written as unsigned bits (`ygg::Value`,
`encode_value`/`decode_value`; urd's rows know no types): unsigned integers as they are,
signed ones as their two's complement in their own width, floats as their bits with
signed zero and all NaNs normalized, bools 0 or 1. Equal values have equal bits, so
rows hash and compare by value. The set of types is closed; `decode(bits, type)` turns a
stored value and its `TypeId` back into a `ColumnValue` (`std::visit` it). No tolerance rounding is performed. This is an
in-process representation, not a portable wire format.

## Rows and ownership

The relation code is data (`formalism/relation_data.hpp`), builders
(`formalism/relation_builder.hpp`: `builder<Relation<F>>` and its `RowPool`),
views (`formalism/relation_view.hpp`) and the value encoding (`formalism/values.hpp`).
A relation belongs to a family `F` of the layer's relation types, whose pools (one per key `index<F>`) hold its rows;
free relations are the `Schema` family, keyed by their column types (interned when a builder is inserted). The
database's `Repository` has `Relation<Schema>` among its symbols (`HasRelations`; `HasRows` says a family has pools),
so only free relations (`builder<Relation<Schema>>`, `index_view<Relation<Schema>, Repository>`).
`urd::builder<Relation<F>>` owns its columns and a pool of its rows (`RowPool`, rows by the columns' widths); the pool
key is given where it is interned: `insert(builder)` for `Schema` (its column types), `insert(pool, builder)` for other
families. Like tyr's state builders it is read through a layer, `repository.view(builder)` or
`urd::resolve(builder, repository)`, a `BuilderRelationView`
(`urd::borrowed_view<Relation<Schema>, Repository, builder<Relation<Schema>>>`), and operations take only
views. A builder holds `urd::data<Columns>` and its rows, nothing else: no index and no content identity. It draws its
number (`layer()`, the context number of its rows' views and its `relation_identity()`) at construction from the
process-wide counter repository layers draw theirs from.

A view (interned `urd::index_view<Relation<F>, Context>` or `BuilderRelationView`) gives
`columns()` (a span of `ColumnLayout`), `size()`, `row(i)`, `rows()`, `contains(row)` and `relation_identity()`. An
interned view reads its row set over its pool through `urd::subset_view`; a builder's view reads the
builder's pool by position. Every row is a `Row<F>` view, the same for every family (tyr's bindings and fact rows
too): `urd::owning_view<Row<F>, C, RowIndex<F>>` holds its key `{relation, row}` (row an `index<Row<F>>`) and reads
through its context, the layer (`layer.view(key)`, `layer[key]` the stored row) or the builder (rows by position).
`value(i)` gives the i-th canonical value, `type(i)` its column's type (its pool's stored types; a builder's columns),
`get(i)` the decoded `ColumnValue`, `get<T>(i)` a `T`, and `get<T>(first, last)` / `get<T>()` ranges of `T`, each
checked against the column types (`std::invalid_argument` for another type; `T` an alternative or an object index of
any tag, resolved through the context with `urd::resolve`; urd views name their member accessor `member<&T::m>()`);
`get_values()` the stored row; a borrowed row of a pool no layer made yet has no types (`std::out_of_range`);
the columns give labels and types. A borrowed row is `urd::borrowed_view<Row<F>, C, data<Row<F>>>`
(`data<Row<F>> {relation, values}`). Inserting, finding and testing membership take a row view, a stored row of equal
widths or a span of values (`encode_value` each), handed to the pool: inserting checks count (`std::invalid_argument`) and fit
(`std::out_of_range`) before mutation; values that do not fit are in no row.
Typed indices resolve through `urd::resolve` and the context's repository.
This keeps object-view construction at the external access boundary. Row and
relation iterators borrow storage; they do not copy payloads.

Builders are movable and noncopyable. Insertion retains existing row storage;
erase moves the last row into the erased slot. Clear invalidates rows but retains
capacity. Schema replacement invalidates borrowed schema spans. Keep a borrowed
row's owner and schema alive and unchanged. A returned pool entry cannot be used
through an old view. There is no internal synchronization.

## Operations

All operations accept contextual views satisfying `RelationViewConcept<V>` (interned views or builders read through
a repository), a plan built once from their columns, and an output builder initialized with the plan's output
columns. Each replaces the output's rows.

| Operation | Plan | Meaning |
| --- | --- | --- |
| `select(input, predicate, out)` | none | Keep rows accepted by a predicate over a stored row. |
| `select_equal_columns(input, plan, out)` | `SelectEqualPlan(input, a, b)` | Compare columns of the same type. |
| `select_equal_value(input, plan, out)` | `SelectValuePlan(input, column, type, value)` | Match a canonical constant of the column's type. |
| `project(input, plan, out, workspace)` | `ProjectionPlan(input, labels)` | Keep the ordered columns and eliminate duplicate rows. |
| `join(lhs, rhs, plan, out, workspace)` | `JoinPlan(lhs, rhs)` | Natural join on common labels, followed by rhs-only columns. |
| `join(lhs, rhs, plan, cache, reuse, out, workspace)` | `JoinPlan(lhs, rhs)` | As join, reusing `JoinIndexCache` indexes of inputs marked in `JoinReuse`. |
| `union_(lhs, rhs, plan, out)` | `SetPlan(lhs, rhs)` | Union with an identical ordered typed schema. |
| `difference(lhs, rhs, plan, out)` | `SetPlan(lhs, rhs)` | Difference with an identical ordered typed schema. |
| `semijoin(lhs, rhs, plan, out, workspace)` | `JoinPlan(lhs, rhs)` | lhs ⋉ rhs: the lhs rows with a match in rhs on the common labels; the output has the lhs columns. |
| `antijoin(lhs, rhs, plan, out, workspace)` | `JoinPlan(lhs, rhs)` | lhs ▷ rhs: the lhs rows without such a match. Without common labels, lhs if rhs is empty. |
| `assign(destination, source)` | none | Replace the destination's columns and rows by the source's. |
| `builder.rename(columns)` | none | Change labels while retaining types and rows. |

Plans take the inputs' `columns()` spans and do all validation: common join labels and equality columns must have
matching types, set inputs equal columns, labels must exist (`std::out_of_range` otherwise) and outputs have unique
labels (`std::invalid_argument`). Per call an operation only clears the output and throws `std::invalid_argument`
where the output is one of its inputs (a builder input at the output's address; interned inputs never alias); debug
builds assert that the inputs' and the output's columns are the plan's. Exceptions during evaluation (for example, a
throwing predicate) may leave partial output. `assign` of a builder's own view leaves it as it is.

Plans resolve their fields (`ColumnSlice`: a column position) outside the row loop; copying a field copies its bits.
A `Workspace` holds only an output row (`row`) and a join index (`join_index`):

```cpp
const ProjectionPlan plan(view.columns(), labels);
urd::builder<Relation<Schema>> output(plan.output_columns());
Workspace workspace;
project(view, plan, output, workspace);
```

Full and incremental operations share key hashing, equality, and field copying.
Join keys mix their fields' canonical values into a hash (`urd::hash_columns`);
collisions compare the values. A join indexes the smaller input with
`UnorderedMultiMap`. `JoinIndexCache` may retain indexes for immutable inputs. Its
key is the input's `relation_identity()` (an interned relation's layer number and index, a builder's address, valid
while it is unchanged) and the ordered key positions.

## Reuse and interning

Keep a `Workspace` and output builders across evaluations. Scratch rows, row
containers, and hash tables retain capacity. `Repository::get_relation_builder<F>(columns)`
hands out builders as tyr's `get_state_builder` does: a `std::shared_ptr` from the layer's
urd slots Store of the family keyed by column widths (`urd::Store<urd::slots<builder<Relation<F>>>, urd::keyed<urd::column_widths>>`),
whose control blocks come from the Store's pool; the slot goes back with its last owner under its widths and keeps its
row storage, so it is reused only for columns of equal widths.
Builders are never interned: they are the non-interned form. The layer must outlive its builders. Prepare a schema once and pass its layout span to avoid rebuilding it in the
update loop. Larger workloads may still grow storage. Each evaluator/thread needs its own
mutable workspace; evaluators take the layer (`const Repository&`) their builders are read
through, and return their results as `BuilderRelationView`s.

`tyr::datalog::database::Repository` is a yggdrasil formalism repository layer
(`formalism::Repository`) whose symbols are the relation model: `Schema` (the column
types), `Columns` (labels and types) and `Relation<Schema>`
(`{pool, columns, rows}`, rows as sorted row ids). Rows are interned per schema, in one pool of the
layer each (row IDs are per pool). Relations with identical columns and rows
share identity regardless of insertion order. Relations of the same column types
share rows; renaming keeps the schema and the rows. Iteration follows sorted
interned row IDs, not value order.

A relation is interned as any record, from scratch records its user checks out of a `BuilderStorage` that lists
`Row<Schema>`, `Columns` and `Relation<Schema>`: each row first (`insert(data<Row<Schema>>)` gives its id in the pool
`repository.schema(types)` declares), then the columns, then the relation (`insert(data<Relation<Schema>>)` sorts its
row ids and checks its columns against its pool):

```cpp
auto relation = checkout<ygg::Relation<ygg::Schema>>(builder);
relation->pool = repository.schema(types);
auto row = checkout<ygg::Row<ygg::Schema>>(builder);
row->relation = relation->pool;
for (const auto& values : rows)
{
    row->values.set(values.begin(), values.end());
    relation->rows.push_back(repository.insert(*row).first.data().row);
}
auto columns = checkout<ygg::Columns>(builder);
columns->values.set(layouts.begin(), layouts.end());
relation->columns = repository.insert(*columns).first.index();
auto [stored, created] = repository.insert(*relation);
```

Renaming is a record with the same pool and rows and new columns; copying from another repository interns the rows
again. Repository clear invalidates all its views
while retaining reusable storage. Reset caches borrowing those rows before
clearing. A view's `relation_identity()` (an interned relation's layer number and
index, a builder's address) is what alias checks and the join cache use.

A zero-column empty relation represents false. Inserting the empty row makes it
true. Projection of a nonempty input onto no columns yields true. An empty
object domain needs no special-case representation.

## Incremental evaluation

`database/incremental/projection.hpp` and `database/incremental/join.hpp` retain
current results and maintain added/removed row sets. Deltas use the same typed
schemas and rows. Projection tracks witnesses; a change that replaces a
witness without changing membership produces no output delta. Join supports
updates to either or both operands. `database/incremental/semijoin.hpp`
maintains semijoins and antijoins: only the changed lhs rows and the lhs rows
sharing a key with a changed rhs row can change membership. Every incremental evaluator's `update` takes
one change per input: an `(added, removed)` pair read with `std::get`, such as
`Delta::change()` (a `Delta` holds the layer its builders are read through) or
`std::tie(added, removed)` of views; `incremental::RelationChange` states the requirement.
Evaluators take the layer (`const Repository&`) first and return their results as
`BuilderRelationView`s. Prepared field operations are shared with full
recomputation. Existing reset, apply, undo, and storage ownership rules remain
unchanged.

## Python

```python
from pyyggdrasil import formalism
from pytyr import database as db

columns = [
    formalism.ColumnLayout(formalism.ColumnIndex(0), formalism.ColumnType.UINT32),
    formalism.ColumnLayout(formalism.ColumnIndex(1), formalism.ColumnType.UINT32),
    formalism.ColumnLayout(formalism.ColumnIndex(2), formalism.ColumnType.FLOAT64),
]
relation = formalism.Relation(columns)
relation.insert([3, 8, 2.5])

repository = formalism.RelationRepository()
interned, created = repository.insert(relation)
assert created and tuple(interned[0]) == (3, 8, 2.5)
assert tuple(repository.view(relation)[0]) == (3, 8, 2.5)
assert relation.columns().type(2) == formalism.ColumnType.FLOAT64

plan = db.ProjectionPlan(interned.columns(), [formalism.ColumnIndex(0)])
projected = formalism.Relation(plan.output_columns())
db.project(interned, plan, projected, db.Workspace())
```

The bindings expose the library types directly. Columns are lists of `ColumnLayout(label, type)`;
`Relation(labels)` (a list of `ColumnIndex`) gives every column `UINT32`, and `Relation(columns)` also accepts another
relation's `columns()`. `ColumnType` also exposes `INT32`, `UINT64`, `INT64`, `FLOAT32`, `BOOL` and `OBJECT`. Rows
yield Python values of the column's type; no packed bytes leak through the Python
API. `RelationRepository.get_relation_builder(columns)` takes a list of `ColumnLayout` or a relation's `columns()`
(keyed by their schema),
and `RelationRepository.view(relation)` reads a builder's rows
(a `BorrowedRelation`). `RelationRepository.insert(relation)`,
`copy(view)`, and `rename(view, labels)` intern relations. Labels are `ColumnIndex`
values; `.columns()` remains a borrowed `ColumnIndex` sequence with
`type(position)` for schema inspection. Rows, views, and schemas keep their
Python owners alive; explicitly clearing an owner still invalidates them.

The operations are bound as in C++, with nothing more: the plans (`ProjectionPlan`, `JoinPlan`, `SetPlan`,
`SelectEqualPlan`, `SelectValuePlan(input, column, type, value)` with a Python value of the column's type), a
`Workspace`, and `project`, `join`, `semijoin`, `antijoin`, `union`, `difference`, `select_equal_columns`,
`select_equal_value` and `assign`, taking `(inputs..., plan, out[, workspace])`. Python is a trust boundary: the
bindings check that the inputs' and the output's columns are the plan's (`ValueError`), which C++ only asserts.

Run the native regression suite with
`ctest --test-dir <build> -R '^database_' --output-on-failure`.
It covers full and incremental operations, interned identities, canonical
encodings, schema validation, and retained allocations. Python lifetime and
mixed-type behavior are covered in `python/tests/test_database.py`.

## Tuple distances

`<tyr/datalog/lifted/database/semantics/distance.hpp>` computes directed, unweighted shortest-path
distances between tuple-valued vertices. Sources and targets have `k` columns;
an edge has `2k`, with its source tuple first and target tuple second. All four
vertex positions must have matching field types in the same order. Their column
labels may differ. The distance field is a `uint_t` column.

```cpp
using namespace ygg;
using namespace tyr::datalog::database;
const auto value = [](uint_t v) { return encode_value(v); };
const auto column = [](std::uint32_t label) { return ColumnLayout { urd::index<Column>(label), type_id<uint_t> }; };
const std::array source_columns { column(10) }, edge_columns { column(1), column(2) }, target_columns { column(20) };
urd::builder<Relation<Schema>> sources(source_columns);
urd::builder<Relation<Schema>> edges(edge_columns);
urd::builder<Relation<Schema>> targets(target_columns);
sources.insert(std::array{value(4)});
edges.insert(std::array{value(4), value(7)});
edges.insert(std::array{value(7), value(9)});
targets.insert(std::array{value(9)});

Repository repository;
DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), urd::index<Column>(3));
DistanceWorkspace workspace(plan);
urd::builder<Relation<Schema>> result(plan.output_columns());
distance(repository.view(sources), repository.view(edges), repository.view(targets), plan, result, workspace);
assert(result.contains(std::array{value(4), value(9), value(2)}));
```

The result has `2k + 1` columns: the original source tuple, the original target
tuple, and their shortest distance as `uint_t`. It uses the edge relation's
ordered column labels followed by the explicitly supplied distance label, which
must be fresh. Each reachable source/target pair occurs once. Equal endpoint
tuples have distance zero even without an edge; unreachable pairs are absent.
Only requested sources and targets appear in the result, but paths may traverse
other vertices. Empty source or target sets produce an empty result. Zero-column
tuples are supported and represent the single empty vertex.

The plan owns its schemas and checks arities/types once. Calls require each
input's original ordered schema and an output of the plan's output columns (asserted in debug builds). The
workspace retains capacity between calls and is reusable for the same vertex size. An output aliasing an input is
rejected before clearing it. A later evaluation failure can leave partial output.

Tuple IDs are private. A pool of vertices (`VertexSet`) maps a tuple's canonical
values to dense IDs and IDs back to tuples; adjacency stores only pairs of IDs.
The full evaluator rebuilds this graph and runs one breadth-first search per
source. It needs graph storage proportional to unique tuples plus edges,
and one vertex-sized distance/queue buffer. It does not materialize all-pairs
distances. The expanded output can nevertheless contain every source/target
pair, so output size can dominate memory. The maximum `uint_t` value is reserved
internally for infinity; attempting to exceed the supported vertex count throws.

### Incremental distance

Include `<tyr/datalog/lifted/database/semantics/incremental/distance.hpp>` and retain one evaluator:

```cpp
incremental::DistanceEvaluator evaluator(repository, plan);
evaluator.initialize(sources, edges, targets);
incremental::Delta source_change(repository, sources.columns());
incremental::Delta edge_change(repository, edges.columns());
incremental::Delta target_change(repository, targets.columns());
edge_change.removed.insert(std::array{encode_value(uint_t(7)), encode_value(uint_t(9))});
evaluator.update(source_change.change(), edge_change.change(), target_change.change());
assert(evaluator.get_result().empty());
assert(evaluator.get_delta().removed.contains(
    std::array{encode_value(uint_t(4)), encode_value(uint_t(9)), encode_value(uint_t(2))}));
```

The evaluator owns the compact graph, source/target membership, maintained
distances, output, and last `Delta`. Inputs are borrowed only during a
call. Pass empty changes for unchanged inputs. Deltas must describe actual set
changes, with disjoint additions/removals and each input's original schema.
Initialization replaces the baseline and clears the output delta. Swapping all
three pairs of added/removed inputs undoes a batch.

Edge updates examine every surviving source and repair its shortest-path state
where needed. They report the exact net result change after the whole batch.
A changed distance removes the old output tuple and adds the new one; replacing
a shortest-path witness without changing the distance produces no output delta.
The algorithm handles simultaneous
source, edge, and target changes, including deletions and cycles. This is exact
maintenance, not a guarantee that every update is faster than full evaluation.

#### Algorithm and reference

Edge repair specializes Ramalingam and Reps,
[*An Incremental Algorithm for a Generalization of the Shortest-Path Problem*](https://research.cs.wisc.edu/wpis/papers/jalg96.pdf),
Section 4, Figure 4, to unit edges. Mixed changes enter together: local heaps
track improving incoming edges and a global heap selects inconsistent vertices.
Equal-length replacements can cancel before descendant distances change.

For one surviving source, let `C = |δ|` count modified edges and vertices whose
final distances change. Let `D = ||δ||` also include their incident dependency
edges. Repair takes expected/amortized `O(D log(2 + D))`, for mixed and
insertion-only batches alike: the heaps are indexed binary heaps, whose push and
update take `O(log n)` (Fibonacci heaps would give insertion-only batches
`O(D + C log(2 + C))`).

Hash lookups contribute the expected qualification; heap operations contribute
the amortized qualification. These are per-source repair bounds. The complete
relation API also pays for tuple encoding, validation, adjacency changes,
growing retained arrays, source initialization, and output materialization.
Every surviving source
processes the edge batch. An empty output delta alone does not imply small work.

The implementation retains `distances` and `supports` for each active source.
`supports[v]` stores the cardinality of the paper's `SP[v]`, rather than a set
or membership bit for every edge and source. Membership is determined from an
edge's old/new contribution relative to the current distance; changes update the
count directly. At a finite consistent distance this counts shortest predecessors,
not complete paths. The source has an independent zero-length support. During
repair, `SP` also includes contributions below the tentative distance; its
infinity case follows the same comparison rule.

The heaps are urd Stores, `urd::Store<urd::queue<Entry>, urd::by_priority, urd::handles>`
(urd's addressable heap: the least entry first, `push` gives a handle that `update` and
`erase` take). One workspace owns the global heap, vertex-local heaps, and a map from
queued endpoint pairs to their local handles. These heaps are empty between repairs and
are reused across source trees, so heap storage is not duplicated per source; each keeps
its storage when cleared, and `memory_usage()` counts it with `urd::memory_usage`.
There is no process-wide heap pool or per-source edge-handle array.

Initialization and newly added sources use BFS. Removed sources release their
active slot for reuse. Target-only changes read retained distances without graph
search. No adaptive fallback from expensive repair to fresh BFS is used.

Tuple IDs are retained until `initialize` starts a new baseline. Removed sources'
state buffers are reused for later sources. The distance/support state can occupy
`O(S_peak * V_seen)`, using the peak simultaneous source count and historical
vertex count. Reinitialization resets membership while retaining reusable
capacities. Adjacency, tuple bytes, shared heap nodes and handles, output, and
output deltas add to this footprint. New tuples can grow storage even when live
cardinality is fixed. `memory_usage()` includes retained pool chunks even after
heaps become empty; it excludes the plan and caller-owned inputs. Warm
finite-universe workloads can reuse allocations, but unbounded churn is not
allocation-free.

Results and deltas are borrowed until the next mutation. Inputs must not alias
the evaluator's owned buffers, and calls must be sequential and nonreentrant.
Schema, overlap, aliasing, and membership rejections (adding a present or
removing an absent row) precede mutation and preserve the previous evaluation.
An exception during mutation requires reinitialization; result/delta access and further updates then
throw `std::logic_error` instead of exposing partial state.

The `database_distance` tests compare full and incremental evaluation with an
independent pairwise BFS, including mixed tuple types, nullary vertices, joint
changes, and undo. The native distance target enables internal work counters for
locality regressions; they are absent from production builds and the public API.
`database_allocations` measures warmed full evaluation, finite-universe source
churn, and edge repair/undo. With `YGGDRASIL_BUILD_PROFILING` enabled,
`database_distance_benchmark` compares full evaluation with incremental updates
for bounded graph sizes and tuple arities 1, 2, 4, and 8. It reports retained
bytes and output rows; timings are measurements, not CI performance thresholds.
Update benchmark iterations measure one forward edge-change batch and its undo,
and report `updates_per_iteration = 2`; divide their displayed time by two for
the mean per-update time. Cases include shortcut replacements and harmless
self-loop changes that leave every distance unchanged. Witness-tail and dense-tie
cases check scaling when updates cancel or most neighboring distances stay fixed. The `initialize_warm`
family measures one baseline initialization per iteration after capacity warmup.
Prepared snapshots and correctness checks are outside the timed loops.

### Minimum distance

When only the smallest distance between any source and any target is needed, use the
aggregates over the same plans. They reuse `distance` and `incremental::DistanceEvaluator`
unchanged and read the distance column:

```cpp
urd::builder<Relation<Schema>> result(plan.output_columns());
const uint_t full = min_distance(sources, edges, targets, plan, result, workspace);

incremental::SourceMinDistance minimum(repository, plan);
minimum.initialize(sources, edges, targets);
minimum.update(source_delta.change(), edge_delta.change(), target_delta.change());
const uint_t current = minimum.get_result();
```

Both return `uint_t`'s maximum when no target is reachable, and 0 when a source equals a target.
The incremental form counts result rows per distance value, so deleting the current minimum
reveals the next one. Memory follows the wrapped evaluator, plus one counter per distance value.

### Must-pass objects

`MustPass` (`semantics/must_pass.hpp`) finds, per key, the probe objects that every path from the key's
sources to its targets passes in some vertex: the landmarks of the key's graph (Hoffmann, Porteous &
Sebastia, JAIR 2004). Sources, edges and targets start with the same key columns; a vertex has k columns;
probes have one column, and a vertex contains an object if one of its columns of the probes' type holds it:

```cpp
// sources (key…, vertex), edges (key…, from, to), targets (key…, vertex), probes (object)
MustPassPlan<> plan(sources.columns(), edge_query.columns(), targets.columns(), probes.columns(), keys);
MustPass<> must_pass(repository, edge_query, plan);
urd::builder<Relation<Schema>> out(plan.output_columns());   // (key…, object)
must_pass.must_pass(input, sources, targets, probes, out);
```

Like `MinDistance`, the edge query is evaluated on demand with the key and a vertex bound, so each key's
graph is explored from its sources and never materialized. A* finds a shortest witness path; only probe
objects on it can be must-pass. Objects in every source or every target are trivially must-pass (initial facts and
goals are landmarks); every other object on the witness is verified as in landmark detection (Richter,
Helmert & Westphal, AAAI 2008) by testing whether the targets stay reachable without the vertices
containing it. Each search is bidirectional: a backward breadth-first search from the targets alternates
with the forward side by the vertices each expanded, and its exhaustion proves unreachability from the
smaller side. An optional heuristic (`MustPass<H>`, given the key followed by a vertex) guides the
forward side; with `exclude(key, object)` it is told which object a verification avoids. A key whose targets are unreachable has none. An edge
query that projects vertices onto fewer columns is an abstraction: objects that must be passed in the
projection must be passed by every original path too.

### Distance from Python

Column labels are `ColumnIndex` values. Python evaluators and `distance` take
`BorrowedRelation` inputs, or interned `RelationView` inputs when every input of
the call is interned. Read a `Relation` through a repository with `repository.view(relation)`;
to mix them, copy a view with `db.assign(formalism.Relation(), view)`. For vertices
consisting of two integers:

```python
from pyyggdrasil import formalism
from pytyr import database as db

def columns(*labels):
    return [formalism.ColumnIndex(label) for label in labels]

sources = formalism.Relation(columns(0, 1))
sources.insert([10, 20])
edges = formalism.Relation(columns(0, 1, 2, 3))
edges.insert([10, 20, 10, 21])
targets = formalism.Relation(columns(2, 3))
targets.insert([10, 21])
plan = db.DistancePlan(sources.columns(), edges.columns(), targets.columns(), formalism.ColumnIndex(4))

repository = formalism.RelationRepository()
inputs = (repository.view(sources), repository.view(edges), repository.view(targets))
result = formalism.Relation(plan.output_columns())
db.distance(*inputs, plan, result, db.DistanceWorkspace(plan))
assert tuple(repository.view(result)[0]) == (10, 20, 10, 21, 1)

evaluator = db.DistanceEvaluator(repository, plan)
evaluator.initialize(*inputs)
assert tuple(evaluator.get_result()[0]) == (10, 20, 10, 21, 1)
assert evaluator.get_delta().added.empty()
```

`distance(sources, edges, targets, plan, out, workspace)` replaces the rows of `out`, a `Relation` of the plan's
output columns. Incremental `get_result()`
returns a read-only `BorrowedRelation` (the library's `BuilderRelationView`), and
`get_delta()` returns a `RelationDelta` whose `added` and `removed` relations are
borrowed the same way. They and their borrowed rows keep the evaluator alive; rows
must not be retained across an evaluator `initialize` or `update`;
`db.assign(formalism.Relation(), result)` copies a result. Like the C++ interface, `update(sources, edges, targets)` takes
one `(added, removed)` tuple per input.

## Query graphs and optimization

`<tyr/datalog/lifted/database/syntax/query_repository.hpp>` adds an interned relational query graph without
capturing input rows. A `QueryRepository` only stores and interns query
records; each repository has its own identity (a process-wide number). Queries are built like other interned
data: check out a `urd::data<Query<Tag>>` from a `QueryBuilder`, fill
its operands and parameters, and intern it with `insert_query(repository,
builder, data)` from `query_construction.hpp`. That inserts the concrete record
and its heterogeneous root. `prepare_for_insert` validates operands, schemas,
constants, and execution orders and derives the prepared metadata. Query handles
borrow the repository. Input-slot numbers identify bindings at evaluation time;
repeated references to a slot use the same input relation.

The data, view, and repository definitions are separated into
`query_data.hpp`, `query_view.hpp`, and `query_repository.hpp`; the index is the generic `urd::index<T>`.
`Query<Tag>` identifies a concrete operation; `Query<>` is the
heterogeneous root whose data stores a variant of concrete query indices. Each
concrete data type stores only its own operands and parameters. For example,
`QueryJoinTag` has `lhs` and `rhs` query indices, while `QuerySelectValueTag` has
`arg`, `column`, and an encoded `constant`. Derived output columns are cached
but do not participate in operator identity. Input and empty schemas, and the
ordered labels requested by projection or rename, do participate in identity.

`insert_query` returns `QueryView`. Its `get_variant()` exposes
concrete views with operation-specific getters such as `get_lhs()`, `get_arg()`,
and `get_constant()`. All query views expose `columns()`; every view borrows its
repository. `size()` counts heterogeneous root queries, not the separate
concrete operator records.

Queries use the same typed representation during optimization and evaluation.
Join, projection, distance, and Generic Join records retain their prepared
execution plans. Selection records retain resolved column positions. These
caches are derived during validated construction and excluded from interning
identity. Their output schemas are read directly from the prepared plans.
Generic Join is a query constructor with input queries, a variable order, and
an independent output order. An empty output order is canonicalized to the
natural union of the input columns.

`compile(roots)` produces a copyable, read-only `QueryPlan`
that owns a const query repository. Only reachable queries are copied, with
children preceding their parents; shared queries and repeated roots stay shared.
The plan preserves root order and ordered, typed output schemas and remains valid
after its source repository is destroyed. `node_count()`, `root_count()`, and
`roots()` expose its shape; indexing by a root query index returns an ordinary
`QueryView`. Row enumeration order is not a query guarantee.

Include `<tyr/datalog/lifted/database/syntax/formatter.hpp>` to format a plan with
`fmt::format("{}", plan)`, `ygg::to_string(plan)`, or `explain(plan)`.
Python's `plan.explain()` uses the same rendering. The `fmt::formatter`
specialization follows `YGG_ENABLE_FMT_FORMATTERS`; `explain()` remains available
when public formatters are disabled.

`<tyr/datalog/lifted/database/optimization/optimization.hpp>` plans the roots from what is known about the data:

```cpp
using namespace tyr::datalog::database;
QueryRepository repository;
QueryBuilder<> builder;
const auto input = [&](size_t slot, const auto& relation)
{
    auto data = checkout<Query<DefaultColumnTypes, QueryInputTag>>(builder);
    data->input_slot = slot;
    data->columns.set(relation.columns().begin(), relation.columns().end());
    return insert_query(repository, builder, *data);
};
auto join = checkout<Query<DefaultColumnTypes, QueryJoinTag>>(builder);
join->lhs = input(0, left_relation).index();
join->rhs = input(1, right_relation).index();
const auto query = insert_query(repository, builder, *join);

// Any range of relation views: interned RelationView or BuilderRelationView over builders.
const std::array bindings { left_view, right_view };
const auto structural = optimize(query);                                   // structure only
const auto measured = optimize(query, collect_statistics<DefaultColumnTypes>(bindings));
QueryEvaluator<> evaluator(repository, measured);
evaluator.evaluate(bindings);
const auto result = evaluator.get_result();  // a BuilderRelationView
```

`optimize(roots, statistics = {})` returns a `QueryPlan` with every root's ordered
schema. It implements established algorithms as published:

1. **Normalization.** The textbook algebraic rewrites [GMUW08]: selections are
   pushed down through joins, projections, renames, unions, and differences, and
   into the lhs of semijoins and antijoins, projections through renames and
   unions, and empty relations are propagated.
2. **Join blocks.** Each maximal tree of joins is planned as a whole. Unions,
   differences, semijoins, antijoins, distances, and selections spanning several
   join operands bound the blocks.
3. **Planning each block.** A block whose operands all have known cardinalities
   (measured inputs, observed queries, or queries over them) is planned
   cost-based, any other block from structure:
   - *Cost-based:* DPccp [MN06] finds the bushy join order of least C_out
     [CM95, Leis15] under System R estimates [Selinger79], with the textbook
     estimates of [GMUW08] for projection, union, and difference, T(R ⋉ S) =
     min(T(R), T(π_R(R ⋈ S))) and T(R ▷ S) = T(R) − T(R ⋉ S), and observed
     cardinalities replacing estimates [LEO01]; disconnected components are
     joined last by cross products. The join tree is then refined by Algorithm 4
     of [Freitag20]: growing joins and their ancestors become one
     worst-case-optimal join.
   - *Structure:* if the GYO reduction [Graham79, YO79] shows the block acyclic,
     Yannakakis' algorithm [Yannakakis81]; otherwise Generic Join [NRR13], whose
     running time is bounded by the AGM bound [AGM08].

   Generic Join's variable order is fixed by column label: any order attains the
   worst-case bound, but [Freitag20] additionally optimizes it with the Tributary
   Join cost model, which is not implemented. Distances have no estimate in the
   literature and are only known when observed.
4. **Compilation** of the planned roots into an owning `QueryPlan`.

Union and difference bound join blocks, so joins are not distributed over unions,
and blocks of different roots are planned independently (shared subplans are
still interned once). Planning for incremental maintenance is not modeled.

References:
[Selinger79] P. G. Selinger et al., *Access Path Selection in a Relational
Database Management System*, SIGMOD 1979.
[GMUW08] H. Garcia-Molina, J. D. Ullman, J. Widom, *Database Systems: The
Complete Book*, 2nd ed., 2008, ch. 16.
[Graham79] M. H. Graham, *On the Universal Relation*, University of Toronto, 1979.
[YO79] C. T. Yu, M. Z. Özsoyoğlu, *An Algorithm for Tree-Query Membership of a
Distributed Query*, COMPSAC 1979.
[Yannakakis81] M. Yannakakis, *Algorithms for Acyclic Database Schemes*, VLDB 1981.
[AGM08] A. Atserias, M. Grohe, D. Marx, *Size Bounds and Query Plans for
Relational Joins*, FOCS 2008.
[NRR13] H. Q. Ngo, C. Ré, A. Rudra, *Skew Strikes Back: New Developments in the
Theory of Join Algorithms*, SIGMOD Record 2013.
[MN06] G. Moerkotte, T. Neumann, *Analysis of Two Existing and One New Dynamic
Programming Algorithm for the Generation of Optimal Bushy Join Trees without
Cross Products*, VLDB 2006; corrected exclusion set from PVLDB 11 (2018), p. 1069.
[CM95] S. Cluet, G. Moerkotte, *On the Complexity of Generating Optimal Left-Deep
Processing Trees with Cross Products*, ICDT 1995.
[Leis15] V. Leis et al., *How Good Are Query Optimizers, Really?*, PVLDB 9(3), 2015.
[Freitag20] M. Freitag, M. Bandle, T. Schmidt, A. Kemper, T. Neumann, *Adopting
Worst-Case Optimal Joins in Relational Database Systems*, PVLDB 13(11), 2020.
[LEO01] M. Stillger, G. Lohman, V. Markl, M. Kandil, *LEO – DB2's LEarning
Optimizer*, VLDB 2001.

`prepare_for_insert` checks schemas, constants, and execution orders before
publishing immutable query data. Query records hold plain indices, so callers
keep operands in the repository they insert into. The roots passed to `optimize`
belong to one repository.

### Relaxation

`relax(roots, known, repository, builder)` (`syntax/translations/relaxation.hpp`) over-approximates queries whose
inputs not marked `known(slot)` may hold any rows. Each result Q̂ reads only the known inputs, which are renumbered once
for all roots into `Relaxation::slots`, and has a subset S' of the query's labels. For all inputs, Q ⊆ Q̂ × U^(labels
not in S'), and `nullopt` means nothing is constrained.

Relaxing π_L(Q) needs only the labels L. Labels no one needs are eliminated as early as possible:
- a conjunct that shares nothing needed becomes a nullary existence test;
- antijoins and differences are kept only as exact filters on needed labels.

This is the projection of pattern databases. The header has the full translation table and references. Evaluate the
results like any query, e.g. after `optimize()`.

### Statistics

`Statistics` describes what is known; everything is optional and nothing
known means an unbounded domain:

- `objects`: the domain size. It caps distinct counts; without it the domain is
  unbounded.
- `inputs`: per input slot, a `RelationStatistics` with `rows` and distinct counts
  keyed by `Index<Column>`. A missing distinct count is the row count, capped by
  `objects`.
- `expressions`: observed results of queries in the roots' repository, keyed by
  `query.index()`. They replace the derived estimate of the equivalent planned
  query [LEO01].

Python keys distinct counts by plain column labels and expressions by
`QueryIndex`. `collect_statistics(bindings)` scans relations to fill `inputs`.
Estimates assume independent, uniform columns and containment of join columns
[Selinger79]; conjunctions are normalized so join association never changes the
estimate, and projection starts a new scope for its hidden columns. Statistics
guide planning only; they never justify replacing an input by a constant or an
empty relation.

### Evaluators

Evaluators differ along two independent axes: what they return, the whole result (*full*, the default) or answers to
lookups with some output columns given (*bound*); and whether they follow input changes (*incremental*) or evaluate
the current inputs only (*one-shot*, the default). A namespace exists only for a non-default mode, its directory under
`<tyr/datalog/lifted/database/semantics/>` has the same name, and the class is always `QueryEvaluator`:

| | one-shot | incremental |
|---|---|---|
| full | `QueryEvaluator` (`query_evaluation.hpp`) | `incremental::QueryEvaluator` (`incremental/query.hpp`) |
| bound | `bound::QueryEvaluator` (`bound/query_evaluation.hpp`) | `incremental::bound::QueryEvaluator` (`incremental/bound/query_evaluation.hpp`) |

Use full evaluation for a whole result, bound evaluation for many selective lookups against the same inputs (the edge
oracle of the distance search), and an incremental evaluator when the inputs change between uses. Bound evaluation
compiles the query into union normal form (`bound/union_normal_form.hpp`) and plans each branch per set of bound
columns (`bound/conjunctive_plan.hpp`). Every incremental evaluator takes its first inputs with `initialize` and exact
changes with `update`; the bound ones also offer `bind`, which diffs the inputs when the changes are unknown, and
`seed`, which reports the answers a change can add. The operator plans of the query records (`JoinPlan`, `ProjectionPlan`,
`GenericJoinPlan`, `DistancePlan`) are shared by all evaluators and stay in `tyr::datalog::database`.

`<tyr/datalog/lifted/database/semantics/query_evaluation.hpp>` provides `QueryEvaluator`.
`evaluate(bindings)` computes all roots, reusing retained output and scratch
storage. `<tyr/datalog/lifted/database/semantics/incremental/query.hpp>` provides
`incremental::QueryEvaluator`. Initialize it once with input bindings,
then call `update(changes)` with one `(added, removed)` pair of relation views per
input slot. Each pair contains actual set additions and removals in that input's ordered typed schema.
An empty change uses an empty relation with the correct schema. Swap additions
and removals to undo a transition.

`get_result(root_index)` borrows a root's current relation;
`get_delta(root_index)` borrows exact net added/removed rows after an incremental
update. Initialization clears output deltas. A subsequent evaluation or update
can invalidate borrowed rows. Input adapters and their underlying storage need
only remain alive during the call. Results belong to the evaluator, and may not
be fed back as aliased inputs to that same evaluator. `memory_usage()` reports
retained evaluator storage; it is distinct from the optimizer's estimate.

#### Distance search

`MinDistance<H>` (`min_distance.hpp`) and `incremental::MinDistance<H>`
(`incremental/min_distance.hpp`) compute the exact minimum distance from any source to any
target over edges given by a query over input slots (or by one edge relation), and return
`uint_t`'s maximum when no target is reachable. Edges are never materialized: a bound
evaluator answers successor, predecessor and edge-check lookups, and A* (a binary heap
ordered by f, then g) runs against a backward breadth-first frontier.

- `MinDistance::min_distance(input, sources, targets, heuristic)` binds the inputs for one
  call; only compiled plans and static slots are kept.
- `incremental::MinDistance` keeps the search tree across calls. `initialize(input)` binds a
  new baseline; `bind(input, maybe_changed)` diffs the inputs against it, and
  `update(change, changed)` takes exact `(added, removed)` changes per slot. A change that
  throws makes the next search start afresh.
  Its `set_targets(targets)` sets the targets, and different targets discard the stored
  search; `min_distance(sources, heuristic)` then searches over the current inputs.

The heuristic `H` (`search/concepts.hpp`, `DistanceHeuristic`) must be consistent in every
graph the inputs can form while the targets stay unchanged: it may depend on the targets and
on static inputs only. `BlindDistanceHeuristic` is the zero estimate.

### Python query API

```python
from pyyggdrasil import formalism
from pytyr import database as db

x, y, z = formalism.ColumnIndex(0), formalism.ColumnIndex(1), formalism.ColumnIndex(2)
left = formalism.Relation([x, y])
left.insert((1, 2))
right = formalism.Relation([y, z])
right.insert((2, 3))

repository = db.QueryRepository()

def insert(data):
    concrete, _ = repository.insert(data)  # e.g. a QueryJoin view
    return repository.insert(db.QueryData(concrete))[0]  # the type-erased Query

a = insert(db.QueryInputData(0, left.columns()))
b = insert(db.QueryInputData(1, right.columns()))
query = insert(db.QueryProjectData(insert(db.QueryJoinData(a, b)), [z, x]))
relations = formalism.RelationRepository()
inputs = [relations.view(left), relations.view(right)]
optimized = db.optimize([query], db.collect_statistics(inputs))
evaluator = db.QueryEvaluator(relations, optimized)
evaluator.evaluate(inputs)
assert {tuple(row) for row in evaluator.get_result()} == {(3, 1)}
independent = db.assign(formalism.Relation(), evaluator.get_result())
print(optimized.explain())
```

Like the C++ library, every operator has an index (`QueryJoinIndex`), a record
(`QueryJoinData`), and a view (`QueryJoin`) with its getters; `repository.insert(data)`
returns `(view, created)`. `QueryData(view)` makes the type-erased `Query` that
operators take as operands. The records are `QueryInputData(slot, columns)`,
`QueryEmptyData(columns)`, `QueryJoinData`/`QueryUnionData`/`QueryDifferenceData`/`QuerySemijoinData`/`QueryAntijoinData(lhs, rhs)`,
`QueryProjectData`/`QueryRenameData(arg, labels)`, `QuerySelectEqualData(arg, lhs, rhs)`,
`QuerySelectValueData(arg, column, type, value)`, `QueryDistanceData(sources, edges,
targets, distance_column)`, and `QueryGenericJoinData(inputs, variable_order,
output_order=[])`. A select constant must have the selected column's type.
Query repositories are made directly (`QueryRepository()`); each has its own identity,
so queries compare across them. `compile(roots)` and
`optimize(roots, statistics=Statistics())` take a list of roots and return a `QueryPlan`.

`IncrementalQueryEvaluator(repository, plan)` exposes `initialize(inputs)` and
`update(changes)` with one `(added, removed)` pair per input slot. Its result and
`RelationDelta` relations are read-only `BorrowedRelation` views that keep the
evaluator alive; `db.assign(formalism.Relation(), relation)` copies one. Query
handles keep their repository alive; compiled plans and evaluators are independent
of it. For sparse slot numbering, an unused slot takes an empty `Relation()`.
Dictionary-valued statistics fields are converted by value; construct them with
keyword dictionaries or replace the field to change entries.

### Query benchmarks

`database_query_benchmark` separates optimizer time from execution. It compares
full and incremental evaluation of the original plan and the plans from structure
and from statistics, validates
forward/undo results and net deltas before timing, and reports retained bytes.
The cold/warm Generic Join cases include full trie construction; warm means
reusing the evaluator's allocated workspace, not skipping input indexing. The
initial triangle fixture is a reproducible correctness and performance smoke
workload, not a broad claim about optimizer quality.
