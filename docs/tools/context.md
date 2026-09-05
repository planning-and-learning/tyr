# Task setup

Use the native parser and planning tasks. Create the state repository, axiom
evaluator, and successor generator for the same task and backend used in search.
The [API example](api.md) shows lifted search; grounded search uses ground factories:

```python
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr.formalism.planning import Parser
from pytyr.planning import ground as native_ground, lifted as native_lifted
from pytyr.tools import ground

options = ParserOptions()
parser = Parser("domain.pddl", options)
task = native_lifted.Task(parser.parse_task("problem.pddl", options))
execution = ExecutionContext(1)
grounded = task.instantiate_ground_task(
    execution, native_lifted.GroundTaskInstantiationOptions(),
)
if grounded.status == native_lifted.GroundTaskInstantiationStatus.SUCCESS:
    ground_task = grounded.task
    state_repository = native_ground.StateRepositoryFactory().create(ground_task)
    axiom_evaluator = native_ground.AxiomEvaluatorFactory().create(ground_task, execution)
    successor_generator = native_ground.SuccessorGeneratorFactory().create(ground_task, execution)
    ground_result = ground.find_satisficing_plan(
        ground_task, state_repository, axiom_evaluator, successor_generator, execution,
    )
else:
    print(grounded.status)
```

Grounding can report `PROVEN_UNSOLVABLE`; inspect its status before using the
returned task. The search budget applies to the subsequent search call.
`ExecutionContext(1)` selects one worker.

Source paths belong to the native formalism objects:
`parser.get_domain().get_path()` and `task.get_formalism_task().get_path()` return
`Path` or `None`. Grounding preserves this metadata. Supplied paths are kept as
provided, including relative paths. String parsing accepts an optional source
path, for example `Parser(domain_text, None, options)` and
`parser.parse_task(problem_text, None, options)`. Programmatic formalism
constructors default to no source path.
