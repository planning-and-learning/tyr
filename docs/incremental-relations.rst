Incremental relations
=====================

The C++ interfaces in ``tyr/datalog/lifted/database/incremental/`` maintain projection and natural join under changes,
including joins where both inputs change. They use the typed schemas, packed rows, ``urd::builder<Relation<Schema>>``
and views of the full operations (:doc:`database`); publishing results in a repository is optional.

Projection
----------

.. code:: cpp

   using namespace ygg;
   using namespace tyr::datalog::database;

   Repository repository;
   const std::array columns { ColumnLayout { urd::index<Column>(0), type_id<uint_t> },
                              ColumnLayout { urd::index<Column>(1), type_id<uint_t> } };
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

Each evaluator owns its result, its last output delta and its bookkeeping. The caller supplies a reusable
``Workspace`` per call; several evaluators can share one sequentially.

Projection keeps a support count per distinct result row and reports the net change of the whole batch. A row whose
count reaches zero stays stored until all additions are processed, so replacing its witnesses leaves it in place and
produces no delta. Because new rows can coexist with rows pending removal, retained capacity can reflect the old
output plus the added rows even when the final cardinality is unchanged.

Natural join
------------

Include ``incremental/join.hpp`` and construct ``JoinEvaluator(layer, plan)``. It takes relation views (a builder is
read as ``layer.view(builder)``):

.. code:: cpp

   incremental::JoinEvaluator joined(repository, JoinPlan(lhs.columns(), rhs.columns()));
   joined.initialize(lhs, rhs, workspace);
   joined.update(lhs_change.added, lhs_change.removed,
                 rhs_change.added, rhs_change.removed, workspace);

   // Undo the complete batch on both inputs.
   joined.update(lhs_change.removed, lhs_change.added,
                 rhs_change.removed, rhs_change.added, workspace);

   // Only the left input changes: pass empty relations for the right one.
   urd::builder<Relation<Schema>> unchanged(rhs.columns());
   joined.update(lhs_change.added, lhs_change.removed, unchanged, unchanged, workspace);

The evaluator owns copies of both inputs and a mutable index for each. Initialization copies the rows; updates
maintain the copies from the deltas alone, so no input view needs to stay alive between calls. Output columns follow
the left/right order of the ``JoinPlan``. A joined tuple identifies its pair of input rows, so join needs no witness
counts.

A batch removes left rows against the old right input, removes right rows against the remaining left input, adds left
rows against the remaining right input, and adds right rows against the new left input. Pairs affected on both sides
are thus counted once, and added rows never match removed ones. The output delta is the exact net change, also for
Cartesian products and zero-column relations.

Only the affected index entries change; indexes are not rebuilt per batch. Erasing or updating a row position
searches its hash group, so large groups cost more. Capacities grow as needed: allocation-free operation holds only
once they are warm.

Delta and lifetime contract
---------------------------

- **Delta.** ``Delta`` owns reusable ``added`` and ``removed`` builders with identical ordered schemas, read through
  the layer it holds (``change()`` gives their views). ``clear()`` keeps their buffers.
- **Changes** must be actual set changes: added rows were absent, removed rows were present, and the two sets are
  disjoint. Overlap is checked before mutation, and schemas against the plan are asserted in debug builds. For
  projection, source membership is the caller's precondition; join rejects absent removals and present additions
  while applying them. Detected invalid changes raise ``std::invalid_argument``.
- **Inputs** are borrowed during the call only. They must not alias the evaluator's result, its delta or the
  workspace's buffers; this is the caller's precondition beyond the ``relation_identity()`` check. Calls must be
  sequential and nonreentrant.
- **Initialization** produces the baseline and an empty delta, and keeps reusable capacities when repeated.
  ``update`` before it throws ``std::logic_error``.
- **Results.** ``get_result()`` returns a ``BuilderRelationView`` and ``get_delta()`` a const reference; do not keep
  their contents across the evaluator's next mutation. Keep the input changes yourself if you need them for undo, and
  feed an upstream output delta to downstream operators before updating the upstream operator again.
- **Exceptions.** Validation failures before mutation preserve the prior result. An exception after mutation started
  can leave a partial result and delta: call ``initialize`` before using that evaluator again.
- **Memory.** ``memory_usage()`` reports the retained working buffers (indexes with their free slots and hash-table
  capacity, projection counts, the join's copies of its inputs). It excludes the plan and the caller's workspace, and
  estimates container storage, not process RSS.

Compact erasure
---------------

Nonconcurrent ``RawArrayPool``, ``RawArraySet`` and relation builders have ``erase(index)``: it replaces that row with
the last row and removes the last slot. The positions and spans of the removed and the moved row are invalidated,
other positions stay, and parallel metadata must do the same swap. ``find(row)`` returns the current position.

Deletion rewinds the allocation cursors and keeps the allocated segments; insertion is unchanged. The hash table may
still clean its tombstones table-wide, so update cost is amortized, not a bound per delta. Invalidate cached indexes
before any mutation of their source, including erasure.

Tests and benchmarks
--------------------

The ``database_incremental`` tests compare the complete results and the exact output deltas after every update with
the full operations. With ``YGGDRASIL_BUILD_PROFILING``, ``database_incremental_benchmark`` compares full evaluation
with incremental maintenance for projection and for joins with one or both inputs changing.

References
----------

The correspondence with the literature is at the operator level: support counting for set projection and first-order
delta rules for binary natural join.

1. A. Gupta, I. S. Mumick, V. S. Subrahmanian. `Maintaining Views Incrementally
   <https://doi.org/10.1145/170035.170066>`__. SIGMOD 1993. Sections 4-5: delta rules and maintenance through
   derivation counts. ``ProjectionEvaluator`` applies the counting to projected tuples; ``JoinEvaluator`` specializes
   the join delta rules into separate removal and addition passes. The paper's recursive Delete and Rederive
   algorithm is outside their scope.
2. M. Budiu, T. Chajed, F. McSherry, L. Ryzhyk, V. Tannen. `DBSP: Automatic Incremental View Maintenance for Rich
   Query Languages <https://doi.org/10.14778/3587136.3587137>`__. PVLDB 16(7), 2023. Theorem 3.4 gives the bilinear
   delta identity underlying the join update; Proposition 4.7 the count threshold used for incremental duplicate
   elimination.

Beyond the current evaluators:

- Y. Ahmad, O. Kennedy, C. Koch, M. Nikolic. `DBToaster: Higher-order Delta Processing for Dynamic, Frequently Fresh
  Views <https://www.vldb.org/pvldb/vol5/p968_yanifahmad_vldb2012.pdf>`__. PVLDB 5(10), 2012. Auxiliary views of
  delta queries, for queries whose maintenance repeats expensive joins.
- A. Kara, M. Nikolic, D. Olteanu, H. Zhang. `F-IVM: Analytics over Relational Databases under Updates
  <https://doi.org/10.1007/s00778-023-00817-w>`__. The VLDB Journal 33, 2024. Factorized higher-order maintenance,
  for large intermediate joins.
- F. McSherry, D. G. Murray, R. Isaacs, M. Isard. `Differential Dataflow
  <https://www.cidrdb.org/cidr2013/Papers/CIDR13_Paper111.pdf>`__. CIDR 2013. Incremental dataflow with nested
  iteration, for recursive evaluation.
- D. Olteanu. `Recent Increments in Incremental View Maintenance <https://arxiv.org/abs/2404.17679>`__. 2024. A
  survey of update-time, preprocessing and enumeration-delay tradeoffs for conjunctive queries.
