# Serialized representations

Serialization produces JSON values for selected native entities and their dependencies. Registered entity types become compact references; other entities remain nested objects. With no registered tables, the complete representation is inline.

A reference combines its table prefix and zero-based row position. For example, `a0` denotes the first row of the table whose prefix is `a`:

```json
{
  "atoms": {
    "prefix": "a",
    "rows": [{"binding": "b0"}]
  },
  "bindings": {
    "prefix": "b",
    "rows": [{"relation": "p0", "objects": ["o0"]}]
  },
  "predicates": {
    "prefix": "p",
    "rows": [{"name": "at", "arity": 1}]
  },
  "objects": {
    "prefix": "o",
    "rows": [{"name": "truck"}]
  }
}
```

Table names and prefixes are chosen by the caller. Both must be nonempty and unique; a prefix cannot end in a digit, and `@` is reserved for enum and variant references. Registrations are fixed when serialization begins. Rows follow first encounter order, including dependencies encountered inside other rows. Values that compare equal under their native equality share a row.

The result, table snapshot, and enum snapshot are separate values. Registered tables remain present even when empty. Snapshots contain the rows collected so far; later serialization does not change an earlier snapshot. Callers select the values to serialize and compose their output. Registering tables controls deduplication, not traversal: serializing a task includes its native domain and task structure.

Sequences are JSON arrays, pairs are two-element arrays, absent optional values are `null`, and paths are strings with `/` separators. Ordinary fields retain their native names. Variant wrappers contain `kind` and `value`, for example `{"kind":"@0","value":3}`. The kind references an encountered native alternative; `value` is its inline representation or dictionary reference. Enum fields also use these references. Numeric data such as constants, costs, indices, and FDR values remain numbers.

`enums()` groups encountered alternatives by native type and sorts each group by native numeric ID:

```json
{
  "FunctionExpression": [
    {"ref": "@0", "id": 0, "name": "constant"}
  ]
}
```

Legend references are unique across all enum and variant types in the registry, including Runir types. They are assigned on first encounter and remain stable when another entry is inserted earlier in a sorted legend. Resolve them through `ref`; neither the native `id` nor the legend row position determines the reference.

Python callers can render snapshots with Yggdrasil's generic `tabulate` adapter:

```python
from pyyggdrasil.serialization import render_table

for name, table in dictionaries.tables().items():
    print(name)
    print(render_table(table["rows"], prefix=table["prefix"]))

for name, entries in dictionaries.enums().items():
    print(name)
    print(render_table(entries))
```

The optional prefix adds entity references in an index column. Nested lists and objects become compact JSON cells; scalar formatting and layouts come from `tabulate`. Pass `tablefmt="github"` for Markdown. The renderer returns text and leaves file handling and report layout to the caller.

Snapshots are ordinary Python dictionaries and lists. Add application columns to snapshot rows before rendering, matching annotations by entity reference rather than evidence-list position. Preserve row order when using `prefix`; if sorting or filtering, put the original references into explicit cells first and omit `prefix`. These edits do not affect the registry or later snapshots. Shared `JSONValue`, `Row`, `Table`, and `EnumEntry` types live in `pyyggdrasil.serialization`; `pytyr.serialization.dictionaries` re-exports them for compatibility.

Planning states contain only `fluent_facts`, `derived_atoms`, and `fluent_fterm_values`. Static facts belong to the task representation. Function-term values are pairs of a term representation or reference and its numeric value. An FDR fact preserves its `variable` and numeric `value`; zero represents the native none value.

Nodes contain `state` and `metric`. Labeled nodes contain `label` and `node`. Plans contain `start_node`, `labeled_succ_nodes`, `length`, and `cost`; repeated states and nodes reuse their registered references. Ground and lifted task wrappers contain `formalism_task`, which preserves the underlying task and domain information without exposing runtime caches.
