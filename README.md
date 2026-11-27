# Tyr: Generalized Planning in C++20 and Python

Tyr is designed to address several challenges in modern planning systems:

1. **Unified grounded and lifted planning** within a type-safe API.

2. **Rapid prototyping** through Python bindings with type hints, backed by a high-performance C++ core.

3. **Support for expressive planning formalisms** including classical, numeric, non-determinstic, probabilistic, and temporal planning, across both grounded and lifted reasoning paradigms (see [Supported PDDL Features](https://planning-and-learning.github.io/tyr/pddl-support.html)).

4. **Integration of learning and planning** by supporting collections of planning tasks over a shared planning domain.

## Technical Overview

- **PDDL frontend**: Tyr uses [Loki](https://github.com/planning-and-learning/loki) to parse, normalize, and translate PDDL input. The parser is implemented with [Boost](https://www.boost.org/) and provides informative error messages for syntactically invalid input. The normalization pipeline largely follows the approach for classical planning described in Section 4 of [*Concise finite-domain representations for PDDL planning tasks*](https://ai.dmi.unibas.ch/papers/helmert-aij2009.pdf).

- **Datalog engine**: Tyr implements a parallel semi-naive Datalog engine for lifted successor generation, axiom evaluation, relaxed planning graph heuristics, and task grounding. Its execution model is synchronous and supports both rule-level and grounding-level parallelism.

- **Ground planning**: For grounded tasks, Tyr uses data structures inspired by [*The Fast Downward Planning System*](https://jair.org/index.php/jair/article/view/10457) to efficiently identify applicable actions in a given state. Grounding often yields substantial performance improvements, although it is not always feasible for large tasks. 

- **State representation**: Tyr statically analyzes domain and problem files and partitions predicates, functions, and related structures into strongly typed categories such as static, fluent, and derived atoms. This design prevents accidental mixing of conceptually different entities. To represent sequences compactly, Tyr uses tree databases of perfectly balanced binary trees, allowing common subsequences to be shared through shared subtrees. In ground mode, Tyr synthesizes finite-domain variables for fluent atoms, largely following the method described in Section 5 of [*Concise finite-domain representations for PDDL planning tasks*](https://ai.dmi.unibas.ch/papers/helmert-aij2009.pdf), enabling more compact storage when grounding is feasible.

- **Memory model**: Tyr keeps every entity it builds (objects, atoms, actions, rules, states) as a record in a store of flat byte buffers that only grow, laid out with [Cista](https://github.com/felixguendling/cista). As in [Cap'n Proto](https://capnproto.org), a record is written through a builder and read in place through a view, with no decoding step. Unlike Cap'n Proto and [Protocol Buffers](https://protobuf.dev), which build self-contained messages, a store keeps each distinct record once and names it by a dense index: a record refers to its children by their indices, so all records form one shared graph, and two records are equal exactly when their indices are. Stores are layered: a layer reads through to its frozen parent without copying it, so multiple tasks share a domain, multiple workers share a task, and a Datalog program refers to its planning task's atoms. Once its buffers have grown, building a record allocates nothing. The [record schema](https://planning-and-learning.github.io/brokkr/schema_rules.html#records) describes the design and the comparison in full.
  
# Getting Started

The library consists of a **formalism** and a **planning** component. The formalism component is responsible for representing PDDL entities. The planning component provides functionality for implementing search algorithms, as well as off-the-shelf implementations of eager A*, lazy GBFS, and heuristics such as blind, max, add, and FF. Below is a minimal overview of the Python and C++ APIs for implementing custom search algorithms.

## Python Interface

Pytyr is available at [PyPI](https://pypi.org/project/pytyr/) and can be installed with `pip install pytyr`.

Detailed examples are available in the `python/examples` directory:

- [`structures.py`](python/examples/formalism/planning/structures.py) – Parse and traverse all planning formalism structures.
- [`builder.py`](python/examples/formalism/planning/builder.py) – Create new planning formalism structures.
- [`invariants.py`](python/examples/formalism/planning/invariants.py) – Synthesize invariants, access candidate variable bindings, and match atoms through unification.
- [`astar_eager.py`](python/examples/planning/astar_eager.py) – Use and customize off-the-shelf search algorithms.
- [`gbfs_lazy.py`](python/examples/planning/gbfs_lazy.py) – Implement a custom search algorithm from scratch.

The Python interface for implementing search algorithms is:

```py
# Recommended namespace aliases
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
import pytyr.formalism as formalism
import pytyr.planning as planning
import pytyr.planning.lifted as lifted  # pytyr.planning.ground also exists

# Parse and translate a task over a domain.
parser_options = ParserOptions()
parser = formalism.Parser("domain.pddl", parser_options)
# Instantiate a lifted task.
task = lifted.Task(parser.parse_task("problem.pddl", parser_options))

# Instantiate a single-threaded execution environment.
execution_context = ExecutionContext(1)

# Instantiate the planning objects. Each state repository has its own number, so
# state views from different state repositories hash and compare correctly.
axiom_evaluator = lifted.AxiomEvaluator(task, execution_context)
state_repository = lifted.StateRepository(task)
successor_generator = lifted.SuccessorGenerator(task, execution_context)

# The initial node: its state, registered, and its metric value.
initial_node = lifted.get_initial_node(state_repository, axiom_evaluator)

# Simple: the successors of a node, kept. Each is a transition (its label) with distributions of outcomes, each outcome
# a registered node with its probability. A classical action has one distribution of one outcome.
for successor in lifted.get_successors(successor_generator, state_repository, axiom_evaluator, initial_node):
    label = successor.get_label()
    for distribution in successor.get_distributions():
        for outcome in distribution.get_outcomes():
            node, probability = outcome.get_node(), outcome.get_probability()

# Expert: each successor borrowed during the callback, nothing interned. Keep what you need: a copy as it is
# (successor.clone()), a copy with its outcome states registered (register_outcomes), or with its label's action
# binding interned (intern_label).
kept_successors = []


def visit(successor: lifted.SuccessorBuilderView) -> bool:
    kept_successors.append(lifted.register_outcomes(state_repository, successor))
    return True  # False stops


lifted.for_each_successor(successor_generator, initial_node, axiom_evaluator, visit)

# Lowest level, what the search algorithms use: the applicable action bindings (borrowed), one successor state each,
# from the state repository, with its auxiliary value (the cost term the metric reads) and its changes to the node's
# state; registering it makes the node, interning the binding its label.
labeled_successor_nodes = []


def visit_binding(binding: formalism.RowActionLiftedBuilderView) -> bool:
    successor_state = lifted.generate_successor_state(successor_generator, state_repository, initial_node, binding)
    node = lifted.register_successor(state_repository, axiom_evaluator, initial_node, successor_state)
    label = lifted.intern_action_binding(successor_generator, binding)
    labeled_successor_nodes.append(lifted.LabeledNodeBuilder(label=label, node=node).view(state_repository))
    return True


lifted.for_each_applicable_action_binding(successor_generator, initial_node, visit_binding)
```

## C++ Interface

The C++ interface for implementing search algorithms is:

```cpp
#include <tyr/tyr.hpp>

// The kind of task: lifted (tyr::GroundTag for a ground one).
using Kind = tyr::LiftedTag;

// Parse and translate a task over a domain.
auto parser = tyr::formalism::Parser("domain.pddl");
// Instantiate a lifted task.
auto task = tyr::planning::Task<Kind>::create(parser.parse_task("problem.pddl"));

// Instantiate a single-threaded execution environment.
auto execution_context = ygg::ExecutionContext::create(1);

// Instantiate the planning objects. A state repository is created shared (create): the states it hands out keep it
// alive. Each has its own number, so state views from different state repositories hash and compare correctly.
auto axiom_evaluator = tyr::planning::AxiomEvaluator<Kind>(task, execution_context);
auto state_repository = tyr::planning::StateRepository<Kind>::create(task);
auto successor_generator = tyr::planning::SuccessorGenerator<Kind>(task, execution_context);

// The initial node: its state, registered, and its metric value, read through the state repository.
const auto initial_node = tyr::planning::get_initial_node(*state_repository, axiom_evaluator);

// Simple: the successors of a node, kept. Each is a transition (its label) with distributions of outcomes, each outcome
// a registered node with its probability. A classical action has one distribution of one outcome.
for (const auto& successor : tyr::planning::get_successors(successor_generator, *state_repository, axiom_evaluator, initial_node))
{
    const auto label = successor.get_label();
    for (const auto distribution : successor.get_distributions())
        for (const auto outcome : distribution.get_outcomes())
        {
            const auto node = outcome.get_node();
            const auto probability = outcome.get_probability();
        }
}

// Expert: each successor written into a builder the callback may take, nothing interned. Keep what you need as it is,
// register its outcome states (register_outcomes), intern its label's action binding (intern_label, through the
// reused scratch records of a formalism builder), each optional.
auto builder = tyr::formalism::Builder {};
auto kept_successors = std::vector<tyr::planning::SuccessorBuilder<Kind>> {};
const auto visit = [&](tyr::planning::SuccessorBuilder<Kind>& successor)
{
    tyr::planning::register_outcomes(*state_repository, successor);
    tyr::planning::intern_label(*state_repository, builder, successor);
    kept_successors.push_back(std::move(successor));
    return true;  // false stops
};
tyr::planning::for_each_successor(successor_generator, initial_node, axiom_evaluator, visit);

// Lowest level, what the search algorithms use: the applicable action bindings (borrowed), one successor state each,
// from the state repository, with its auxiliary value (the cost term the metric reads) and its changes to the node's
// state; registering it makes the node, interning the binding its label.
auto labeled_successor_nodes = tyr::planning::LabeledNodeList<Kind> {};
const auto visit_binding = [&](tyr::planning::ActionBindingView<Kind> binding)
{
    auto successor_state = tyr::planning::generate_successor_state(successor_generator, *state_repository, initial_node, binding);
    auto node = tyr::planning::register_successor(*state_repository, axiom_evaluator, initial_node, std::move(successor_state));
    const auto label = tyr::planning::intern_action_binding(successor_generator, builder, binding);
    labeled_successor_nodes.push_back(tyr::planning::LabeledNodeBuilder<Kind> { label.index(), std::move(node) });
    return true;
};
tyr::planning::for_each_applicable_action_binding(successor_generator, initial_node, visit_binding);
```

## Dependencies

Tyr consumes native dependencies from Python packages:

- `pyyggdrasil >= 0.14.0, < 0.15` for shared third-party native dependencies.
- `pypddl >= 1.15.0, < 1.16` for Loki's PDDL parser library, headers, and CMake package.
- `pypddl-datasets >= 0.0.22, < 0.1` for the PDDL benchmark data used by the C++ tests and benchmarks (resolved from its cache at CMake configure time).

The shared workspace layout, layered install order, and the common
build-from-source and CMake-integration patterns are documented in the
[Planning and Learning build instructions](https://github.com/planning-and-learning/.github/blob/main/profile/README.md#building-from-source);
the sections below cover `tyr`/`pytyr`-specific details.

## Build C++

Install Tyr's native dependency providers into the active Python environment,
then configure CMake with their native prefixes:

```console
python -m pip install 'pyyggdrasil>=0.14.0,<0.15' 'pybrokkr>=0.3.1,<0.4' 'pypddl>=1.15.0,<1.16' 'pypddl-datasets>=0.0.22,<0.1'

cmake -S . -B build \
  -DPython_EXECUTABLE="$(python -c 'import sys; print(sys.executable)')"

cmake --build build -j4
```

CMake discovers the installed provider packages automatically: it finds the
build helpers of [brokkr](https://github.com/verdog-ai/brokkr) (`pybrokkr`,
installed with the package), whose `brokkr_find_python_native_prefix` adds
`pyyggdrasil`'s native prefix to `CMAKE_PREFIX_PATH`; `find_package(yggdrasil)`
then resolves the rest of the chain and links against the `yggdrasil::yggdrasil` and
`loki::parsers` targets. To point at different prefixes explicitly:

```console
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$(python -m pyyggdrasil --prefix);$(python -m pypddl --prefix)"
```

CMake options:

| Option | Default | Description |
| --- | --- | --- |
| `TYR_BUILD_TESTS` | `OFF` | Build Tyr tests. |
| `TYR_BUILD_EXECUTABLES` | `OFF` | Build Tyr executables. |
| `TYR_BUILD_BENCHMARKS` | `OFF` | Build Tyr benchmark targets. |
| `TYR_BUILD_PYTYR` | `OFF` | Build `pytyr` Python bindings. |
| `TYR_HEADER_INSTANTIATION` | `OFF` | Instantiate templates in in-tree translation units at higher compile-time cost. |
| `TYR_ENABLE_INNER_PARALLELISM` | `OFF` | Enable inner parallelism for expensive lifted rule evaluation. |
| `TYR_ENABLE_SEMI_NAIVE` | `ON` | Enable semi-naive lifted Datalog evaluation. |
| `TYR_USE_LLD` | `ON` | Use LLVM `lld` with Clang when available. |
| `TYR_ENABLE_LTO` | `ON` | Enable link-time optimization for Release builds. |
| `TYR_STATE_STORAGE_POLICY` | `TreeBitset` | State storage backend; accepted values are `TreeVector`, `TreeBitset`, and `Hashset`. `TreeBitset` stores fluent facts and derived atoms as position-stable bitset trees and inserts successors as deltas to their parent. `TreeVector` and `Hashset` store them as vectors, as trees with shared structure or whole in hash tables. |

`Repository(parent_repository=None)` makes a repository layer over `parent_repository` (none: a root).

Single-config CMake builds default to Release. On GCC and Clang, Debug builds
use `-Og` with debug symbols, RelWithDebInfo keeps frame pointers and disables
LTO, and Release LTO uses GCC LTO or Clang ThinLTO. Editable installs and
wheels disable `TYR_USE_LLD` and `TYR_ENABLE_LTO` by default for build
reliability.

Install Tyr from a configured build directory with:

```console
cmake --install build --prefix=<path/to/installation-directory>
```

More detailed Tyr-specific build instructions are available in
[build instructions](https://planning-and-learning.github.io/tyr/build.html).

## Build Python

```console
python -m pip install .[test]
pytest python/tests
```

## CMake Integration

This section covers `pytyr`-specific paths and targets; the general pattern for
consuming the native prefixes from CMake is in the
[common CMake integration instructions](https://github.com/planning-and-learning/.github/blob/main/profile/README.md#cmake-integration).

The Python package `pytyr` installs Tyr's native headers, shared library, and
CMake package config under `pytyr.native_prefix()`. Use `pytyr.cmake_prefix()`
and `pytyr.cmake_dir()` (or `python -m pytyr --prefix` / `--cmake-dir` from the
shell) to locate them. Downstream CMake projects should include the native
prefixes of `pytyr` and its native package dependencies in
`CMAKE_PREFIX_PATH`:

```console
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$(python -m pyyggdrasil --prefix);$(python -m pypddl --prefix);$(python -m pytyr --prefix)"
```

Tyr exports the `tyr::core` aggregate target.
