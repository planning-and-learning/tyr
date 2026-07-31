# Tyr: Generalized Planning in C++20 and Python

Tyr is designed to address several challenges in modern planning systems:

1. **Unified grounded and lifted planning** within a type-safe API.

2. **Rapid prototyping** through Python bindings with type hints, backed by a high-performance C++ core.

3. **Support for expressive numeric planning formalisms** across both grounded and lifted reasoning paradigms (see [Supported PDDL Features](docs/PDDL_SUPPORT.md)).

4. **Integration of learning and reasoning** by supporting collections of planning tasks over a shared planning domain.

## Technical Overview

- **PDDL frontend**: Tyr uses [Loki](https://github.com/planning-and-learning/loki) to parse, normalize, and translate PDDL input. The parser is implemented with [Boost](https://www.boost.org/) and provides informative error messages for syntactically invalid input. The normalization pipeline largely follows the approach described in Section 4 of [*Concise finite-domain representations for PDDL planning tasks*](https://ai.dmi.unibas.ch/papers/helmert-aij2009.pdf).

- **Datalog engine**: Tyr implements a parallel semi-naive Datalog engine for lifted successor generation, axiom evaluation, relaxed planning graph heuristics, and task grounding. Its execution model is synchronous and supports both rule-level and grounding-level parallelism.

- **Ground planning**: For grounded tasks, Tyr uses data structures inspired by [*The Fast Downward Planning System*](https://jair.org/index.php/jair/article/view/10457) to efficiently identify applicable actions in a given state. Grounding often yields substantial performance improvements, although it is not always feasible for large tasks.

- **State representation**: Tyr statically analyzes domain and problem files and partitions predicates, functions, and related structures into strongly typed categories such as static, fluent, and derived atoms. This design prevents accidental mixing of conceptually different entities. To represent sequences compactly, Tyr uses tree databases of perfectly balanced binary trees, allowing common subsequences to be shared through shared subtrees. As a special case, Tyr synthesizes finite-domain variables for fluent atoms in grounded planning, largely following the method described in Section 5 of [*Concise finite-domain representations for PDDL planning tasks*](https://ai.dmi.unibas.ch/papers/helmert-aij2009.pdf), enabling more compact storage when grounding is feasible.

- **Memory model**: Tyr stores generated data in hierarchically structured, geometrically growing buffers. For variable-sized objects, it uses [Cista](https://github.com/felixguendling/cista) for serialization and zero-copy deserialization. This design allows derived buffers to inherit data from parent buffers without duplication. For example, multiple tasks can share a domain, and multiple workers can share task data.
  
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
from pytyr.planning import ExecutionContext
import pytyr.formalism.planning as tfp
import pytyr.planning.lifted as tpl  # pytyr.planning.ground also exists

# Parse and translate a task over a domain.
parser = tfp.Parser("domain.pddl")
# Instantiate a lifted task.
task = tpl.Task(parser.parse_task("problem.pddl"))

# Instantiate a single-threaded execution environment.
execution_context = ExecutionContext(1)

# Instantiate the planning objects. Factories assign unique context indices so
# state views from different state repositories hash and compare correctly.
axiom_evaluator_factory = tpl.AxiomEvaluatorFactory()
state_repository_factory = tpl.StateRepositoryFactory()
successor_generator_factory = tpl.SuccessorGeneratorFactory()
axiom_evaluator = axiom_evaluator_factory.create(task, execution_context)
state_repository = state_repository_factory.create(task, axiom_evaluator)
successor_generator = successor_generator_factory.create(task, execution_context, state_repository)

# Get the initial node (state + metric value)
initial_node = successor_generator.get_initial_node()

# Get the labeled successor nodes (sequence of action binding + node)
labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(initial_node)
```

## C++ Interface

The C++ interface for implementing search algorithms is:

```cpp
#include <tyr/tyr.hpp>

// Recommended namespace aliases.
namespace tfp = tyr::formalism::planning;
namespace tp = tyr::planning;

// Parse and translate a task over a domain.
auto parser = tfp::Parser("domain.pddl");
// Instantiate a lifted task.
auto task = tp::Task<tp::LiftedTag>::create(parser.parse_task("problem.pddl"));

// Instantiate a single-threaded execution environment
auto execution_context = ygg::ExecutionContext::create(1);

// Instantiate the planning objects. Factories assign unique context indices so
// state views from different state repositories hash and compare correctly.
auto axiom_evaluator_factory = tp::AxiomEvaluatorFactory<tp::LiftedTag>();
auto state_repository_factory = tp::StateRepositoryFactory<tp::LiftedTag>();
auto successor_generator_factory = tp::SuccessorGeneratorFactory<tp::LiftedTag>();

auto axiom_evaluator = axiom_evaluator_factory.create(task, execution_context);
auto state_repository = state_repository_factory.create(task, axiom_evaluator);
auto successor_generator = successor_generator_factory.create(task, execution_context, state_repository);

// Get the initial node (state + metric value).
auto initial_node = successor_generator->get_initial_node();

// Get the labeled successor nodes (sequence of action binding + node).
auto labeled_successor_nodes = successor_generator->get_labeled_successor_nodes(initial_node);

```

## Dependencies

Tyr consumes native dependencies from Python packages:

- `pyyggdrasil >= 0.0.27, < 0.1` for shared third-party native dependencies.
- `pypddl >= 1.0.26, < 1.1` for Loki's PDDL parser library, headers, and CMake package.
- `pypddl-datasets >= 0.0.9, < 0.1` for the PDDL benchmark data used by the C++ test and profiling fixtures (resolved from its cache at CMake configure time).

The shared workspace layout, layered install order, and the common
build-from-source and CMake-integration patterns are documented in the
[Planning and Learning build instructions](https://github.com/planning-and-learning/.github/blob/main/profile/README.md#building-from-source);
the sections below cover `tyr`/`pytyr`-specific details.

## Build C++

Install Tyr's native dependency providers into the active Python environment,
then configure CMake with their native prefixes:

```console
python -m pip install 'pyyggdrasil>=0.0.27,<0.1' 'pypddl>=1.0.26,<1.1' 'pypddl-datasets>=0.0.9,<0.1'

cmake -S . -B build \
  -DPython_EXECUTABLE="$(python -c 'import sys; print(sys.executable)')"

cmake --build build -j4
```

CMake discovers the installed provider packages automatically through
`cmake/bootstrap_pyyggdrasil.cmake` (which locates `pyyggdrasil` and adds its
native prefix to `CMAKE_PREFIX_PATH`; `find_package(yggdrasil)` then resolves
the rest of the chain) and links against the `yggdrasil::yggdrasil` and
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
| `TYR_BUILD_PROFILING` | `OFF` | Build Tyr profiling targets. |
| `TYR_BUILD_PYTYR` | `OFF` | Build `pytyr` Python bindings. |
| `TYR_HEADER_INSTANTIATION` | `OFF` | Instantiate templates in in-tree translation units at higher compile-time cost. |
| `TYR_ENABLE_INNER_PARALLELISM` | `OFF` | Enable inner parallelism for expensive lifted rule evaluation. |
| `TYR_ENABLE_SEMI_NAIVE` | `ON` | Enable semi-naive lifted Datalog evaluation. |
| `TYR_USE_LLD` | `ON` | Use LLVM `lld` with Clang when available. |
| `TYR_ENABLE_LTO` | `ON` | Enable link-time optimization for Release builds. |
| `TYR_STATE_STORAGE_POLICY` | `Tree` | State storage backend; accepted values are `Tree` and `Hashset`. |
| `TYR_RELATION_STORAGE_POLICY` | `Word` | Relation storage backend; accepted values are `Word` and `Bit`. Bit packing uses `num_objects` supplied when creating repositories. |

`RepositoryFactory.create_repository(parent_repository=None, num_objects=None)` accepts the total object count for the repository. `None` keeps the full object-index width; child counts include the parent object universe.

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
[`docs/BUILD.md`](docs/BUILD.md).

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
