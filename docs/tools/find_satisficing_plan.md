# Find satisficing plan

```python
from datetime import timedelta
from pytyr.planning import SearchBudget

result = lifted.find_satisficing_plan(
    task, state_repository, axiom_evaluator, successor_generator, execution_context,
    search_budget=SearchBudget(100_000, timedelta(seconds=5)),
)
```

Both `ground.find_satisficing_plan` and `lifted.find_satisficing_plan` run lazy
GBFS with hFF and return a native search result. Search uses the supplied native
components; the helper creates the FF heuristic. The call does not write files.

| Argument | Meaning |
|---|---|
| `task` | Native `pytyr.planning.ground.Task` or `pytyr.planning.lifted.Task`, matching the tool module. |
| `state_repository` | Native `StateRepository` created for this task. |
| `axiom_evaluator` | Native `AxiomEvaluator` created for this task. |
| `successor_generator` | Native `SuccessorGenerator` created for this task. |
| `execution_context` | Caller-owned `pyyggdrasil.execution.ExecutionContext`. |
| `search_budget` | Native `SearchBudget`; defaults to `SearchBudget(1_000_000, timedelta(seconds=60))`. |

All search components must use the task's backend. `SearchBudget.max_num_states`
is an `int` or `None`; `max_time` is a `datetime.timedelta` or `None`.
`SearchBudget()` defaults both limits to `None`, meaning unlimited. Zero is a
valid state limit. Both fields are writable and can also be passed by keyword
to the constructor.

The same budget type is used by native BRFS, A*, lazy GBFS, and IW:

```python
from pytyr.planning import lifted as native_lifted

options = native_lifted.gbfs_lazy.Options()
options.search_budget = SearchBudget(100_000, timedelta(seconds=5))
```

Native algorithm options default to `SearchBudget()`. Assigning a budget copies
its values into the options.

The return type is the matching `pytyr.planning.ground.SearchResult` or
`pytyr.planning.lifted.SearchResult`. `result.status` is a `SearchStatus`, such as
`SOLVED` or `OUT_OF_TIME`. `result.plan` is the native plan or `None`; a plan exposes
`get_length()` and `get_cost()`.

Use `dump_result(task, result, output_dir)` for [file output](dumping.md).
