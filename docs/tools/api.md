# Python API

`pytyr.tools` runs satisficing search on native tasks and writes results on request.

```python
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr.formalism.planning import Parser
from pytyr.planning import SearchStatus, lifted as native_lifted
from pytyr.tools import DumpFormat, dump_result, lifted

options = ParserOptions()
parser = Parser("domain.pddl", options)
task = native_lifted.Task(parser.parse_task("problem.pddl", options))
execution = ExecutionContext(1)
state_repository = native_lifted.StateRepositoryFactory().create(task)
axiom_evaluator = native_lifted.AxiomEvaluatorFactory().create(task, execution)
successor_generator = native_lifted.SuccessorGeneratorFactory().create(task, execution)
result = lifted.find_satisficing_plan(
    task, state_repository, axiom_evaluator, successor_generator, execution,
)

if result.status == SearchStatus.SOLVED:
    assert result.plan is not None
    print(result.plan.get_length(), result.plan.get_cost())

dumped = dump_result(
    task, result, "artifacts/find-plan",
    formats=(DumpFormat.JSON, DumpFormat.MD), include_plan_text=True,
)
```

The public exports are `ground`, `lifted`, `find_satisficing_plan`, `SearchBudget`,
`DumpFormat`, `DumpResult`, and `dump_result`.
The root `find_satisficing_plan` selects lifted search. Each backend accepts its
native task and caller-owned search components, and returns its native
`SearchResult`; use `result.status` and `result.plan` directly. The helper creates
the FF heuristic for each call.

`SearchBudget` is the native `pytyr.planning.SearchBudget`, also available through
`pytyr.tools`. The search helpers default to 1,000,000 states and 60 seconds.

See [task setup](context.md) for grounded search, [search](find_satisficing_plan.md)
for budget details, and [dumping](dumping.md) for the output contract.
