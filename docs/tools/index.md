# pytyr.tools

Compressed, JSON-compatible representations of native Tyr planning entities.
Each representation is a plain dictionary with an explicit `TypedDict` schema.
Downstream applications choose how to combine, render, and save them.

## Representations

| Representation | Fields |
|---|---|
| Task | `name`, `domain_path`, `task_path`, and `static`. Unavailable paths are `null`. |
| Static facts | `atoms`: static atom aliases; `values`: static function aliases mapped to numbers. |
| State facts | `fluent`: fluent atom aliases; `derived`: derived atom aliases; `values`: fluent function aliases mapped to numbers. |
| Plan | `length`, `cost`, and ordered `steps`. |
| Plan step | `step`: integer position; `action`: action alias or `null`; `state`: state alias. |

Step `0` references the initial state and has `action: null`. Later steps reference
the arriving action and its resulting state. An empty plan still contains its
initial step. Revisiting a state reuses its alias.

Static facts appear once in the task representation. State definitions contain
only fluent and derived facts and fluent numeric values. A changing numeric value
reuses the same function alias.

## Dictionaries

| Dictionary | Alias prefix | Entry |
|---|---|---|
| `actions` | `a` | `id`, `action` description. |
| `static_atoms` | `c` | `id`, `atom` description. |
| `fluent_atoms` | `p` | `id`, `atom` description. |
| `derived_atoms` | `d` | `id`, `atom` description. |
| `static_functions` | `k` | `id`, `function` description. |
| `fluent_functions` | `n` | `id`, `function` description. |
| `states` | `s` | State alias mapped to state facts. |

Symbol dictionaries contain lists of entries. The state dictionary maps aliases
to state definitions. Empty dictionaries are omitted.

Aliases start at zero within each dictionary and are local to its
`Dictionaries` instance. Representations using the same instance share
definitions. Formatting states and plans registers their referenced entities;
format the dictionaries after those entities to include all their definitions.
