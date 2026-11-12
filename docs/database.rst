Relational algebra
==================

``<tyr/datalog/lifted/database/semantics/operations.hpp>`` provides header-only set operations in
``tyr::datalog::database``. Each takes its inputs, a plan built from their columns, an output builder and, where it
needs scratch storage, a ``Workspace``. Relations have runtime columns and rows packed by their widths (urd's rows of
unsigned columns, ``urd::by_column_widths``); the relation model (columns, schemas, row sets, relations) is yggdrasil
formalism's. Columns of different types need no tag or variant per cell. Nullary relations and runtime arities are
supported.

Types and schemas
-----------------

A column holds one alternative of ``ygg::ColumnValue`` (``<yggdrasil/formalism/values.hpp>``): ``uint32_t``,
``uint64_t``, ``int32_t``, ``int64_t``, ``float``, ``double``, ``bool`` or ``urd::index<formalism::Object<void>>``.
Its ``ygg::TypeId`` is the alternative's index (``type_id<T>``; an object index of any tag has the object
alternative's), so relations, schemas, plans and repositories carry no list of value types.

A relation's column types live with its rows: the layer's pool of its key keeps them (``relations().types(key)``,
written when the pool is made and checked on every later use). The layer interns the ``Columns`` record from the
caller's labels and the pool's types; insert, copy and rename check the caller's types against the pool's
(``std::invalid_argument``).

A row stores each value as an unsigned ``ygg::Value`` (``encode_value``, ``decode_value``; ``encode`` and
``decode(bits, type)`` for a ``ColumnValue``): unsigned integers as they are, signed ones as two's complement in their
own width, floats as their bits with signed zero and all NaNs normalized, bools as 0 or 1, and indices as ``uint_t``
(``index.value``; the code that owns a relation knows a column's tag and converts back with ``Index<Tag>(value)``).
Equal values have equal bits, so rows hash and compare by value, without tolerance rounding. This is an in-process
representation, not a portable wire format.

.. code:: cpp

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

``Column`` identifies a schema-local label. Columns are a ``std::span<const ColumnLayout>`` of labels and ``TypeId``\ s;
interned ``Columns`` records identify by the ordered labels and types. Where a column lies in a row, the builder's
widths say (``.widths()``, an ``urd::column_widths``: each column whole, a bool in one bit, an object in 32 bits, or
in the layer's object bits for a layer's builders and pools). Find a label's position with
``std::ranges::find(columns, label, &ColumnLayout::label)``. Labels must be unique and types valid:
``builder<Relation<F>>`` validates them in its constructor, ``initialize`` and ``rename``, and plans validate the
columns they are built from (``ygg::validate_columns``).

Rows and ownership
------------------

The relation code is data (``formalism/relation_data.hpp``), builders (``formalism/relation_builder.hpp``), views
(``formalism/relation_view.hpp``) and the value encoding (``formalism/values.hpp``).

**Families.** A relation belongs to a family ``F`` of the layer's relation types, whose pools (one per key
``index<F>``) hold its rows. Free relations are the ``Schema`` family, keyed by their column types. The database's
``Repository`` has only ``Relation<Schema>`` among its symbols (``HasRelations``; ``HasRows`` says a family has
pools).

**Builders.** ``urd::builder<Relation<F>>`` owns its columns (``urd::data<Columns>``) and a pool of its rows
(``RowPool``), nothing else: no index and no content identity. The pool key is given where it is interned:
``insert(builder)`` for ``Schema``, ``insert(pool, builder)`` for other families. A builder is read through a layer
(``repository.view(builder)`` or ``urd::resolve(builder, repository)``, a ``BuilderRelationView``), and operations
take only views. It draws its number (``layer()``, the context number of its rows' views and of its
``relation_identity()``) at construction from the process-wide counter of the repository layers.

**Views.** An interned ``urd::index_view<Relation<F>, Context>`` and a ``BuilderRelationView`` give ``columns()``,
``size()``, ``row(i)``, ``rows()``, ``contains(row)`` and ``relation_identity()`` (an interned relation's layer number
and index, or a builder's address, valid while it is unchanged; alias checks and the join cache use it). An interned
view reads its row set over its pool through ``urd::subset_view``; a builder's view reads the builder's pool by
position.

**Rows.** Every row is a ``Row<F>`` view, the same for every family (tyr's bindings and fact rows too). It holds its
key ``{relation, row}`` and reads through its context, the layer or the builder:

.. list-table::
   :header-rows: 1

   * - Accessor
     - Result
   * - ``value(i)``
     - The i-th canonical value.
   * - ``type(i)``
     - Its column's type. A borrowed row of a pool no layer made yet has none (``std::out_of_range``).
   * - ``get(i)``
     - The decoded ``ColumnValue``.
   * - ``get<T>(i)``, ``get<T>(first, last)``, ``get<T>()``
     - A ``T`` or a range of them, checked against the column types (``std::invalid_argument``). ``T`` is an
       alternative or an object index of any tag, resolved through the context with ``urd::resolve``.
   * - ``get_values()``
     - The stored row.

Inserting, finding and testing membership take a row view, a stored row of equal widths or a span of values. Inserting
checks the count (``std::invalid_argument``) and the fit (``std::out_of_range``) before it mutates. Row and relation
iterators borrow storage; they copy no payloads.

**Lifetimes.** Builders are movable and noncopyable. Insertion keeps existing row storage, erase moves the last row
into the erased slot, clear invalidates rows but keeps capacity, and replacing the schema invalidates borrowed schema
spans. Keep a borrowed row's owner and schema alive and unchanged. There is no internal synchronization.

Operations
----------

Operations accept views satisfying ``RelationViewConcept<V>`` (interned views, or builders read through a repository),
a plan built once from their columns, and an output builder initialized with the plan's output columns. Each replaces
the output's rows.

.. list-table::
   :header-rows: 1

   * - Operation
     - Plan
     - Meaning
   * - ``select(input, predicate, out)``
     - none
     - Keep rows accepted by a predicate over a stored row.
   * - ``select_equal_columns(input, plan, out)``
     - ``SelectEqualPlan(input, a, b)``
     - Compare columns of the same type.
   * - ``select_equal_value(input, plan, out)``
     - ``SelectValuePlan(input, column, type, value)``
     - Match a canonical constant of the column's type.
   * - ``project(input, plan, out, workspace)``
     - ``ProjectionPlan(input, labels)``
     - Keep the ordered columns and eliminate duplicate rows.
   * - ``join(lhs, rhs, plan, out, workspace)``
     - ``JoinPlan(lhs, rhs)``
     - Natural join on common labels, followed by rhs-only columns.
   * - ``join(lhs, rhs, plan, cache, reuse, out, workspace)``
     - ``JoinPlan(lhs, rhs)``
     - As join, reusing ``JoinIndexCache`` indexes of inputs marked in ``JoinReuse``.
   * - ``union_(lhs, rhs, plan, out)``
     - ``SetPlan(lhs, rhs)``
     - Union with an identical ordered typed schema.
   * - ``difference(lhs, rhs, plan, out)``
     - ``SetPlan(lhs, rhs)``
     - Difference with an identical ordered typed schema.
   * - ``semijoin(lhs, rhs, plan, out, workspace)``
     - ``JoinPlan(lhs, rhs)``
     - lhs ⋉ rhs: the lhs rows with a match in rhs on the common labels; the output has the lhs columns.
   * - ``antijoin(lhs, rhs, plan, out, workspace)``
     - ``JoinPlan(lhs, rhs)``
     - lhs ▷ rhs: the lhs rows without such a match. Without common labels, lhs if rhs is empty.
   * - ``assign(destination, source)``
     - none
     - Replace the destination's columns and rows by the source's.
   * - ``builder.rename(columns)``
     - none
     - Change labels while retaining types and rows.

Plans take the inputs' ``columns()`` spans, do all validation and resolve their fields outside the row loop: common
join labels and equality columns must have matching types, set inputs equal columns, labels must exist
(``std::out_of_range``) and outputs have unique labels (``std::invalid_argument``). Per call an operation only clears
the output and throws ``std::invalid_argument`` where the output is one of its inputs; debug builds assert that the
inputs' and the output's columns are the plan's. An exception during evaluation (a throwing predicate) may leave
partial output. ``assign`` of a builder's own view leaves it as it is.

Join keys mix their fields' canonical values into a hash (``urd::hash_columns``); collisions compare the values. A
join indexes the smaller input. ``JoinIndexCache`` may retain indexes for immutable inputs, keyed by the input's
``relation_identity()`` and the ordered key positions. Full and incremental operations share key hashing, equality and
field copying.

Reuse and interning
-------------------

Keep a ``Workspace`` (an output row and a join index) and the output builders across evaluations: they retain their
capacity. Each evaluator or thread needs its own workspace. Evaluators take the layer (``const Repository&``) their
builders are read through, and return their results as ``BuilderRelationView``\ s.

``Repository::get_relation_builder<F>(columns)`` hands out builders as a ``std::shared_ptr`` from the layer's slots,
keyed by column widths. A slot goes back with its last owner and keeps its row storage, so it is reused only for
columns of equal widths. Builders are never interned, and the layer must outlive them.

``tyr::datalog::database::Repository`` is a yggdrasil formalism repository layer whose symbols are the relation model:
``Schema`` (the column types), ``Columns`` (labels and types) and ``Relation<Schema>`` (``{pool, columns, rows}``,
rows as sorted row ids). Rows are interned per schema, in one pool of the layer each, so relations of the same column
types share rows, and relations with identical columns and rows share identity regardless of insertion order.
Iteration follows the sorted row ids, not value order.

A relation is interned as any record, from scratch records checked out of a ``BuilderStorage`` that lists
``Row<Schema>``, ``Columns`` and ``Relation<Schema>``: each row first, then the columns, then the relation (whose
insertion sorts its row ids and checks its columns against its pool):

.. code:: cpp

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

Renaming is a record with the same pool and rows and new columns; copying from another repository interns the rows
again. Clearing a repository invalidates all its views while keeping reusable storage; reset the caches that borrow
its rows first.

A zero-column empty relation represents false; inserting the empty row makes it true. Projecting a nonempty input
onto no columns yields true.

Incremental evaluation
----------------------

``database/incremental/projection.hpp``, ``join.hpp`` and ``semijoin.hpp`` retain current results and maintain the
added and removed rows, in the same typed schemas and rows:

- Projection tracks witnesses; replacing a witness without changing membership produces no output delta.
- Join supports updates to either or both operands.
- Semijoin and antijoin re-examine only the changed lhs rows and the lhs rows sharing a key with a changed rhs row.

Every evaluator takes the layer first, and its ``update`` takes one change per input: an ``(added, removed)`` pair
read with ``std::get``, such as ``Delta::change()`` or ``std::tie(added, removed)`` of views
(``incremental::RelationChange`` states the requirement). Results are ``BuilderRelationView``\ s. See
:doc:`incremental-relations`.

Python
------

.. code:: python

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

The bindings expose the library types directly:

- **Columns** are lists of ``ColumnLayout(label, type)`` with ``ColumnIndex`` labels. ``Relation(labels)`` gives
  every column ``UINT32``; ``Relation(columns)`` also accepts another relation's ``columns()``. ``ColumnType`` has
  ``UINT32``, ``INT32``, ``UINT64``, ``INT64``, ``FLOAT32``, ``FLOAT64``, ``BOOL`` and ``OBJECT``.
  ``.columns()`` is a borrowed ``ColumnIndex`` sequence with ``type(position)``.
- **Rows** yield Python values of the column's type.
- **Repository.** ``RelationRepository.get_relation_builder(columns)`` gives a builder, ``view(relation)`` reads a
  builder's rows (a ``BorrowedRelation``), and ``insert(relation)``, ``copy(view)`` and ``rename(view, labels)``
  intern relations.
- **Operations** are bound as in C++: the plans (``ProjectionPlan``, ``JoinPlan``, ``SetPlan``, ``SelectEqualPlan``,
  ``SelectValuePlan(input, column, type, value)``), a ``Workspace``, and ``project``, ``join``, ``semijoin``,
  ``antijoin``, ``union``, ``difference``, ``select_equal_columns``, ``select_equal_value`` and ``assign``, taking
  ``(inputs..., plan, out[, workspace])``. The bindings check that the inputs' and the output's columns are the
  plan's (``ValueError``), which C++ only asserts.
- **Lifetimes.** Rows, views and schemas keep their Python owners alive; clearing an owner still invalidates them.

Run the native tests with ``ctest --test-dir <build> -R '^database_' --output-on-failure``; the Python tests are in
``python/tests/test_database.py``.

Query graphs and optimization
-----------------------------

``<tyr/datalog/lifted/database/syntax/query_repository.hpp>`` adds an interned relational query graph that captures
no input rows. Input-slot numbers identify the bindings at evaluation time; repeated references to a slot use the same
input relation.

- **Building.** Check out a ``urd::data<Query<Tag>>`` from a ``QueryBuilder``, fill its operands and parameters, and
  intern it with ``insert_query(repository, builder, data)`` (``query_construction.hpp``), which inserts the concrete
  record and its heterogeneous root. ``prepare_for_insert`` validates operands, schemas, constants and execution
  orders. Records hold plain indices, so operands stay in the repository they are inserted into.
- **Records.** ``Query<Tag>`` is a concrete operation holding only its own operands and parameters (``QueryJoinTag``
  has ``lhs`` and ``rhs``; ``QuerySelectValueTag`` has ``arg``, ``column`` and an encoded ``constant``). ``Query<>``
  is the heterogeneous root, a variant of concrete query indices. Data, views and the repository are in
  ``query_data.hpp``, ``query_view.hpp`` and ``query_repository.hpp``.
- **Identity.** Input and empty schemas and the ordered labels of projection and rename are part of a record's
  identity. Derived output columns and prepared plans (of join, projection and Generic Join; the resolved
  positions of selections) are cached and are not.
- **Views.** ``insert_query`` returns a ``QueryView``; ``get_variant()`` gives the concrete view with its getters
  (``get_lhs()``, ``get_arg()``, ``get_constant()``). Every view has ``columns()`` and borrows its repository.
  ``size()`` counts the root queries.
- **Generic Join** is a query constructor with input queries, a variable order and an independent output order; an
  empty output order becomes the natural union of the input columns.

``compile(roots)`` produces a copyable, read-only ``QueryPlan`` that owns its query repository and stays valid after
the source repository is destroyed. It copies only reachable queries, children before parents, keeps shared queries
shared, and preserves the root order and the ordered, typed output schemas. ``node_count()``, ``root_count()`` and
``roots()`` give its shape; indexing by a root query index returns a ``QueryView``. Row enumeration order is not
guaranteed.

Include ``<tyr/datalog/lifted/database/syntax/formatter.hpp>`` to format a plan with ``fmt::format("{}", plan)``,
``ygg::to_string(plan)`` or ``explain(plan)`` (Python: ``plan.explain()``). The ``fmt::formatter`` follows
``YGG_ENABLE_FMT_FORMATTERS``; ``explain()`` is always available.

``<tyr/datalog/lifted/database/optimization/optimization.hpp>`` plans the roots from what is known about the data:

.. code:: cpp

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

``optimize(roots, statistics = {})`` returns a ``QueryPlan`` with every root's ordered schema; the roots belong to one
repository. It implements established algorithms as published:

1. **Normalization.** The textbook algebraic rewrites [GMUW08]: selections are pushed down through joins, projections,
   renames, unions and differences, and into the lhs of semijoins and antijoins; projections through renames and
   unions; empty relations are propagated.

2. **Join blocks.** Each maximal tree of joins is planned as a whole. Unions, differences, semijoins, antijoins
   and selections spanning several join operands bound the blocks.

3. **Planning each block.** A block whose operands all have known cardinalities (measured inputs, observed queries, or
   queries over them) is planned cost-based, any other block from structure:

   - *Cost-based:* DPccp [MN06] finds the bushy join order of least C_out [CM95, Leis15] under System R estimates
     [Selinger79], with the textbook estimates of [GMUW08] for projection, union, and difference, T(R ⋉ S) = min(T(R),
     T(π_R(R ⋈ S))) and T(R ▷ S) = T(R) − T(R ⋉ S), and observed cardinalities replacing estimates [LEO01]; disconnected
     components are joined last by cross products. The join tree is then refined by Algorithm 4 of [Freitag20]: growing
     joins and their ancestors become one worst-case-optimal join.
   - *Structure:* if the GYO reduction [Graham79, YO79] shows the block acyclic, Yannakakis' algorithm [Yannakakis81];
     otherwise Generic Join [NRR13], whose running time is bounded by the AGM bound [AGM08].

   Generic Join's variable order is fixed by column label: any order attains the worst-case bound, but [Freitag20]
   additionally optimizes it with the Tributary Join cost model, which is not implemented.

4. **Compilation** of the planned roots into an owning ``QueryPlan``.

Joins are not distributed over unions, and blocks of different roots are planned independently (shared subplans are
still interned once). Planning for incremental maintenance is not modeled.

References:

- [AGM08] A. Atserias, M. Grohe, D. Marx, *Size Bounds and Query Plans for Relational Joins*, FOCS 2008.
- [CM95] S. Cluet, G. Moerkotte, *On the Complexity of Generating Optimal Left-Deep Processing Trees with Cross
  Products*, ICDT 1995.
- [Freitag20] M. Freitag, M. Bandle, T. Schmidt, A. Kemper, T. Neumann, *Adopting Worst-Case Optimal Joins in
  Relational Database Systems*, PVLDB 13(11), 2020.
- [GMUW08] H. Garcia-Molina, J. D. Ullman, J. Widom, *Database Systems: The Complete Book*, 2nd ed., 2008, ch. 16.
- [Graham79] M. H. Graham, *On the Universal Relation*, University of Toronto, 1979.
- [LEO01] M. Stillger, G. Lohman, V. Markl, M. Kandil, *LEO – DB2's LEarning Optimizer*, VLDB 2001.
- [Leis15] V. Leis et al., *How Good Are Query Optimizers, Really?*, PVLDB 9(3), 2015.
- [MN06] G. Moerkotte, T. Neumann, *Analysis of Two Existing and One New Dynamic Programming Algorithm for the
  Generation of Optimal Bushy Join Trees without Cross Products*, VLDB 2006; corrected exclusion set from PVLDB 11
  (2018), p. 1069.
- [NRR13] H. Q. Ngo, C. Ré, A. Rudra, *Skew Strikes Back: New Developments in the Theory of Join Algorithms*, SIGMOD
  Record 2013.
- [Selinger79] P. G. Selinger et al., *Access Path Selection in a Relational Database Management System*, SIGMOD 1979.
- [Yannakakis81] M. Yannakakis, *Algorithms for Acyclic Database Schemes*, VLDB 1981.
- [YO79] C. T. Yu, M. Z. Özsoyoğlu, *An Algorithm for Tree-Query Membership of a Distributed Query*, COMPSAC 1979.

Relaxation
~~~~~~~~~~

``relax(roots, known, repository, builder)`` (``syntax/translations/relaxation.hpp``) over-approximates queries whose
inputs not marked ``known(slot)`` may hold any rows. Each result Q̂ reads only the known inputs, which are renumbered
once for all roots into ``Relaxation::slots``, and has a subset S' of the query's labels. For all inputs, Q ⊆ Q̂ ×
U^(labels not in S'), and ``nullopt`` means nothing is constrained.

Relaxing π_L(Q) needs only the labels L. Labels no one needs are eliminated as early as possible:

- a conjunct that shares nothing needed becomes a nullary existence test;
- antijoins and differences are kept only as exact filters on needed labels.

This is the projection of pattern databases. The header has the full translation table and references. Evaluate the
results like any query, e.g. after ``optimize()``.

Statistics
~~~~~~~~~~

``Statistics`` describes what is known; everything is optional:

- ``objects``: the domain size. It caps distinct counts; without it the domain is unbounded.
- ``inputs``: per input slot, a ``RelationStatistics`` with ``rows`` and distinct counts keyed by ``Index<Column>``. A
  missing distinct count is the row count, capped by ``objects``.
- ``expressions``: observed results of queries in the roots' repository, keyed by ``query.index()``. They replace the
  derived estimate of the equivalent planned query [LEO01].

``collect_statistics(bindings)`` scans relations to fill ``inputs``. Python keys distinct counts by plain column
labels and expressions by ``QueryIndex``. Estimates assume independent, uniform columns and containment of join
columns [Selinger79]; conjunctions are normalized so join association never changes the estimate, and projection
starts a new scope for its hidden columns. Statistics guide planning only; they never justify replacing an input by a
constant or an empty relation.

Evaluators
~~~~~~~~~~

Evaluators differ along two independent axes: what they return, the whole result (*full*, the default) or answers to
lookups with some output columns given (*bound*); and whether they follow input changes (*incremental*) or evaluate
the current inputs only (*one-shot*, the default). A namespace exists only for a non-default mode, its directory under
``<tyr/datalog/lifted/database/semantics/>`` has the same name, and the class is always ``QueryEvaluator``:

.. list-table::
   :header-rows: 1

   * -
     - one-shot
     - incremental
   * - full
     - ``QueryEvaluator`` (``query_evaluation.hpp``)
     - ``incremental::QueryEvaluator`` (``incremental/query.hpp``)
   * - bound
     - ``bound::QueryEvaluator`` (``bound/query_evaluation.hpp``)
     - ``incremental::bound::QueryEvaluator`` (``incremental/bound/query_evaluation.hpp``)

Use full evaluation for a whole result, bound evaluation for many selective lookups against the same inputs (the edge
oracle of a goal search), and an incremental evaluator when the inputs change between uses. The operator plans
of the query records (``JoinPlan``, ``ProjectionPlan``, ``GenericJoinPlan``) are shared by all
evaluators.

- **Full.** ``evaluate(bindings)`` computes all roots, reusing retained output and scratch storage.
- **Incremental.** ``initialize`` takes the first inputs and ``update(changes)`` one ``(added, removed)`` pair of
  relation views per input slot: actual set additions and removals in that input's ordered typed schema, an empty
  relation of that schema for no change. Swap additions and removals to undo a transition.
- **Bound.** The query is compiled into union normal form (``bound/union_normal_form.hpp``) and each branch is
  planned per set of bound columns (``bound/conjunctive_plan.hpp``). The incremental bound evaluator also offers
  ``bind``, which diffs the inputs when the changes are unknown, and ``seed``, which reports the answers a change can
  add.

``get_result(root_index)`` borrows a root's current relation and ``get_delta(root_index)`` the exact net added and
removed rows after an update; initialization clears the deltas. A later evaluation or update can invalidate borrowed
rows. Inputs need only stay alive during the call, and an evaluator's results may not be fed back to it as inputs.
``memory_usage()`` reports the evaluator's retained storage.

Python query API
~~~~~~~~~~~~~~~~

.. code:: python

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

As in C++, every operator has an index (``QueryJoinIndex``), a record (``QueryJoinData``) and a view (``QueryJoin``)
with its getters. ``repository.insert(data)`` returns ``(view, created)``, and ``QueryData(view)`` makes the
type-erased ``Query`` that operators take as operands. The records:

- ``QueryInputData(slot, columns)``, ``QueryEmptyData(columns)``
- ``QueryJoinData``, ``QueryUnionData``, ``QueryDifferenceData``, ``QuerySemijoinData``,
  ``QueryAntijoinData(lhs, rhs)``
- ``QueryProjectData``, ``QueryRenameData(arg, labels)``
- ``QuerySelectEqualData(arg, lhs, rhs)``, ``QuerySelectValueData(arg, column, type, value)`` (the constant must have
  the selected column's type)
- ``QueryGenericJoinData(inputs, variable_order, output_order=[])``

``compile(roots)`` and ``optimize(roots, statistics=Statistics())`` take a list of roots and return a ``QueryPlan``.
``IncrementalQueryEvaluator(repository, plan)`` has ``initialize(inputs)`` and ``update(changes)`` with one
``(added, removed)`` pair per input slot; its results and ``RelationDelta`` relations are read-only
``BorrowedRelation`` views that keep the evaluator alive (``db.assign(formalism.Relation(), relation)`` copies one).
Query handles keep their repository alive; compiled plans and evaluators are independent of it. With sparse slot
numbers, an unused slot takes an empty ``Relation()``. Dictionary-valued statistics fields are converted by value:
replace the field to change entries.

``database_query_benchmark`` separates optimizer time from execution and compares full and incremental evaluation of
the original plan and of the plans from structure and from statistics.

Datalog
-------

``<tyr/datalog/lifted/database/datalog/>`` (namespace ``tyr::datalog::database::datalog``) is a stratified Datalog
evaluator over relations of object columns, with aggregates and weighted relations. Its records are interned in a
``DatalogRepository`` (``repository.hpp``), built with ``checkout``/``insert`` and validated by ``prepare_for_insert``
(``construction.hpp``):

- a relation: an input (with an input slot of the evaluator) or a derived relation (with an id);
- an atom: a relation applied to terms, each a variable numbered within its rule or an object constant;
- an aggregate: ``result = function value : atoms, not negated, comparisons``, grouped by the variables that the
  enclosing rule's positive atoms bind; COUNT and SUM are 0 for a group without rows;
- a rule: ``head ← atoms, not negated, comparisons, aggregates, arithmetic``, with a derived head of distinct variables
  and every variable bound by a positive atom, an aggregate's result or an arithmetic result;
- comparisons ``=``, ``≠``, ``<``, ``≤`` over values, and arithmetic ``result = lhs op rhs`` (``+ − × ÷ min max``, in
  order, each reading the variables bound before it) over {0, …, M − 1} ∪ {∞} with M the largest ``uint_t`` as ∞:
  ``+`` and ``×`` saturate to ∞, ``−`` at 0, ``÷ 0`` is ∞;
- a weighted relation: a derived relation whose last column is a weight, keeping per row of its other columns the least
  weight derived; a rule into it computes the head's weight as the sum or maximum (``+``, ``max``) of its weighted
  atoms' weights and other terms, and uses those weights nowhere else, so that the weight functions are non-decreasing
  and superior; weighted and unweighted relations are not recursive together;
- a program: rules, stratified by the strongly connected components of their dependencies
  (``detail/stratification.hpp``).

Evaluation
~~~~~~~~~~

``Evaluator(layer, program, outputs, static_slots)`` (``evaluator.hpp``) evaluates the requested relations rule by rule
over indexed relations, without intermediate relations. Its inputs are relation views of ``uint_t`` columns, one per
input slot; its results (``get_result(i)``, columns labeled 0, …, arity − 1) are read through ``layer``, and
``get_delta(i)`` is an output's exact change by the last update.

- Only the strata the outputs depend on are evaluated, and only the inputs they read are read (``reads(slot)``); each
  stratum is one object of its kind (``detail/strata.hpp``: plain, recursive, aggregate or weighted), which owns the
  state it maintains. Static input slots are read once.
- Each rule is an index nested-loop join, planned once per literal that reads a change, with the most bound literal
  next; hash indexes are chosen per access pattern. Atoms whose only unbound variables are those a step binds are
  intersected with it: the one with the fewest matches enumerates and the others are checked (Generic Join [NRR13]).
- ``evaluate(inputs)`` is stratified and semi-naive. ``initialize(inputs)`` and ``update(changes)`` maintain the
  relations: a stratum without recursion by derivation counts, a recursive one by Delete and Rederive [Gupta93]; when
  the over-deleted rows exceed a tenth of the stratum's rows, the stratum is evaluated anew. ``initialize()`` of an
  initialized evaluator updates by the difference to the rows read last, so that what the strata maintain carries over.
- An aggregate keeps the satisfying assignments of its body by counting and its groups incrementally.

Weighted strata and goal searches
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A stratum of weighted relations is evaluated by Knuth's lightest derivation algorithm [Knuth77, as stated in FM07 §3]:
rows settle in order of weight from a priority queue, each expanded by the rules that read it with the settled rows.
The evaluator recognizes shapes of such strata and searches them instead (``detail/goal_search.hpp``); these criteria
decide the search, the results are the same:

1. **Goals.** If only goals read the stratum (rules of later strata with a ``min`` head over one atom of its relation,
   the targets given by literals over earlier strata, and none of its relations an output), it stops once every goal
   holds. A goal with key columns, which the stratum's rules keep, holds per key, and the rows of a key whose goal
   holds are no longer settled.
2. **Heuristic.** With goals, rows settle by weight plus a heuristic derived from the rules [FM07 §4, §5.1]: their
   literals over static inputs give an abstract graph per key column (single-column pattern databases [CS98]), whose
   distances to the current targets bound the weight from below; h is their maximum. With one goal whose key columns
   are target columns, each goal key has its own tables to its own targets. Once a search has expanded as many
   vertices as these tables have values, it also prunes the vertices from which no target is reachable in the
   abstraction that keeps the literals over the current state (``ProvesUnreachable``), so that the proof at most
   doubles the search's cost.
3. **Frontier.** With one goal whose targets bind every key column, a breadth-first search per goal key backward from
   its targets over the rules reversed with all their literals advances with the forward one; its exhaustion without
   reaching a source proves that the key's goal has no row [Pohl71].
4. **Unit costs.** If, in addition, the base rules derive weight 0 and each recursive rule adds 1 to the weight of the
   row it reads (nothing else reads that weight), and every recursive rule can be reversed with all its literals, each
   goal key is searched by A\* (``semantics/search/a_star.hpp``) over the rules as edges, with learned distances
   [GAA*], stored paths [MPGAA*] and a backward frontier, kept across updates while the key's targets and the heuristic
   stay the same. The edges an update adds are derived by the rules with a changed literal reading its change and
   repaired (``TreeRepair``), or restart the frontier where they break its bounds; a change the rules cannot express
   discards the searches.
5. **Any path.** If, with unit costs, no weight of the goals' heads can be observed (no output or aggregate reads it,
   and no rule uses it but by copying it into such a head), a goal holds with any weight: each search ends at the
   first path it finds and derives the row at weight 0, and the backward frontier starts anew once a table that the
   recursive rules read changed.

Otherwise an update evaluates a weighted stratum anew.

Distances and must-pass objects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Datalog expresses the former query operator for distances and runir's must-pass features. The minimum distance from
``source`` to ``target`` over ``edge``, searched by criteria 1 to 4:

.. code:: text

   dist(x, min 0)  ← source(x)
   dist(y, min v)  ← dist(x, w), edge(x, y), v = w + 1
   distance(min w) ← dist(x, w), target(x)

The probes ``c`` that every path from a key's sources to its targets passes (the targets being reachable). Nothing
reads the weights of ``reachable`` and ``escapes``: ``reach`` is searched by criteria 1 to 5; ``avoid``, whose goal key
includes the probe that the targets do not bind, by criteria 1 and 2.

.. code:: text

   reach(k, x, min 0)     ← source(k, x)
   reach(k, y, min v)     ← reach(k, x, w), edge(k, x, y), v = w + 1
   reachable(k, min w)    ← reach(k, x, w), target(k, x)
   avoid(k, c, x, min 0)  ← source(k, x), probe(c), x ≠ c
   avoid(k, c, y, min v)  ← avoid(k, c, x, w), edge(k, x, y), y ≠ c, v = w + 1
   escapes(k, c, min w)   ← avoid(k, c, x, w), target(k, x)
   escaped(k, c)          ← escapes(k, c, w)
   must_pass(k, c)        ← reachable(k, w), probe(c), not escaped(k, c)

The tests ``datalog_database_datalog`` check both programs against naive evaluation under updates, and the must-pass
program against breadth-first searches as well.

Python exposes the records as ``DatalogRelationData``, ``DatalogAtomData``, ``DatalogAggregateData``,
``DatalogRuleData``, ``DatalogProgramData`` (interned by ``DatalogRepository().insert``) and the evaluator as
``DatalogEvaluator(repository, program, outputs, static_slots=[])``, with ``evaluate``, ``initialize`` and ``update``
as above.

- [CS98] J. C. Culberson, J. Schaeffer, *Pattern Databases*, Computational Intelligence 14(3), 1998.
- [FM07] P. F. Felzenszwalb, D. McAllester, *The Generalized A\* Architecture*, JAIR 29, 2007.
- [GAA*] X. Sun, S. Koenig, W. Yeoh, *Generalized Adaptive A\**, AAMAS 2008.
- [Gupta93] A. Gupta, I. S. Mumick, V. S. Subrahmanian, *Maintaining Views Incrementally*, SIGMOD 1993.
- [Knuth77] D. E. Knuth, *A Generalization of Dijkstra's Algorithm*, Information Processing Letters 6(1), 1977.
- [MPGAA*] C. Hernández, R. Asín, J. A. Baier, *Reusing Previously Found A\* Paths for Fast Goal-Directed Navigation
  in Dynamic Terrain*, AAAI 2015.
- [Pohl71] I. Pohl, *Bi-directional Search*, Machine Intelligence 6, 1971.
