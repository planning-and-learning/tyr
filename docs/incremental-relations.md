# Incremental relations

The C++ interfaces in `tyr/datalog/lifted/database/incremental/` maintain projection and
natural join, including joins where both inputs change. Full `project` and `join`
use the same typed schemas and packed rows. All results use the
existing `urd::builder<Relation<Schema>>` and contextual `View` interfaces; publishing them
in a repository is optional.

## Projection

```cpp
using namespace ygg;
using namespace tyr::datalog::database;

Repository repository;
const std::array columns { ColumnLayout { urd::index<Column>(0), type_id<uint_t> }, ColumnLayout { urd::index<Column>(1), type_id<uint_t> } };
const auto input = repository.get_relation_builder<Schema>(columns);
input->insert(std::array { encode_value(uint_t(7)), encode_value(uint_t(10)) });
input->insert(std::array { encode_value(uint_t(7)), encode_value(uint_t(11)) });

Workspace workspace;
const std::array kept { urd::index<Column>(0) };
incremental::ProjectionEvaluator projection(repository, ProjectionPlan(input->columns(), kept));
projection.initialize(repository.view(*input), workspace);

incremental::Delta change(repository, input->columns());
change.removed.insert(std::array { encode_value(uint_t(7)), encode_value(uint_t(10)) });
projection.update(change.change(), workspace);
// (7) still has one witness, so get_delta() is empty and get_result() contains it.

const auto [added, removed] = change.change();
projection.update(std::pair { removed, added }, workspace); // undo
```

Each evaluator owns its maintained result, last output delta, and persistent
bookkeeping. A caller supplies reusable `Workspace<Values>` scratch for each call;
multiple evaluators can share it sequentially. Projection stores a support count
for each distinct result row. Updates report the net change of the entire batch,
including cancellation when a removed last witness is replaced by another.

A tuple whose support count temporarily reaches zero remains stored until all
additions have been processed. Replacing its witnesses can therefore leave the
output row in place. A reusable vector records the pending row positions;
remaining zero-support rows are erased in descending position order so compact
erasure does not invalidate the positions still to process. The final output
delta contains only actual membership changes.

New output rows may coexist temporarily with rows pending removal. Retained
capacity can therefore reflect the old output plus newly added output rows,
including support-vector growth, even when the final cardinality is unchanged.
Witness-only replacement avoids this growth and avoids result erasure entirely.

## Natural join

Include `incremental/join.hpp` and construct `JoinEvaluator(layer, plan)`. The
interface accepts relation views (a builder is read as `layer.view(builder)`):

```cpp
incremental::JoinEvaluator joined(repository, JoinPlan(lhs.columns(), rhs.columns()));
joined.initialize(lhs, rhs, workspace);
joined.update(lhs_change.added, lhs_change.removed,
              rhs_change.added, rhs_change.removed, workspace);

// Undo the complete batch on both inputs.
joined.update(lhs_change.removed, lhs_change.added,
              rhs_change.removed, rhs_change.added, workspace);
```

The evaluator owns the current left and right relations and a mutable index for
each. Initialization copies the supplied rows; later updates maintain these
copies from the deltas alone. Pass empty changes for an unchanged input. No
input views need to stay alive between calls, and no repository is needed.
For example, when only the left input changes:

```cpp
urd::builder<Relation<Schema>> unchanged(rhs.columns());
joined.update(lhs_change.added, lhs_change.removed, unchanged, unchanged, workspace);
```

Full natural-join tuples uniquely identify their pair of set-valued input rows,
so join needs no projection-style witness counters. Output columns follow the
original left/right ordering in `JoinPlan`.

A batch first removes left rows against the old right input, then removes right
rows against the remaining left input. It next adds left rows against the
remaining right input, then adds right rows against the new left input. This
counts pairs affected on both sides once and avoids temporary matches between
added and removed rows. The output delta is the exact net set change, including
for Cartesian products and zero-column relations.

The mutable indexes reuse the existing join probing and actual-key comparisons.
They use the same `UnorderedMultiMap` as full-join operations. Its flat hash table
points into contiguous value slots; erased values are destroyed and their slots
are linked into a free list for later insertions. Only affected entries are
changed; indexes are not rebuilt per batch. Erasing or updating a row position
searches its hash group, so large groups or hash collisions increase that work.
Removing the last value for a key does not grow storage. Subsequent insertions
may grow the key table to obtain tombstone headroom, even when the live key count
stays constant; with enough headroom, GTL cleans tombstones in place.
Reusable capacities grow as needed;
allocation-free operation is a warmed-capacity property, not an unconditional
promise for arbitrarily growing relations.

## Delta and lifetime contract

- `Delta` owns reusable `added` and `removed` relation builders with identical
  ordered schemas, read through the layer it holds (`change()` gives their views).
  `clear()` retains their buffers.
- Input deltas must be actual set changes: added rows were absent, removed rows
  were present, and the sets are disjoint. Overlap errors are checked before mutation; schemas against the plan are
  asserted in debug builds. Source membership is a caller
  precondition for projection. Join retains both inputs for evaluation and rejects
  absent removals or already-present additions while applying them. Detected invalid changes raise `std::invalid_argument`.
- Inputs are borrowed during the call only. They cannot alias the evaluator's
  result, delta, or the workspace buffers. This nonaliasing requirement is a caller
  precondition beyond the `relation_identity()` check against the evaluator's own builders. Calls must be sequential and
  nonreentrant.
- `initialize` produces the baseline and an empty delta. Reinitialization keeps
  reusable capacities. `update` before initialization throws `std::logic_error`.
- `get_result()` returns a `BuilderRelationView` and `get_delta()` a const reference. Borrowed contents
  must not be retained across the next evaluator mutation. Persist input changes
  separately if they will be needed for undo; reversing a batch exchanges its
  added/removed inputs. Feed an upstream output delta to downstream operators
  before updating the upstream operator again.
- An exception after mutation starts can leave a partial result/delta. Getters
  still expose those buffers, but they are not a complete evaluation. Call
  `initialize` before consuming them or updating that evaluator again. Validation failures detected
  before mutation preserve the prior result and readiness.
- `memory_usage()` reports retained working buffers, including indexes, projection
  counts, and the join's owned input relations. Multimap indexes include retained
  value slots (including free slots) and hash-table capacity.
  It excludes the prepared immutable plan and caller-owned workspace. Container
  accounting estimates dynamic payload/control storage, not process RSS.

## Compact erasure

Nonconcurrent `RawArrayPool`, `RawArraySet`, and relation builders now expose
`erase(index)`. It replaces that row with the previous last row and removes the
last slot. The removed and moved row positions/spans are invalidated; other
positions stay unchanged. Parallel metadata must perform the same swap-and-pop.
Array-set lookup is repaired before returning. Builder erasure clears its old
canonical index only after success, and `find(row)` returns the current position.

Deletion rewinds existing allocation cursors, retaining allocated segments. The
ordinary insertion path is unchanged. No free list or per-insert deletion check
is introduced. Erasing can reserve additional hash capacity before mutation;
repeated bounded-cardinality churn can then reuse that storage. GTL may still
perform a table-wide tombstone cleanup: update cost is amortized, not a guarantee
of strict per-delta latency. Cached indexes must already be invalidated before
any mutation of their source, including erasure.

## Profiling and checks

`YGGDRASIL_BUILD_PROFILING` enables the `database_incremental_benchmark` executable. It
compares full evaluation using retained output/workspace with incremental
maintenance for projection and joins with one or both inputs changing, including
forward, reverse, and sibling transitions. Projection cases cover both changed
projected keys and witness replacement with unchanged output membership. Prepared
input snapshots and deltas are outside timed sections. Dedicated latency cases expose
long-running churn; the append-only case supports comparison against a
pre-change checkout. No timing threshold is enforced by the unit tests.

The `database_incremental` unit test target compares complete results and exact
output deltas after every update against the existing full operations. The
allocation tests cover warmed bounded-cardinality updates with new tuple values
and two changing join inputs with new key groups and row compaction.
The existing tracker measures C++ allocation/deallocation functions; direct
`malloc/free` calls, including Cista's column/plan storage, are not intercepted.

## Algorithmic references

The correspondence with the literature is at the operator level: support counting
for set projection and first-order delta rules for binary natural join. The
containers, mutable indexes, and buffer reuse are local implementation choices.

1. **Ashish Gupta, Inderpal Singh Mumick, and V. S. Subrahmanian.**
   [Maintaining Views Incrementally](https://www.cs.columbia.edu/~gravano/Qual/Papers/13%20-%20Maintaining%20Views%20Incrementally.pdf).
   SIGMOD 1993, pp. 157-166.
   [DOI](https://doi.org/10.1145/170035.170066).
   Sections 4-5 describe delta rules and maintenance through derivation counts.
   `ProjectionEvaluator` applies the counting technique to projected input tuples:
   a result exists while its support count is positive, and only net membership
   changes propagate. `JoinEvaluator` specializes the join delta rules into
   separate removal and addition passes, counting pairs affected on both sides
   exactly once. The paper's recursive Delete and Rederive algorithm is outside
   the scope of these evaluators.
2. **Mihai Budiu, Tej Chajed, Frank McSherry, Leonid Ryzhyk, and Val Tannen.**
   [DBSP: Automatic Incremental View Maintenance for Rich Query Languages](https://www.vldb.org/pvldb/vol16/p1601-budiu.pdf).
   PVLDB 16(7), pp. 1601-1614, 2023.
   [DOI](https://doi.org/10.14778/3587136.3587137).
   Theorem 3.4 gives the bilinear delta identity underlying our join update;
   Proposition 4.7 describes the count-threshold behavior used for incremental
   duplicate elimination. This is a modern algebraic reference for the operators;
   the current implementation has explicit added/removed sets and per-operator
   state, without DBSP's general stream-circuit transformation framework.

## Further reading

These papers cover extensions and broader theory beyond the current evaluators.

- **Yanif Ahmad, Oliver Kennedy, Christoph Koch, and Milos Nikolic.**
  [DBToaster: Higher-order Delta Processing for Dynamic, Frequently Fresh Views](https://www.vldb.org/pvldb/vol5/p968_yanifahmad_vldb2012.pdf).
  PVLDB 5(10), pp. 968-979, 2012.
  Materializes auxiliary views of delta queries and maintains them with further
  deltas. Relevant when maintaining a complete query repeatedly performs expensive
  joins that auxiliary state could avoid.
- **Ahmet Kara, Milos Nikolic, Dan Olteanu, and Haozhe Zhang.**
  [F-IVM: Analytics over Relational Databases under Updates](https://doi.org/10.1007/s00778-023-00817-w).
  The VLDB Journal 33, pp. 903-929, 2024.
  Combines higher-order maintenance, factorized computation, and ring-valued
  payloads. Relevant when large intermediate joins dominate time or memory.
- **Frank McSherry, Derek G. Murray, Rebecca Isaacs, and Michael Isard.**
  [Differential Dataflow](https://www.cidrdb.org/cidr2013/Papers/CIDR13_Paper111.pdf).
  CIDR 2013.
  Extends incremental dataflow to nested iteration using differences across
  partially ordered versions. Relevant background for recursive feature
  evaluation; adopting this execution model would be a separate design decision.
- **Dan Olteanu.**
  [Recent Increments in Incremental View Maintenance](https://arxiv.org/abs/2404.17679).
  2024, arXiv:2404.17679, accompanying the Gems of PODS 2024 talk.
  Surveys update-time, preprocessing, and enumeration-delay tradeoffs and
  optimality for conjunctive queries. Useful for distinguishing storage overhead
  from limitations of a query's maintenance strategy.
